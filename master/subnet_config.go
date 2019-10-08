package kea

import (
	"fmt"
	"strings"
	"time"

	"cement/jconf"
	"kea/util"
)

func poolFromString(ipRange string) (*Pool, error) {
	ips := strings.Split(ipRange, "-")
	if len(ips) != 2 {
		return nil, fmt.Errorf("pool ip range %s isn't valid", ipRange)
	}

	return parsePool(ips[0], ips[1])
}

func LoadSubnet(conf *jconf.Config) ([]*Subnet, error) {
	subnetsConf := conf.GetObjects("dhcp4.subnet4")
	subnets := []*Subnet{}
	initHostManagers()
	for _, subnetConf := range subnetsConf {
		subnetID := SubnetID(subnetConf.GetUint("id"))
		pools := []*Pool{}
		reservedPools := []*Pool{}
		for _, poolConf := range subnetConf.GetObjects("pools") {
			ipRange := poolConf.GetString("pool")
			pool, err := poolFromString(ipRange)
			if err != nil {
				return nil, err
			}

			if poolConf.HasKey("reservated-addr") && poolConf.GetBool("reservated-addr") {
				reservedPools = append(reservedPools, pool)
			} else {
				pools = append(pools, pool)
			}
		}

		validLifeTime := time.Duration(4800) * time.Second
		if subnetConf.HasKey("default-valid-lifetime") {
			validLifeTime = time.Duration(subnetConf.GetInt("default-valid-lifetime")) * time.Second
		}

		if subnetConf.HasKey("reservations") {
			hostManager := newHostStore()
			for _, hostConf := range subnetConf.GetObjects("reservations") {
				host, err := parseHost(hostConf.GetString("hw-address"), hostConf.GetString("ip-address"))
				if err != nil {
					return nil, err
				}

				hostManager.AddHost(host)
			}
			HostManagerInstance().addHostManager(subnetID, hostManager)
		}

		subnet := &Subnet{
			Id:            subnetID,
			RenewTime:     time.Duration(4000) * time.Second,
			RebindTime:    time.Duration(4000) * time.Second,
			ValidLifeTime: validLifeTime,
			Pools:         pools,
			ReservedPools: reservedPools,
		}
		util.Logger().Debug("load subnet %d with capacity %d", subnet.Id, subnet.Capacity())
		subnets = append(subnets, subnet)
	}

	return subnets, nil
}
