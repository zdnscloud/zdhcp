package kea

import (
	"kea/util"
	"net"
	"testing"

	ut "cement/unittest"
)

func TestIteratorAllocator(t *testing.T) {
	pool1, _ := poolFromString("192.0.0.1 - 192.0.0.5")
	pool2, _ := poolFromString("192.127.0.1 - 192.127.0.5")
	subnet := &Subnet{
		Pools: []*Pool{pool1, pool2},
	}

	allocator := newIterativeAllocator(subnet)
	var addr net.IP
	for round := 0; round < 1000; round++ {
		addr = util.IPv4FromString("192.0.0.1")
		for i := 0; i < 5; i++ {
			ut.Equal(t, allocator.PickAddr(), addr)
			util.AddIPv4(addr, 1)
		}

		addr = util.IPv4FromString("192.127.0.1")
		for i := 0; i < 5; i++ {
			ut.Equal(t, allocator.PickAddr(), addr)
			util.AddIPv4(addr, 1)
		}
	}
}
