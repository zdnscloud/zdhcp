package kea

import (
	"bytes"
	"fmt"
	"net"
)

type LeaseStore struct {
	ip_to_leases       map[uint32]*Lease
	mac_to_leases      map[uint64]*Lease
	clientid_to_leases map[string]*Lease
}

func keyForIP(ip net.IP) uint32 {
	return uint32(ip[0]) | uint32(ip[1])<<8 | uint32(ip[2])<<16 | uint32(ip[3])<<24
}

func keyForMac(mac net.HardwareAddr) uint64 {
	return (uint64(mac[0]) | uint64(mac[1])<<8 | uint64(mac[2])<<16 | uint64(mac[3])<<24 |
		uint64(mac[4])<<32 | uint64(mac[5])<<40)
}

func newLeaseStore() LeaseManager {
	return &LeaseStore{
		ip_to_leases:       make(map[uint32]*Lease),
		mac_to_leases:      make(map[uint64]*Lease),
		clientid_to_leases: make(map[string]*Lease),
	}
}

func (store *LeaseStore) GetLeaseWithIp(ip net.IP) *Lease {
	lease, _ := store.ip_to_leases[keyForIP(ip)]
	return lease
}

func (store *LeaseStore) GetLeaseWithMac(mac net.HardwareAddr) *Lease {
	lease, _ := store.mac_to_leases[keyForMac(mac)]
	return lease
}

func (store *LeaseStore) GetLeaseWithClient(clientID ClientID) *Lease {
	lease, _ := store.clientid_to_leases[string(clientID)]
	return lease
}

func (store *LeaseStore) GetLeaseWithClientAndMac(clientID ClientID, mac net.HardwareAddr) *Lease {
	lease := store.GetLeaseWithClient(clientID)
	if lease != nil && bytes.Equal(lease.Mac, mac) {
		return lease
	} else {
		return nil
	}
}

func (store *LeaseStore) AddLease(lease *Lease) error {
	key := keyForIP(lease.Address)
	var err error
	if _, ok := store.ip_to_leases[keyForIP(lease.Address)]; ok == false {
		store.ip_to_leases[key] = lease
		if lease.Mac != nil {
			store.mac_to_leases[keyForMac(lease.Mac)] = lease
		}

		if lease.ClientId != nil {
			store.clientid_to_leases[string(lease.ClientId)] = lease
		}
	} else {
		err = fmt.Errorf("lease with ip %v already exists", lease.Address)
	}
	return err
}

func (store *LeaseStore) UpdateLease(lease *Lease) error {
	store.DeleteLease(lease.SubnetId, lease.Address)
	return store.AddLease(lease)
}

func (store *LeaseStore) DeleteLease(subnetId SubnetID, ip net.IP) {
	lease := store.GetLeaseWithIp(ip)
	if lease != nil && lease.SubnetId == subnetId {
		delete(store.ip_to_leases, keyForIP(ip))
		if lease.Mac != nil {
			macKey := keyForMac(lease.Mac)
			if oldLease, ok := store.mac_to_leases[macKey]; ok && oldLease.Address.Equal(ip) {
				delete(store.mac_to_leases, macKey)
			}
		}

		if lease.ClientId != nil {
			if oldLease, ok := store.clientid_to_leases[string(lease.ClientId)]; ok &&
				oldLease.Address.Equal(ip) {
				delete(store.clientid_to_leases, string(lease.ClientId))
			}
		}
	}
}

func (store *LeaseStore) GetLeaseCount(subnetId SubnetID) int {
	leaseCount := 0
	for _, lease := range store.ip_to_leases {
		if lease.SubnetId == subnetId {
			leaseCount += 1
		}
	}
	return leaseCount
}
