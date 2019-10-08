package kea

import (
	"sync"

	"cement/jconf"
	"kea/util"
)

type AddrAllocator struct {
	engines         map[SubnetID]Engine
	engineLocks     map[SubnetID]*sync.Mutex
	sharedSubnetMgr *SharedSubnetMgr
}

func CreateAddrAllocator(conf *jconf.Config) (*AddrAllocator, error) {
	err := initLeaseDBPool(conf)
	if err != nil {
		return nil, err
	}

	allocator := &AddrAllocator{
		engines:         make(map[SubnetID]Engine),
		engineLocks:     make(map[SubnetID]*sync.Mutex),
		sharedSubnetMgr: newSharedSubnetMgr(),
	}

	if err := allocator.reloadConf(conf); err != nil {
		return nil, err
	} else {
		return allocator, nil
	}
}

func (allocator *AddrAllocator) reloadConf(conf *jconf.Config) error {
	subnets, err := LoadSubnet(conf)
	if err != nil {
		return err
	}

	subnetAndLeaseManager := make(map[*Subnet]LeaseManager)
	engines := make(map[SubnetID]Engine)
	engineLocks := make(map[SubnetID]*sync.Mutex)
	for _, subnet := range subnets {
		subnetAndLeaseManager[subnet] = newLeaseStore()
	}
	LeaseDBPoolInstance().LoadLease(subnetAndLeaseManager)

	for _, subnet := range subnets {
		engines[subnet.Id] = newEngine(subnet, subnetAndLeaseManager[subnet])
		engineLocks[subnet.Id] = &sync.Mutex{}
	}

	allocator.engines = engines
	allocator.engineLocks = engineLocks
	allocator.sharedSubnetMgr.reloadConf(conf)
	return nil
}

func (allocator *AddrAllocator) HandleRequest(ctx *Context) LeaseResult {
	if _, ok := allocator.engines[ctx.SubnetID]; ok == false {
		util.Logger().Warn("unknown subnet with id %v", ctx.SubnetID)
		return ToLeaseResult(nil)
	}

	switch ctx.RequestType {
	case ContextMsg_Discover:
		return allocator.planLease(ctx)
	case ContextMsg_Request:
		return allocator.allocateLease(ctx)
	case ContextMsg_Release:
		return allocator.releaseLease(ctx)
	case ContextMsg_Decline:
		return allocator.declineLease(ctx)
	case ContextMsg_ConflictIP:
		return allocator.declineConflictIP(ctx)
	default:
		panic("Context request type is invalid")
	}
}

func (allocator *AddrAllocator) planLease(ctx *Context) LeaseResult {
	if lease, err := allocator.planLeaseInSubnet(ctx); err == nil {
		util.Logger().Debug("discover addr %s for client with mac %v in subnet %v", lease.Address.String(), lease.Mac, lease.SubnetId)
		return ToLeaseResult(lease)
	} else {
		util.Logger().Warn("discover lease in subnet %v failed:%s", ctx.SubnetID, err.Error())
	}

	sharedSubnets := allocator.sharedSubnetMgr.GetSubnetsSharedWith(ctx.SubnetID)
	for _, subnetID := range sharedSubnets {
		ctx.SubnetID = subnetID
		if lease, _ := allocator.planLeaseInSubnet(ctx); lease != nil {
			util.Logger().Debug("discover addr %s for client with mac %v in shared subnet %v", lease.Address.String(), lease.Mac, lease.SubnetId)
			return ToLeaseResult(lease)
		}
	}

	util.Logger().Debug("no addr to plan for client with mac %v in subnet %v", ctx.Mac, ctx.SubnetID)
	return ToLeaseResult(nil)
}

func (allocator *AddrAllocator) planLeaseInSubnet(ctx *Context) (*Lease, error) {
	engineLock := allocator.engineLocks[ctx.SubnetID]
	engineLock.Lock()
	defer engineLock.Unlock()
	return allocator.engines[ctx.SubnetID].PlanLease(ctx)
}

