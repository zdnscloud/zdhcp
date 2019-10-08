package kea

import (
	"kea/util"
	"net"
	"testing"

	ut "cement/unittest"
)

var (
	subnetId = SubnetID(3171095)
	ips      = []net.IP{util.IPv4FromString("1.1.1.1"), util.IPv4FromString("1.1.1.2"), util.IPv4FromString("1.1.1.3"), util.IPv4FromString("1.1.1.4"), util.IPv4FromString("1.1.1.5")}
)

func TestAddAndDeleteLease(t *testing.T) {
	store := newLeaseStore()
	mac, err := net.ParseMAC("01:23:45:67:89:ab")
	ut.Assert(t, err == nil, "err should be nil but %v", err)
	mac2, err := net.ParseMAC("01:23:45:67:89:ac")
	ut.Assert(t, err == nil, "err should be nil but %v", err)

	leases := []*Lease{
		&Lease{
			Address:  ips[0],
			SubnetId: subnetId,
			Mac:      mac,
		},
		&Lease{
			Address:  ips[1],
			SubnetId: subnetId,
			Mac:      mac,
		},
		&Lease{
			Address:  ips[2],
			SubnetId: subnetId,
			Mac:      mac,
		},
		&Lease{
			Address:  ips[3],
			SubnetId: subnetId,
			Mac:      mac2,
		},
		&Lease{
			Address:  ips[4],
			SubnetId: subnetId,
			Mac:      mac2,
		},
	}

	for _, lease := range leases {
		err = store.AddLease(lease)
		ut.Assert(t, err == nil, "err should be nil but %v", err)
	}

	oldLeaseByMac := store.GetLeaseWithMac(mac)
	ut.Assert(t, oldLeaseByMac.Address.Equal(ips[0]) == false, "ip of lease which found by mac shoud not be 1.1.1.1")
	ut.Assert(t, oldLeaseByMac.Address.Equal(ips[1]) == false, "ip of lease which found by mac shoud not be 1.1.1.2")
	ut.Assert(t, oldLeaseByMac.Address.Equal(ips[2]), "ip of lease which found by mac shoud be 1.1.1.3")

	oldLeaseByMac2 := store.GetLeaseWithMac(mac2)
	ut.Assert(t, oldLeaseByMac2.Address.Equal(ips[3]) == false, "ip of lease which found by mac2 shoud not be 1.1.1.4")
	ut.Assert(t, oldLeaseByMac2.Address.Equal(ips[4]), "ip of lease which found by mac2 shoud be 1.1.1.5")

	ut.Assert(t, store.GetLeaseWithIp(ips[0]) != nil, "should found lease with 1.1.1.1")
	store.DeleteLease(subnetId, ips[0])
	ut.Assert(t, store.GetLeaseWithIp(ips[0]) == nil, "should not found deleted lease with 1.1.1.1")
	ut.Assert(t, store.GetLeaseWithMac(mac) != nil, "should found lease by mac")

	ut.Assert(t, store.GetLeaseWithIp(ips[1]) != nil, "should found lease with 1.1.1.2")
	store.DeleteLease(subnetId, ips[1])
	ut.Assert(t, store.GetLeaseWithIp(ips[1]) == nil, "should not found deleted lease with 1.1.1.2")
	ut.Assert(t, store.GetLeaseWithMac(mac) != nil, "should found lease by mac")

	ut.Assert(t, store.GetLeaseWithIp(ips[2]) != nil, "should found lease with 1.1.1.3")
	store.DeleteLease(subnetId, ips[2])
	ut.Assert(t, store.GetLeaseWithIp(ips[2]) == nil, "should not found deleted lease with 1.1.1.3")
	ut.Assert(t, store.GetLeaseWithMac(mac) == nil, "should not found deleted lease by mac")

	ut.Assert(t, store.GetLeaseWithIp(ips[3]) != nil, "should found lease with 1.1.1.4")
	store.DeleteLease(subnetId, ips[3])
	ut.Assert(t, store.GetLeaseWithIp(ips[3]) == nil, "should not found deleted lease with 1.1.1.4")
	ut.Assert(t, store.GetLeaseWithMac(mac2) != nil, "should found lease by mac2")

	ut.Assert(t, store.GetLeaseWithIp(ips[4]) != nil, "should found lease with 1.1.1.5")
	store.DeleteLease(subnetId, ips[4])
	ut.Assert(t, store.GetLeaseWithIp(ips[4]) == nil, "should not found deleted lease with 1.1.1.5")
	ut.Assert(t, store.GetLeaseWithMac(mac2) == nil, "should not found deleted lease by mac2")
}

