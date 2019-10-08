package kea

import (
	"net"
	"time"

	"kea/util"
)

type LeaseState int
type ClientID []byte

const (
	Normal           LeaseState = 0
	Declined                    = 1
	ExpiredReclaimed            = 2
)

type Lease struct {
	Address             net.IP
	State               LeaseState
	ValidLifeTime       time.Duration
	ClientId            ClientID
	SubnetId            SubnetID
	ClientLastTransTime time.Time
	HostName            string
	Mac                 net.HardwareAddr

	RenewTime  time.Duration //in secs
	RebindTime time.Duration //in secs
}

func (lease *Lease) IsExpired() bool {
	return lease.GetExpiredTime().Before(time.Now())
}

func (lease *Lease) GetExpiredTime() time.Time {
	return lease.ClientLastTransTime.Add(lease.ValidLifeTime)
}

func (lease *Lease) Decline() *Lease {
	declined := *lease
	declined.State = Declined
	declined.ClientLastTransTime = time.Now()
	declined.ClientId = nil
	declined.Mac = nil
	return &declined
}

type LeaseManager interface {
	GetLeaseWithIp(net.IP) *Lease
	GetLeaseWithMac(net.HardwareAddr) *Lease
	GetLeaseWithClient(ClientID) *Lease
	GetLeaseWithClientAndMac(ClientID, net.HardwareAddr) *Lease
	AddLease(*Lease) error
	UpdateLease(*Lease) error
	DeleteLease(SubnetID, net.IP)
	GetLeaseCount(SubnetID) int
	DeleteSubnet(SubnetID)
	DeletePool(SubnetID, *Pool)
}

func ToLeaseResult(lease *Lease) LeaseResult {
	if lease != nil {
		return LeaseResult{
			Succeed:  true,
			Addr:     util.IPv4ToLong(lease.Address),
			SubnetID: uint32(lease.SubnetId),
		}
	} else {
		return LeaseResult{
			Succeed:  false,
			SubnetID: uint32(1), //avoid empty lease result after marshal
		}
	}
}
