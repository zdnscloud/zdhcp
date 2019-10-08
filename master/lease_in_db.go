package kea

import (
	"bytes"
	"encoding/hex"
	"fmt"
	"net"
	"strings"
	"time"

	"kea/util"
	"quark/rest"
)

type Lease4 struct {
	Id            string
	Address       uint32 `sql:"uk"`
	State         int
	ValidLifeTime uint32
	ClientId      string
	SubnetId      uint32
	Expire        time.Time
	HostName      string
	Mac           string
}

func (lease *Lease4) Validate() error {
	return nil
}

func FromLease(lease *Lease) *Lease4 {
	return &Lease4{
		Address:       util.IPv4ToLongHBO(lease.Address),
		State:         int(lease.State),
		ValidLifeTime: uint32(lease.ValidLifeTime.Seconds()),
		ClientId:      hex.EncodeToString(lease.ClientId),
		SubnetId:      uint32(lease.SubnetId),
		Expire:        lease.GetExpiredTime(),
		HostName:      lease.HostName,
		Mac:           lease.Mac.String(),
	}
}

func (lease *Lease4) ToLease() *Lease {
	clientId, _ := hex.DecodeString(lease.ClientId)
	mac, _ := net.ParseMAC(lease.Mac)
	return &Lease{
		Address:             util.IPv4FromLongHBO(lease.Address),
		State:               LeaseState(lease.State),
		ValidLifeTime:       (time.Second * time.Duration(lease.ValidLifeTime)),
		ClientId:            clientId,
		SubnetId:            SubnetID(lease.SubnetId),
		ClientLastTransTime: lease.Expire.Add(-1 * time.Second * time.Duration(lease.ValidLifeTime)),
		HostName:            lease.HostName,
		Mac:                 mac,
	}
}

type LeaseDB struct {
	rest.ResourceStore
}

func NewLeaseDB(host, user, passwd, dbname string) (LeaseManager, error) {
	meta, err := rest.NewResourceMeta([]rest.Resource{&Lease4{}})
	if err != nil {
		return nil, err
	}

	var postgresqlDBConnInfo = map[string]interface{}{
		"host":     host,
		"user":     user,
		"password": passwd,
		"dbname":   dbname,
	}
	store, err := rest.NewRStore(rest.Postgresql, postgresqlDBConnInfo, meta)
	util.Logger().Info("connect to -h %s -U %s -p %s -d %s", host, user, passwd, dbname)
	if err != nil {
		return nil, err
	}

	return &LeaseDB{store}, nil
}

func (db *LeaseDB) GetLeaseWithIp(ip net.IP) *Lease {
	return db.getLeaseWithAttrs(map[string]interface{}{"address": util.IPv4ToLongHBO(ip)})
}

func (db *LeaseDB) getLeasesWithAttrs(attrs map[string]interface{}) []Lease4 {
	tx, err := db.Begin()
	if err != nil {
		util.Logger().Error("create transaction failed:%s", err.Error())
		return nil
	}

	leases_, err := tx.Get("lease4", attrs)
	tx.Commit()
	if err == nil {
		return leases_.([]Lease4)
	} else {
		util.Logger().Error("get lease4 failed:%s", err.Error())
		return nil
	}
}

func (db *LeaseDB) getLeaseWithAttrs(attrs map[string]interface{}) *Lease {
	leases := db.getLeasesWithAttrs(attrs)
	if len(leases) == 1 {
		return leases[0].ToLease()
	} else {
		return nil
	}
}

func (db *LeaseDB) GetLeaseWithMac(mac net.HardwareAddr) *Lease {
	return db.getLeaseWithAttrs(map[string]interface{}{"mac": mac.String()})
}

func (db *LeaseDB) GetLeaseWithClient(clientID ClientID) *Lease {
	return db.getLeaseWithAttrs(map[string]interface{}{
		"client_id": hex.EncodeToString(clientID),
	})
}