func TestAddAndDeleteLeaseByClientId(t *testing.T) {
	store := newLeaseStore()
	clientId := ClientID([]byte("013c0cdb20b331"))
	clientId2 := ClientID([]byte("013c0cdb20b332"))

	leases := []*Lease{
		&Lease{
			Address:  ips[0],
			SubnetId: subnetId,
			ClientId: ClientID(clientId),
		},
		&Lease{
			Address:  ips[1],
			SubnetId: subnetId,
			ClientId: ClientID(clientId),
		},
		&Lease{
			Address:  ips[2],
			SubnetId: subnetId,
			ClientId: ClientID(clientId),
		},
		&Lease{
			Address:  ips[3],
			SubnetId: subnetId,
			ClientId: ClientID(clientId2),
		},
		&Lease{
			Address:  ips[4],
			SubnetId: subnetId,
			ClientId: ClientID(clientId2),
		},
	}

	for _, lease := range leases {
		err := store.AddLease(lease)
		ut.Assert(t, err == nil, "err should be nil but %v", err)
	}

	oldLease := store.GetLeaseWithClient(clientId)
	ut.Assert(t, oldLease.Address.Equal(ips[0]) == false, "ip of lease which found by clientId shoud not be 1.1.1.1")
	ut.Assert(t, oldLease.Address.Equal(ips[1]) == false, "ip of lease which found by clientId shoud not be 1.1.1.2")
	ut.Assert(t, oldLease.Address.Equal(ips[2]), "ip of lease which found by clientId shoud be 1.1.1.3")

	ut.Assert(t, store.GetLeaseWithIp(ips[0]) != nil, "should found lease with 1.1.1.1")
	store.DeleteLease(subnetId, ips[0])
	ut.Assert(t, store.GetLeaseWithIp(ips[0]) == nil, "should not found deleted lease with 1.1.1.1")
	ut.Assert(t, store.GetLeaseWithClient(clientId) != nil, "should found lease by clientId")

	ut.Assert(t, store.GetLeaseWithIp(ips[1]) != nil, "should found lease with 1.1.1.2")
	store.DeleteLease(subnetId, ips[1])
	ut.Assert(t, store.GetLeaseWithIp(ips[1]) == nil, "should not found deleted lease with 1.1.1.2")
	ut.Assert(t, store.GetLeaseWithClient(clientId) != nil, "should found lease by clientId")

	ut.Assert(t, store.GetLeaseWithIp(ips[2]) != nil, "should found lease with 1.1.1.3")
	store.DeleteLease(subnetId, ips[2])
	ut.Assert(t, store.GetLeaseWithIp(ips[2]) == nil, "should not found deleted lease with 1.1.1.3")
	ut.Assert(t, store.GetLeaseWithClient(clientId) == nil, "should found deleted lease by clientId")

	ut.Assert(t, store.GetLeaseWithIp(ips[3]) != nil, "should found lease with 1.1.1.4")
	store.DeleteLease(subnetId, ips[3])
	ut.Assert(t, store.GetLeaseWithIp(ips[3]) == nil, "should not found deleted lease with 1.1.1.4")
	ut.Assert(t, store.GetLeaseWithClient(clientId2) != nil, "should found lease by clientId")

	ut.Assert(t, store.GetLeaseWithIp(ips[4]) != nil, "should found lease with 1.1.1.5")
	store.DeleteLease(subnetId, ips[4])
	ut.Assert(t, store.GetLeaseWithIp(ips[4]) == nil, "should not found deleted lease with 1.1.1.5")
	ut.Assert(t, store.GetLeaseWithClient(clientId2) == nil, "should found deleted lease by clientId")
}
