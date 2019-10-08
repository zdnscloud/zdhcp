package kea

import (
	"container/list"
	"net"

	"kea/util"
)

type SmartAllocator struct {
	addrList *list.List
}

func newSmartAllocator(subnet *Subnet) Allocator {
	addr_list := list.New()
	for _, pool := range subnet.Pools {
		ipInPool := pool.Start
		for ; ipInPool <= pool.End; ipInPool++ {
			addr_list.PushBack(ipInPool)
		}
	}
	return &SmartAllocator{addrList: addr_list}
}

func (allocator *SmartAllocator) PickAddr() net.IP {
	if allocator.addrList.Len() == 0 {
		return nil
	} else {
		pickAddr := allocator.addrList.Front()
		allocator.addrList.Remove(pickAddr)
		return util.IPv4FromLongHBO(pickAddr.Value.(uint32))
	}
}
