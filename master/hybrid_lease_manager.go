package kea

import (
	"net"
)

type HybridLeaseManager struct {
	mem  LeaseManager
	pool *LeaseDBPool
}

func newHybridLeaseManager(mem LeaseManager, pool *LeaseDBPool) LeaseManager {
	manager := &HybridLeaseManager{
		mem:  mem,
		pool: pool,
	}
	return manager
}

func (manager *HybridLeaseManager) GetLeaseWithIp(ip net.IP) *Lease {
	return manager.mem.GetLeaseWithIp(ip)
}

func (manager *HybridLeaseManager) GetLeaseWithMac(mac net.HardwareAddr) *Lease {
	return manager.mem.GetLeaseWithMac(mac)
}

func (manager *HybridLeaseManager) GetLeaseWithClient(clientID ClientID) *Lease {
	return manager.mem.GetLeaseWithClient(clientID)
}

func (manager *HybridLeaseManager) GetLeaseWithClientAndMac(clientID ClientID, mac net.HardwareAddr) *Lease {
	return manager.mem.GetLeaseWithClientAndMac(clientID, mac)
}

func (manager *HybridLeaseManager) AddLease(lease *Lease) error {
	err := manager.mem.AddLease(lease)
	if err == nil {
		manager.pool.AddLease(lease)
	}
	return err
}

func (manager *HybridLeaseManager) UpdateLease(lease *Lease) error {
	err := manager.mem.UpdateLease(lease)
	if err == nil {
		manager.pool.UpdateLease(lease)
	}
	return err

}

func (manager *HybridLeaseManager) DeleteLease(subnetId SubnetID, addr net.IP) {
	manager.mem.DeleteLease(subnetId, addr)
	manager.pool.DeleteLease(subnetId, addr)
}

func (manager *HybridLeaseManager) GetLeaseCount(subnetId SubnetID) int {
	return manager.mem.GetLeaseCount(subnetId)
}
