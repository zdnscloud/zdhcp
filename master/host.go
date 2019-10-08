package kea

import (
	"net"
)

type Host struct {
	ReserveAddr   net.IP
	Name          string
	ClientClasses []ClientClass
	Mac           net.HardwareAddr
}

var _hostManagerInstance *HostManagers

type HostManagers struct {
	managers map[SubnetID]HostManager
}

type HostManager interface {
	GetHostWithMac(net.HardwareAddr) *Host
	GetHostWithIp(net.IP) *Host
	AddHost(*Host) error
	DeleteHost(*Host) error
}

func initHostManagers() {
	_hostManagerInstance = &HostManagers{make(map[SubnetID]HostManager)}
}

func HostManagerInstance() *HostManagers {
	return _hostManagerInstance
}

func (hostManagers *HostManagers) addHostManager(subnetId SubnetID, hostManager HostManager) {
	hostManagers.managers[subnetId] = hostManager
}
