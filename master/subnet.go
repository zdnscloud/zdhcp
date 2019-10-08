package kea

import (
	"fmt"
	"net"
	"time"

	"kea/util"
)

type SubnetID uint32

type ClientClass string

type Pool struct {
	Start uint32
	End   uint32
}

func newPool(start, end net.IP) (*Pool, error) {
	startVal := util.IPv4ToLongHBO(start)
	endVal := util.IPv4ToLongHBO(end)
	if startVal > endVal {
		return nil, fmt.Errorf("start %s is bigger than end %s", start.String(), end.String())
	}
	return &Pool{
		Start: startVal,
		End:   endVal,
	}, nil
}

func (pool *Pool) IncludeAddr(ip net.IP) bool {
	return pool.IncludeVal(util.IPv4ToLongHBO(ip))
}

func (pool *Pool) IncludeVal(ipVal uint32) bool {
	return ipVal >= pool.Start && ipVal <= pool.End
}

func (pool *Pool) String() string {
	return util.IPv4FromLongHBO(pool.Start).String() + " - " + util.IPv4FromLongHBO(pool.End).String()
}

func (pool *Pool) Capacity() uint32 {
	return pool.End - pool.Start + 1
}

type Subnet struct {
	Id            SubnetID
	RenewTime     time.Duration
	RebindTime    time.Duration
	ValidLifeTime time.Duration

	Pools         []*Pool
	ReservedPools []*Pool
}

func (subnet *Subnet) IncludeAddr(ip net.IP) bool {
	return getPoolIncludeAddr(subnet.Pools, ip) != nil
}

func (subnet *Subnet) IsAddrReserved(ip net.IP) bool {
	return getPoolIncludeAddr(subnet.ReservedPools, ip) != nil
}

func getPoolIncludeAddr(pools []*Pool, ip net.IP) *Pool {
	for _, pool := range pools {
		if pool.IncludeAddr(ip) {
			return pool
		}
	}
	return nil
}

func (subnet *Subnet) Capacity() uint32 {
	capacity := uint32(0)
	for _, pool := range subnet.Pools {
		capacity += pool.Capacity()
	}

	return capacity
}

type SubnetManager interface {
	GetSubnet(SubnetID) *Subnet
}
