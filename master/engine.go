package kea

import (
	"bytes"
	"errors"
	"net"
	"time"

	"kea/util"
)

var ErrNoAddressLeft = errors.New("all the ip address has been allocated")
var ErrWantUsedAddress = errors.New("request addr is used by others")
var ErrWantReservedAddress = errors.New("request addr is reserved")
var ErrRequestWithoutTarget = errors.New("request without wanted addr")
var ErrRelaseOthersAddr = errors.New("release lease doesn't belongs to current user")
var ErrRelaseNotExistAddr = errors.New("release lease doesn't exist")
var ErrDeclineOhtersAddr = errors.New("decline lease doesn't belongs to current user")
var ErrNotWantReservedAddr = errors.New("request addr is not same with reserved")
var ErrWantAddressInOtherSubnet = errors.New("request addr is not belongs to this subnet")
var ErrDeclineConflictAddr = errors.New("decline conflict ip has beed allocated")
var ErrDeclineNotExistAddr = errors.New("decline lease doesn't exist")

type SubnetEngine struct {
	leaseManager LeaseManager
	hostManager  HostManager
	subnet       *Subnet
	allocator    Allocator
}

func newEngine(subnet *Subnet, leaseManager LeaseManager) Engine {
	return &SubnetEngine{
		leaseManager: newHybridLeaseManager(leaseManager, LeaseDBPoolInstance()),
		hostManager:  HostManagerInstance().managers[subnet.Id],
		subnet:       subnet,
		allocator:    newIterativeAllocator(subnet),
	}
}

func (e *SubnetEngine) Subnet() *Subnet {
	return e.subnet
}

func (e *SubnetEngine) PlanLease(ctx *Context) (*Lease, error) {
	oldLease := e.findOldLease(ctx)
	host := e.getHostWithMac(ctx.Mac)

	if host != nil {
		if oldLease != nil && oldLease.Address.Equal(host.ReserveAddr) {
			return e.renewLease(oldLease, ctx), nil
		}

		newLease, err := e.allocateLeaseWithAddr(host.ReserveAddr, ctx)
		if err == nil {
			return newLease, nil
		}
	}

	if oldLease != nil {
		if newLease := e.renewLease(oldLease, ctx); newLease != nil {
			return newLease, nil
		}
	}

	//user want last assigned address
	if ctx.RequestAddr != nil && e.subnet.IncludeAddr(ctx.RequestAddr) {
		if e.addressIsReserved(ctx.RequestAddr, ctx.Mac) == false {
			if newLease, err := e.allocateLeaseWithAddr(ctx.RequestAddr, ctx); err == nil {
				return newLease, nil
			}
		}
	}

	return e.allocateUnreservedLease(ctx)
}

func (e *SubnetEngine) findOldLease(ctx *Context) *Lease {
	if len(ctx.ClientID) != 0 {
		if lease := e.leaseManager.GetLeaseWithClient(ctx.ClientID); lease != nil {
			return lease
		}
	}
	if ctx.Mac != nil {
		return e.leaseManager.GetLeaseWithMac(ctx.Mac)
	} else {
		return nil
	}
}

func (e *SubnetEngine) AllocateLease(ctx *Context) (*Lease, error) {
	if ctx.RequestAddr == nil {
		return nil, ErrRequestWithoutTarget
	}

	if e.addressIsReserved(ctx.RequestAddr, ctx.Mac) {
		return nil, ErrWantReservedAddress
	}

	host := e.getHostWithMac(ctx.Mac)
	if host != nil {
		if host.ReserveAddr.Equal(ctx.RequestAddr) == false {
			oldLease := e.leaseManager.GetLeaseWithIp(host.ReserveAddr)
			if oldLease == nil || oldLease.IsExpired() || bytes.Equal(oldLease.Mac, ctx.Mac) {
				return nil, ErrNotWantReservedAddr
			}
		}
	}

	oldLease := e.findOldLease(ctx)
	if oldLease != nil {
		if oldLease.Address.Equal(ctx.RequestAddr) {
			return e.renewLease(oldLease, ctx), nil
		}
	}

	if host == nil && e.subnet.IncludeAddr(ctx.RequestAddr) == false {
		return nil, ErrWantAddressInOtherSubnet
	}

	newLease, err := e.allocateLeaseWithAddr(ctx.RequestAddr, ctx)
	if oldLease != nil && newLease != nil && oldLease.Address.Equal(newLease.Address) == false {
		e.leaseManager.DeleteLease(oldLease.SubnetId, oldLease.Address)
	}
	return newLease, err
}

