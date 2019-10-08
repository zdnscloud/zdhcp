package kea

import (
	"fmt"
	"kea/util"
	"net"
	"testing"
	"time"

	ut "cement/unittest"
)

func strToIP(s string) net.IP {
	return net.ParseIP(s).To4()
}

func strToMac(s string) net.HardwareAddr {
	mac, _ := net.ParseMAC(s)
	return mac
}

func createLease(ip net.IP, mac net.HardwareAddr) *Lease {
	return &Lease{
		Address:             ip,
		State:               Normal,
		ValidLifeTime:       time.Duration(3600) * time.Second,
		ClientId:            []byte{1, 2, 3, 4},
		SubnetId:            1,
		ClientLastTransTime: time.Now(),
		HostName:            "www.zdns.cn.",
		Mac:                 mac,
	}
}

func TestCURDLease(t *testing.T) {
	util.InitLog("", "debug")
	db, err := NewLeaseDB("127.0.0.1", "joe", "zdns", "zdns")
	ut.Assert(t, err == nil, "err should be nil but %v", err)

	addr1 := strToIP("1.1.1.1")
	mac1 := strToMac("00:00:00:00:00:01")
	lease1 := createLease(addr1, mac1)

	addr2 := strToIP("1.1.1.2")
	mac2 := strToMac("00:00:00:00:00:02")
	lease2 := createLease(addr2, mac2)

	ut.Assert(t, db.AddLease(lease1) == nil, "add lease should ok but get %v\n", err)
	ut.Assert(t, db.AddLease(lease2) == nil, "add lease should ok but get %v\n", err)

	lease1InDB := db.GetLeaseWithIp(addr1)
	ut.Equal(t, lease1InDB.Mac, mac1)
	lease2InDB := db.GetLeaseWithIp(addr2)
	ut.Equal(t, lease2InDB.Mac, mac2)

	mac3 := strToMac("00:00:00:00:00:03")
	lease2InDB.Mac = mac3
	ut.Assert(t, db.UpdateLease(lease2InDB) == nil, "update lease should ok")
	lease2InDB = db.GetLeaseWithMac(mac3)
	ut.Equal(t, lease2InDB.Address, addr2)

	db.DeleteLease(1, addr1)
	lease1InDB = db.GetLeaseWithMac(mac1)
	ut.Equal(t, lease1InDB, (*Lease)(nil))

	db.(*LeaseDB).Clean()
	db.(*LeaseDB).Destroy()
}

func TestDeleteLeaseNotInSubnet(t *testing.T) {
	util.InitLog("", "debug")
	db_, err := NewLeaseDB("127.0.0.1", "joe", "zdns", "zdns")
	ut.Assert(t, err == nil, "err should be nil but %v", err)

	db := db_.(*LeaseDB)
	for i := 0; i < 100; i++ {
		db.AddLease(createLease(strToIP(fmt.Sprintf("0.0.0.%d", i)),
			strToMac(fmt.Sprintf("00:00:00:00:00:%d", i))))
	}

	leases := db.getLeasesWithAttrs(map[string]interface{}{"subnet_id": 1})
	ut.Equal(t, len(leases), 100)

	subnet := &Subnet{
		Id:    1,
		Pools: []*Pool{&Pool{Start: 1, End: 10}, &Pool{Start: 30, End: 39}},
	}
	db.deleteLeaseNotInSubnet(subnet)

	leases = db.getLeasesWithAttrs(map[string]interface{}{"subnet_id": 1})
	ut.Equal(t, len(leases), 20)

	db.Clean()
	db.Destroy()
}

func TestSubnetAllocateConnection(t *testing.T) {
	util.InitLog("", "debug")
	dbpool, err := newLeaseDBPool(4, "127.0.0.1", "joe", "zdns", "zdns")
	ut.Assert(t, err == nil, "err should be nil but %v", err)

	subnet1 := &Subnet{
		Id:    1,
		Pools: []*Pool{&Pool{Start: 1, End: 10}},
	}
	subnet2 := &Subnet{
		Id:    2,
		Pools: []*Pool{&Pool{Start: 20, End: 29}},
	}
	subnet3 := &Subnet{
		Id:    3,
		Pools: []*Pool{&Pool{Start: 30, End: 39}},
	}
	subnets := []*Subnet{subnet1, subnet2, subnet3}
	subnetToIdexs := dbpool.SubnetAllocateConnection(subnets)
	ut.Equal(t, len(subnetToIdexs), 3)
	ut.Equal(t, subnetToIdexs[1], 0)
	ut.Equal(t, subnetToIdexs[2], 1)
	ut.Equal(t, subnetToIdexs[3], 2)

	subnet4 := &Subnet{
		Id:    4,
		Pools: []*Pool{&Pool{Start: 40, End: 49}},
	}
	subnet5 := &Subnet{
		Id:    5,
		Pools: []*Pool{&Pool{Start: 50, End: 59}},
	}
	subnet6 := &Subnet{
		Id:    6,
		Pools: []*Pool{&Pool{Start: 60, End: 69}},
	}
	subnets2 := []*Subnet{subnet4, subnet2, subnet3, subnet6, subnet5}
	subnetToIdexs2 := dbpool.SubnetAllocateConnection(subnets2)
	ut.Equal(t, len(subnetToIdexs2), 5)
	ut.Equal(t, subnetToIdexs2[4], 0)
	ut.Equal(t, subnetToIdexs2[2], 1)
	ut.Equal(t, subnetToIdexs2[3], 2)
	ut.Equal(t, subnetToIdexs2[6], 3)
	ut.Equal(t, subnetToIdexs2[5], 0)

	subnet7 := &Subnet{
		Id:    7,
		Pools: []*Pool{&Pool{Start: 70, End: 79}},
	}
	subnets3 := []*Subnet{subnet4, subnet2, subnet3, subnet6, subnet7}
	subnetToIdexs3 := dbpool.SubnetAllocateConnection(subnets3)
	ut.Equal(t, len(subnetToIdexs3), 5)
	ut.Equal(t, subnetToIdexs3[4], 0)
	ut.Equal(t, subnetToIdexs3[2], 1)
	ut.Equal(t, subnetToIdexs3[3], 2)
	ut.Equal(t, subnetToIdexs3[6], 3)
	ut.Equal(t, subnetToIdexs3[7], 0)

	subnet8 := &Subnet{
		Id:    8,
		Pools: []*Pool{&Pool{Start: 80, End: 89}},
	}
	subnets4 := []*Subnet{subnet8}
	subnetToIdexs4 := dbpool.SubnetAllocateConnection(subnets4)
	ut.Equal(t, len(subnetToIdexs4), 1)
	ut.Equal(t, subnetToIdexs4[8], 0)
}
