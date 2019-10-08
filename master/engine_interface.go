package kea

import (
	"kea/util"
	"net"
)

type Allocator interface {
	PickAddr() net.IP
}

type Context struct {
	RequestType ContextMsg_RequestType
	SubnetID    SubnetID
	ClientID    ClientID
	Mac         net.HardwareAddr
	RequestAddr net.IP
	HostName    string
}

func FromContextMsg(msg *ContextMsg) *Context {
	return &Context{
		RequestType: msg.RequestType,
		SubnetID:    SubnetID(msg.SubnetID),
		ClientID:    ClientID(msg.ClientID),
		Mac:         net.HardwareAddr(msg.Mac),
		RequestAddr: util.IPv4FromLong(msg.RequestAddr),
		HostName:    msg.HostName,
	}
}

type Engine interface {
	Subnet() *Subnet
	LeaseManager() LeaseManager
	SetHostManager(hostManager HostManager)
	PlanLease(ctx *Context) (*Lease, error)
	AllocateLease(ctx *Context) (*Lease, error)
	ReleaseLease(ctx *Context) error
	DeclineLease(ctx *Context) error
	DeclineConflictIP(ctx *Context) error
}

type Configurable interface {
	ReloadConf(subnet *Subnet)
}
