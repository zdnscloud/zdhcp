package kea

import (
	"fmt"
	"math/rand"
	"net"
	"sync"
	"time"

	"cement/randomdata"
	"kea/util"
	"quark/rest"
)

type AddSubnet struct {
	SubnetId uint `json:"id"`
	Ttl      uint `json:"default-valid-lifetime"`
}

func (s *AddSubnet) String() string {
	return fmt.Sprintf("add subnet and id: %d with default-valid-lifetime: %d\n", s.SubnetId, s.Ttl)
}

type DeleteSubnet struct {
	SubnetId uint `json:"id"`
}

func (s *DeleteSubnet) String() string {
	return fmt.Sprintf("delete subnet %d\n", s.SubnetId)
}

type UpdateSubnet struct {
	SubnetId uint `json:"id"`
	Ttl      uint `json:"default-valid-lifetime"`
}

func (s *UpdateSubnet) String() string {
	return fmt.Sprintf("update subnet and id: %d and default-valid-lifetime: %d\n", s.SubnetId, s.Ttl)
}

type AddPool struct {
	SubnetId uint   `json:"id"`
	StartIP  string `json:"start-ip"`
	EndIP    string `json:"end-ip"`
	HWAddr   string `json:"hw-address"`
	IPAddr   string `json:"ip-address"`
	Reserved bool   `json:"reservated-addr"`
}

func (p *AddPool) String() string {
	if p.StartIP != "" {
		return fmt.Sprintf("add pool %s-%s to subnet %d\n", p.StartIP, p.EndIP, p.SubnetId)
	} else {
		return fmt.Sprintf("add reservation pool %s|%s to subnet %d\n", p.HWAddr, p.IPAddr, p.SubnetId)
	}
}

type DeletePool struct {
	SubnetId uint   `json:"id"`
	StartIP  string `json:"start-ip"`
	EndIP    string `json:"end-ip"`
	HWAddr   string `json:"hw-address"`
	IPAddr   string `json:"ip-address"`
	Reserved bool   `json:"reservated-addr"`
}

func (p *DeletePool) String() string {
	if p.StartIP != "" {
		return fmt.Sprintf("delete pool %s-%s from subnet %d\n", p.StartIP, p.EndIP, p.SubnetId)
	} else {
		return fmt.Sprintf("delete reservation pool %s|%s from subnet %d\n", p.HWAddr, p.IPAddr, p.SubnetId)
	}
}

type AddSharedNetwork struct {
	Name      string `json:"name"`
	SubnetIds []uint `json:"subnet-ids"`
}

func (s *AddSharedNetwork) String() string {
	return fmt.Sprintf("add shared network %s with subnet-ids: %v\n", s.Name, s.SubnetIds)
}

type DeleteSharedNetwork struct {
	Name string `json:"name"`
}

func (s *DeleteSharedNetwork) String() string {
	return fmt.Sprintf("delete shared network %s\n", s.Name)
}

type UpdateSharedNetwork struct {
	Name      string `json:"name"`
	SubnetIds []uint `json:"subnet-ids"`
}

func (s *UpdateSharedNetwork) String() string {
	return fmt.Sprintf("update shared network %s with subnet-ids: %v\n", s.Name, s.SubnetIds)
}

func (s *Server) addSubnet(subnetId SubnetID, ttl uint) error {
	s.stopClient()

	if _, ok := s.allocator.engines[subnetId]; ok {
		return fmt.Errorf("add duplicate subnet with id: %d\n", subnetId)
	}

	s.allocator.engines[subnetId] = newEngine(&Subnet{
		Id:            subnetId,
		RenewTime:     time.Duration(4000) * time.Second,
		RebindTime:    time.Duration(4000) * time.Second,
		ValidLifeTime: time.Duration(ttl) * time.Second,
	}, newLeaseStore())
	s.allocator.engineLocks[subnetId] = &sync.Mutex{}
	LeaseDBPoolInstance().AllocateNewDBConn(subnetId)

	return nil
}

func (pool *LeaseDBPool) AllocateNewDBConn(subnetID SubnetID) {
	randomdata.SeedUsingNow()
	pool.subnetToDbIndex[subnetID] = rand.Intn(DefaultDBConnCount)
}

func parsePool(startIpStr, endIpStr string) (*Pool, error) {
	startIp := util.IPv4FromString(startIpStr)
	if startIp == nil {
		return nil, fmt.Errorf("start ip %s in pool isn't valid", startIpStr)
	}

	endIp := util.IPv4FromString(endIpStr)
	if endIp == nil {
		return nil, fmt.Errorf("end ip %s in pool isn't valid", endIpStr)
	}

	return newPool(startIp, endIp)
}