func (allocator *AddrAllocator) allocateLease(ctx *Context) LeaseResult {
	if lease, err := allocator.allocateLeaseInSubnet(ctx); err == nil {
		util.Logger().Debug("allocate addr %s for client with mac %v in subnet %v", lease.Address.String(), lease.Mac, lease.SubnetId)
		return ToLeaseResult(lease)
	} else {
		util.Logger().Warn("request lease in subnet %v failed:%s", ctx.SubnetID, err.Error())
	}

	sharedSubnets := allocator.sharedSubnetMgr.GetSubnetsSharedWith(ctx.SubnetID)
	for _, subnetID := range sharedSubnets {
		if allocator.engines[subnetID].Subnet().IncludeAddr(ctx.RequestAddr) {
			ctx.SubnetID = subnetID
			if lease, err := allocator.allocateLeaseInSubnet(ctx); err == nil {
				util.Logger().Debug("allocate addr %s for client with mac %v in shared subnet %v", lease.Address.String(), lease.Mac, lease.SubnetId)
				return ToLeaseResult(lease)
			} else {
				util.Logger().Warn("request lease in shared network %v failed:%s", ctx.SubnetID, err.Error())
			}
		}
	}

	util.Logger().Debug("no addr to allocate for client with mac %v in subnet %v", ctx.Mac, ctx.SubnetID)
	return ToLeaseResult(nil)
}

func (allocator *AddrAllocator) allocateLeaseInSubnet(ctx *Context) (*Lease, error) {
	engineLock := allocator.engineLocks[ctx.SubnetID]
	engineLock.Lock()
	defer engineLock.Unlock()
	return allocator.engines[ctx.SubnetID].AllocateLease(ctx)
}

func (allocator *AddrAllocator) releaseLease(ctx *Context) LeaseResult {
	if err := allocator.releaseLeaseInSubnet(ctx); err == nil {
		util.Logger().Debug("release addr %v for subnet %v successed", ctx.RequestAddr, ctx.SubnetID)
		return LeaseResult{Succeed: true}
	} else {
		util.Logger().Warn("release addr %v[%v] for subnet %v failed:%s", ctx.RequestAddr, ctx.Mac, ctx.SubnetID, err.Error())
	}

	sharedSubnets := allocator.sharedSubnetMgr.GetSubnetsSharedWith(ctx.SubnetID)
	for _, subnetID := range sharedSubnets {
		if allocator.engines[subnetID].Subnet().IncludeAddr(ctx.RequestAddr) {
			ctx.SubnetID = subnetID
			if err := allocator.releaseLeaseInSubnet(ctx); err == nil {
				util.Logger().Debug("release lease in shared subnet %v successed", ctx.SubnetID)
				return LeaseResult{Succeed: true}
			}
		}
	}

	return ToLeaseResult(nil)
}

func (allocator *AddrAllocator) releaseLeaseInSubnet(ctx *Context) error {
	engineLock := allocator.engineLocks[ctx.SubnetID]
	engineLock.Lock()
	defer engineLock.Unlock()
	return allocator.engines[ctx.SubnetID].ReleaseLease(ctx)
}

func (allocator *AddrAllocator) declineLease(ctx *Context) LeaseResult {
	if err := allocator.declineLeaseInSubnet(ctx); err == nil {
		util.Logger().Debug("decline lease in subnet %v successed", ctx.SubnetID)
		return LeaseResult{Succeed: true}
	} else {
		util.Logger().Warn("decline lease in subnet %v failed:%s", ctx.SubnetID, err.Error())
	}

	sharedSubnets := allocator.sharedSubnetMgr.GetSubnetsSharedWith(ctx.SubnetID)
	for _, subnetID := range sharedSubnets {
		if allocator.engines[subnetID].Subnet().IncludeAddr(ctx.RequestAddr) {
			ctx.SubnetID = subnetID
			if err := allocator.declineLeaseInSubnet(ctx); err == nil {
				util.Logger().Debug("decline lease in shared subnet %v successed", ctx.SubnetID)
				return LeaseResult{Succeed: true}
			}
		}
	}

	return ToLeaseResult(nil)
}

func (allocator *AddrAllocator) declineLeaseInSubnet(ctx *Context) error {
	engineLock := allocator.engineLocks[ctx.SubnetID]
	engineLock.Lock()
	defer engineLock.Unlock()
	return allocator.engines[ctx.SubnetID].DeclineLease(ctx)
}

func (allocator *AddrAllocator) declineConflictIP(ctx *Context) LeaseResult {
	engineLock := allocator.engineLocks[ctx.SubnetID]
	engineLock.Lock()
	defer engineLock.Unlock()
	if err := allocator.engines[ctx.SubnetID].DeclineConflictIP(ctx); err != nil {
		return ToLeaseResult(nil)
	}
	return LeaseResult{Succeed: true}
}

func (allocator *AddrAllocator) Stop() {
	LeaseDBPoolInstance().Stop()
}