func (e *SubnetEngine) renewLease(lease *Lease, ctx *Context) *Lease {
	newLease := *lease
	newLease.SubnetId = e.subnet.Id
	newLease.Mac = ctx.Mac
	newLease.State = Normal
	newLease.ClientId = ctx.ClientID
	newLease.ClientLastTransTime = time.Now()
	newLease.RenewTime = e.subnet.RenewTime
	newLease.RebindTime = e.subnet.RebindTime
	newLease.ValidLifeTime = e.subnet.ValidLifeTime
	newLease.HostName = ctx.HostName

	if ctx.RequestType == ContextMsg_Request {
		e.leaseManager.UpdateLease(&newLease)
	}

	return &newLease
}

func (e *SubnetEngine) allocateLeaseWithAddr(ip net.IP, ctx *Context) (*Lease, error) {
	oldLease := e.leaseManager.GetLeaseWithIp(ip)
	if oldLease == nil {
		newLease := &Lease{
			Address:             ip,
			State:               Normal,
			ValidLifeTime:       e.subnet.ValidLifeTime,
			ClientId:            ctx.ClientID,
			SubnetId:            ctx.SubnetID,
			ClientLastTransTime: time.Now(),
			HostName:            ctx.HostName,
			Mac:                 ctx.Mac,
			RenewTime:           e.subnet.RenewTime,
			RebindTime:          e.subnet.RebindTime,
		}

		if ctx.RequestType == ContextMsg_Request {
			e.leaseManager.AddLease(newLease)
		}

		return newLease, nil
	}

	if oldLease.IsExpired() == false {
		return nil, ErrWantUsedAddress
	} else {
		return e.renewLease(oldLease, ctx), nil
	}
}

func (e *SubnetEngine) addressIsReserved(ip net.IP, mac net.HardwareAddr) bool {
	if e.hostManager != nil {
		host := e.hostManager.GetHostWithIp(ip)
		if host != nil {
			return bytes.Equal(host.Mac, mac) == false
		}
	}

	return e.subnet.IsAddrReserved(ip)
}

func (e *SubnetEngine) allocateUnreservedLease(ctx *Context) (*Lease, error) {
	maxAddrCount := e.subnet.Capacity()
	for i := uint32(0); i < maxAddrCount; i++ {
		addr := e.allocator.PickAddr()
		if e.addressIsReserved(addr, ctx.Mac) == false {
			if newLease, err := e.allocateLeaseWithAddr(addr, ctx); err == nil {
				return newLease, nil
			}
		}
	}
	return nil, ErrNoAddressLeft
}

func (e *SubnetEngine) getHostWithMac(mac net.HardwareAddr) *Host {
	if e.hostManager != nil {
		return e.hostManager.GetHostWithMac(mac)
	} else {
		return nil
	}
}

func (e *SubnetEngine) ReleaseLease(ctx *Context) error {
	oldLease := e.findOldLease(ctx)
	if oldLease == nil {
		return ErrRelaseNotExistAddr
	} else if oldLease.Address.Equal(ctx.RequestAddr) == false {
		return ErrRelaseOthersAddr
	} else {
		e.leaseManager.DeleteLease(oldLease.SubnetId, oldLease.Address)
		return nil
	}
}

func (e *SubnetEngine) DeclineLease(ctx *Context) error {
	oldLease := e.findOldLease(ctx)
	if oldLease == nil {
		return ErrDeclineNotExistAddr
	} else if oldLease.Address.Equal(ctx.RequestAddr) == false {
		return ErrDeclineOhtersAddr
	} else {
		e.leaseManager.UpdateLease(oldLease.Decline())
		return nil
	}
}

func (e *SubnetEngine) DeclineConflictIP(ctx *Context) error {
	oldLease := e.leaseManager.GetLeaseWithIp(ctx.RequestAddr)
	if oldLease == nil {
		err := e.leaseManager.AddLease(
			&Lease{
				Address:             ctx.RequestAddr,
				State:               Declined,
				ValidLifeTime:       e.subnet.ValidLifeTime,
				SubnetId:            ctx.SubnetID,
				ClientLastTransTime: time.Now(),
				HostName:            ctx.HostName,
				RenewTime:           e.subnet.RenewTime,
				RebindTime:          e.subnet.RebindTime,
			})

		if err != nil {
			util.Logger().Error("decline conflict ip %v with error: %v", ctx.RequestAddr, err.Error())
			return ErrDeclineConflictAddr
		} else {
			util.Logger().Debug("decline conflict ip %v", ctx.RequestAddr)
			return nil
		}
	} else {
		util.Logger().Debug("decline conflict ip %v for illegal lease", ctx.RequestAddr)
		e.leaseManager.UpdateLease(oldLease.Decline())
		return nil
	}
}