func (db *LeaseDB) GetLeaseWithClientAndMac(clientID ClientID, mac net.HardwareAddr) *Lease {
	return db.getLeaseWithAttrs(map[string]interface{}{
		"client_id": hex.EncodeToString(clientID),
		"mac":       mac.String(),
	})
}

func (db *LeaseDB) AddLease(lease *Lease) error {
	return rest.WithTx(db, func(tx rest.Transaction) error {
		return db.AddLeaseInTx(tx, lease)
	})
}

func (db *LeaseDB) AddLeaseInTx(tx rest.Transaction, lease *Lease) error {
	_, err := tx.Insert(FromLease(lease))
	if err != nil {
		util.Logger().Error("add lease failed:%s", err.Error())
	}
	return err
}

func (db *LeaseDB) UpdateLease(lease *Lease) error {
	return rest.WithTx(db, func(tx rest.Transaction) error {
		return db.UpdateLeaseInTx(tx, lease)
	})
}

func (db *LeaseDB) UpdateLeaseInTx(tx rest.Transaction, lease *Lease) error {
	lease4 := FromLease(lease)
	lease4Map, err := rest.ResourceToMap(lease4)
	if err != nil {
		return err
	}
	_, err = tx.Update("lease4", lease4Map, map[string]interface{}{"address": lease4.Address})
	return err
}

func (db *LeaseDB) DeleteLease(subnetId SubnetID, addr net.IP) {
	rest.WithTx(db, func(tx rest.Transaction) error {
		db.DeleteLeaseInTx(tx, subnetId, addr)
		return nil
	})
}

func (db *LeaseDB) DeleteLeaseInTx(tx rest.Transaction, subnetId SubnetID, addr net.IP) {
	if addr != nil {
		tx.Delete("lease4", map[string]interface{}{"address": util.IPv4ToLongHBO(addr), "subnet_id": subnetId})
	} else {
		tx.Delete("lease4", map[string]interface{}{"subnet_id": subnetId})
	}
}

type LoadLeaseTask struct {
	notifyChan   chan struct{}
	subnet       *Subnet
	leaseManager LeaseManager
}

func (db *LeaseDB) LoadLease(task *LoadLeaseTask) {
	leaseManager := task.leaseManager
	if len(task.subnet.Pools) == 0 {
		util.Logger().Warn("delete lease which subnet id is %d", task.subnet.Id)
		db.DeleteLease(task.subnet.Id, nil)
	} else {
		db.deleteLeaseNotInSubnet(task.subnet)
		leases := db.getLeasesWithAttrs(map[string]interface{}{"subnet_id": task.subnet.Id})
		for _, lease := range leases {
			leaseManager.AddLease(lease.ToLease())
		}
	}
	task.notifyChan <- struct{}{}
}

func (db *LeaseDB) deleteLeaseNotInSubnet(subnet *Subnet) {
	tx, err := db.Begin()
	if err != nil {
		util.Logger().Error("create transaction failed:%s", err.Error())
		return
	}

	var sqlBuf bytes.Buffer
	sqlBuf.WriteString(fmt.Sprintf("delete from zc_lease4 where subnet_id = %d and not(", subnet.Id))
	for _, pool := range subnet.Pools {
		sqlBuf.WriteString(fmt.Sprintf("(address >= %d and address <= %d)", pool.Start, pool.End))
		sqlBuf.WriteString(" or ")
	}

	sql := strings.TrimRight(sqlBuf.String(), "or ") + ")"
	deletedLeaseCount, err := tx.DeleteEx(sql)
	if err == nil {
		if deletedLeaseCount > 0 {
			util.Logger().Warn("delete %d lease which is out of pool", deletedLeaseCount)
		}
		tx.Commit()
	} else {
		tx.RollBack()
		util.Logger().Error("delete lease not in subnet failed:%s", err.Error())
	}
}

func (db *LeaseDB) Close() {
	db.Destroy()
}

func (db *LeaseDB) GetLeaseCount(subnetId SubnetID) int {
	return len(db.getLeasesWithAttrs(map[string]interface{}{"subnet_id": subnetId}))
}
