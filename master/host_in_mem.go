package kea

import (
	"fmt"
	"net"
)

type HostStore struct {
	hosts map[string]*Host
}

func newHostStore() HostManager {
	return &HostStore{make(map[string]*Host)}
}

func (store *HostStore) GetHostWithMac(mac net.HardwareAddr) *Host {
	key := mac.String()
	host, _ := store.hosts[key]
	return host
}

func (store *HostStore) GetHostWithIp(addr net.IP) *Host {
	for _, host := range store.hosts {
		if host.ReserveAddr.Equal(addr) {
			return host
		}
	}
	return nil
}

func (store *HostStore) AddHost(host *Host) error {
	key := host.Mac.String()
	var err error
	if _, ok := store.hosts[key]; ok == false {
		store.hosts[key] = host
	} else {
		err = fmt.Errorf("host with mac add %s already exists", host.Mac.String())
	}
	return err
}
