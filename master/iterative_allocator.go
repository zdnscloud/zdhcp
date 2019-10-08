package kea

import (
	"net"

	"kea/util"
)

type IterativeAllocator struct {
	subnet           *Subnet
	lastAllocateAddr uint32
}

func newIterativeAllocator(subnet *Subnet) Allocator {
	return &IterativeAllocator{
		subnet:           subnet,
		lastAllocateAddr: 0,
	}
}

func (allocator *IterativeAllocator) PickAddr() net.IP {
	subnet := allocator.subnet
	poolCount := len(subnet.Pools)

	if allocator.lastAllocateAddr == 0 {
		allocator.lastAllocateAddr = subnet.Pools[0].Start
		return util.IPv4FromLongHBO(subnet.Pools[0].Start)
	}

	lastAllocateAddr := allocator.lastAllocateAddr
	lastPoolIndex := 0
	var lastPool *Pool
	for ; lastPoolIndex < poolCount; lastPoolIndex++ {
		if subnet.Pools[lastPoolIndex].IncludeVal(lastAllocateAddr) {
			lastPool = subnet.Pools[lastPoolIndex]
			break
		}
	}

	if lastPool == nil {
		allocator.lastAllocateAddr = subnet.Pools[0].Start
		return util.IPv4FromLongHBO(lastPool.Start)
	}

	nextAddr := lastAllocateAddr + 1
	if lastPool.IncludeVal(nextAddr) {
		allocator.lastAllocateAddr = nextAddr
		return util.IPv4FromLongHBO(nextAddr)
	}

	nextPool := subnet.Pools[(lastPoolIndex+1)%poolCount]
	allocator.lastAllocateAddr = nextPool.Start
	return util.IPv4FromLongHBO(allocator.lastAllocateAddr)
}