func (s *Server) deleteSubnet(subnetId SubnetID) error {
	s.stopClient()

	engine, ok := s.allocator.engines[subnetId]
	if ok == false {
		return fmt.Errorf("delete non-exist subnet with id: %d\n", subnetId)
	}

	engine.LeaseManager().DeleteSubnet(subnetId)
	delete(s.allocator.engines, subnetId)
	delete(s.allocator.engineLocks, subnetId)
	return nil
}

func (e *SubnetEngine) LeaseManager() LeaseManager {
	return e.leaseManager
}

func (manager *HybridLeaseManager) DeleteSubnet(subnetId SubnetID) {
	manager.mem.DeleteSubnet(subnetId)
	manager.pool.DeleteSubnet(subnetId)
}

func (store *LeaseStore) DeleteSubnet(subnetId SubnetID) {
	for _, lease := range store.ip_to_leases {
		if lease.SubnetId == subnetId {
			delete(store.ip_to_leases, keyForIP(lease.Address))
			if lease.Mac != nil {
				macKey := keyForMac(lease.Mac)
				if oldLease, ok := store.mac_to_leases[macKey]; ok && oldLease.SubnetId == subnetId {
					delete(store.mac_to_leases, macKey)
				}
			}

			if lease.ClientId != nil {
				if oldLease, ok := store.clientid_to_leases[string(lease.ClientId)]; ok &&
					oldLease.SubnetId == subnetId {
					delete(store.clientid_to_leases, string(lease.ClientId))
				}
			}
		}
	}
}

func (pool *LeaseDBPool) DeleteSubnet(subnetId SubnetID) {
	pool.asyncDoOption(subnetId, LeaseOpt{
		typ: OptDeleteSubnet,
		param: &deleteLeaseTask{
			subnetId: subnetId,
		},
	})
}

func (db *LeaseDB) DeleteSubnetInTx(tx rest.Transaction, subnetId SubnetID) {
	tx.Delete("lease4", map[string]interface{}{"subnet_id": subnetId})
}

func (db *LeaseDB) DeleteSubnet(subnetId SubnetID) {
	rest.WithTx(db, func(tx rest.Transaction) error {
		db.DeleteSubnetInTx(tx, subnetId)
		return nil
	})
}

func (s *Server) updateSubnet(subnetId SubnetID, ttl uint) error {
	s.stopClient()

	engine, ok := s.allocator.engines[subnetId]
	if ok == false {
		return fmt.Errorf("operate non-exist subnet with id: %d\n", subnetId)
	}

	engine.Subnet().ValidLifeTime = time.Duration(ttl) * time.Second
	return nil
}

func (s *Server) addPool(subnetId SubnetID, startIp, endIp, hwaddr, ipaddr string, reserved bool) error {
	s.stopClient()

	engine, ok := s.allocator.engines[subnetId]
	if ok == false {
		return fmt.Errorf("operate non-exist subnet with id: %d\n", subnetId)
	}

	if hwaddr == "" {
		pool, err := parsePool(startIp, endIp)
		if err != nil {
			return err
		}

		if reserved {
			engine.LeaseManager().DeletePool(subnetId, pool)
			engine.Subnet().ReservedPools = append(engine.Subnet().ReservedPools, pool)
		} else {
			engine.Subnet().Pools = append(engine.Subnet().Pools, pool)
		}
	} else {
		host, err := parseHost(hwaddr, ipaddr)
		if err != nil {
			return err
		}

		hostManager, ok := HostManagerInstance().managers[subnetId]
		if ok == false {
			hostManager = newHostStore()
			HostManagerInstance().addHostManager(subnetId, hostManager)
		}

		hostManager.AddHost(host)
		engine.SetHostManager(hostManager)
	}

	return nil
}

func parseHost(hwaddr, ipaddr string) (*Host, error) {
	mac, err := net.ParseMAC(hwaddr)
	if err != nil {
		return nil, fmt.Errorf("parse mac %s of host faild %s", hwaddr, err.Error())
	}

	ip := util.IPv4FromString(ipaddr)
	if ip == nil {
		return nil, fmt.Errorf("parse ip %s of host faild", ipaddr)
	}

	return &Host{
		ReserveAddr: ip,
		Mac:         mac,
	}, nil
}

func (engine *SubnetEngine) SetHostManager(hostManager HostManager) {
	engine.hostManager = hostManager
}

func (s *Server) deletePool(subnetId SubnetID, startIp, endIp, hwaddr, ipaddr string, reserved bool) error {
	s.stopClient()

	engine, ok := s.allocator.engines[subnetId]
	if ok == false {
		return fmt.Errorf("operate non-exist subnet with id: %d\n", subnetId)
	}

	if hwaddr == "" {
		pool, err := parsePool(startIp, endIp)
		if err != nil {
			return err
		}

		if reserved {
			if pools, ok := deletePoolFromPools(pool, engine.Subnet().ReservedPools); ok {
				engine.Subnet().ReservedPools = pools
			} else {
				return fmt.Errorf("no found reservation pool %s-%s for subnet-id %d", startIp, endIp, subnetId)
			}
		} else {
			if pools, ok := deletePoolFromPools(pool, engine.Subnet().Pools); ok {
				engine.Subnet().Pools = pools
				engine.LeaseManager().DeletePool(subnetId, pool)
			} else {
				return fmt.Errorf("no found pool %s-%s for subnet-id %d", startIp, endIp, subnetId)
			}
		}
	} else {
		host, err := parseHost(hwaddr, ipaddr)
		if err != nil {
			return err
		}

		hostManager, ok := HostManagerInstance().managers[subnetId]
		if ok == false {
			return fmt.Errorf("operate non-exist host in subnet %d\n", subnetId)
		}

		engine.LeaseManager().DeleteLease(subnetId, host.ReserveAddr)
		if err := hostManager.DeleteHost(host); err != nil {
			return err
		}

		engine.SetHostManager(hostManager)
	}

	return nil
}

func deletePoolFromPools(pool *Pool, pools []*Pool) ([]*Pool, bool) {
	rePools := pools
	found := false
	for i, p := range pools {
		if p.Start == pool.Start && p.End == pool.End {
			rePools = append(pools[:i], pools[i+1:]...)
			found = true
			break
		}
	}

	return rePools, found
}

func (manager *HybridLeaseManager) DeletePool(subnetId SubnetID, pool *Pool) {
	manager.mem.DeletePool(subnetId, pool)
	manager.pool.DeletePool(subnetId, pool)
}

func (store *LeaseStore) DeletePool(subnetId SubnetID, pool *Pool) {
	for ip := pool.Start; ip <= pool.End; ip++ {
		store.DeleteLease(subnetId, util.IPv4FromLongHBO(ip))
	}
}

func (pool *LeaseDBPool) DeletePool(subnetId SubnetID, p *Pool) {
	pool.asyncDoOption(subnetId, LeaseOpt{
		typ: OptDeletePool,
		param: &deleteLeaseTask{
			subnetId: subnetId,
			startIp:  p.Start,
			endIp:    p.End,
		},
	})
}

func (db *LeaseDB) DeletePoolInTx(tx rest.Transaction, subnetId SubnetID, startIp, endIp uint32) {
	tx.DeleteEx(fmt.Sprintf("delete from zc_lease4 where subnet_id = %d and address >= %d and address <= %d",
		subnetId, startIp, endIp))
}

func (db *LeaseDB) DeletePool(subnetId SubnetID, pool *Pool) {
	rest.WithTx(db, func(tx rest.Transaction) error {
		db.DeletePoolInTx(tx, subnetId, pool.Start, pool.End)
		return nil
	})
}

func (store *HostStore) DeleteHost(host *Host) error {
	key := host.Mac.String()
	oldHost, ok := store.hosts[key]
	if ok == false {
		return fmt.Errorf("operate non-exist host with mac %s and ip %s\n", host.Mac.String(), host.ReserveAddr.String())
	}

	if host.ReserveAddr.Equal(oldHost.ReserveAddr) == false {
		return fmt.Errorf("ip %s don`t match host ip %s for mac %s\n",
			host.ReserveAddr.String(), oldHost.ReserveAddr.String(), host.Mac.String())
	}

	delete(store.hosts, key)
	return nil
}

func (s *Server) addSharedNetwork(name string, subnetIds []uint) error {
	s.stopClient()
	s.allocator.sharedSubnetMgr.addSharedSubnet(name, parseSubnetIDs(subnetIds))
	return nil
}

func parseSubnetIDs(subnetIds []uint) []SubnetID {
	subnetIDs := make([]SubnetID, len(subnetIds), len(subnetIds))
	for i, id := range subnetIds {
		subnetIDs[i] = SubnetID(id)
	}

	return subnetIDs
}

func (s *Server) deleteSharedNetwork(name string) error {
	s.stopClient()
	s.allocator.sharedSubnetMgr.deleteSharedSubnet(name)
	return nil
}

func (mgr *SharedSubnetMgr) deleteSharedSubnet(name string) {
	for i, subnet := range mgr.sharedSubnets {
		if subnet.name == name {
			mgr.sharedSubnets = append(mgr.sharedSubnets[:i], mgr.sharedSubnets[i+1:]...)
			break
		}
	}
}

func (s *Server) updateSharedNetwork(name string, subnetIds []uint) error {
	s.stopClient()
	s.allocator.sharedSubnetMgr.updateSharedSubnet(name, parseSubnetIDs(subnetIds))
	return nil
}

func (mgr *SharedSubnetMgr) updateSharedSubnet(name string, subnetIds []SubnetID) {
	for i, subnet := range mgr.sharedSubnets {
		if subnet.name == name {
			mgr.sharedSubnets[i].subnetIDs = subnetIds
			break
		}
	}
}
