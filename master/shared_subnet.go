package kea

import (
	"cement/jconf"
)

type SharedSubnet struct {
	name      string
	subnetIDs []SubnetID
}

func (ss *SharedSubnet) getSharedSubnets(subnetID SubnetID) []SubnetID {
	for i, id := range ss.subnetIDs {
		if id == subnetID {
			otherSubnets := make([]SubnetID, len(ss.subnetIDs))
			copy(otherSubnets, ss.subnetIDs)
			return append(otherSubnets[:i], otherSubnets[i+1:]...)
		}
	}
	return nil
}

type SharedSubnetMgr struct {
	sharedSubnets []SharedSubnet
}

func newSharedSubnetMgr() *SharedSubnetMgr {
	return &SharedSubnetMgr{}
}

func (mgr *SharedSubnetMgr) addSharedSubnet(name string, subnetIDs []SubnetID) {
	mgr.sharedSubnets = append(mgr.sharedSubnets, SharedSubnet{
		name:      name,
		subnetIDs: subnetIDs,
	})
}

func (mgr *SharedSubnetMgr) reloadConf(conf *jconf.Config) {
	mgr.sharedSubnets = []SharedSubnet{}
	if conf.HasKey("dhcp4.shared-network") == false {
		return
	}

	sharedSubnetsConf := conf.GetObjects("dhcp4.shared-network")
	for _, sharedSubnetConf := range sharedSubnetsConf {
		subnetIDs_ := sharedSubnetConf.GetUints("subnet-ids")
		subnetIDs := make([]SubnetID, len(subnetIDs_), len(subnetIDs_))
		for i, subnetID_ := range subnetIDs_ {
			subnetIDs[i] = SubnetID(subnetID_)
		}
		mgr.addSharedSubnet(sharedSubnetConf.GetString("name"), subnetIDs)
	}
}

func (mgr *SharedSubnetMgr) GetSubnetsSharedWith(subnetID SubnetID) []SubnetID {
	for _, sharedSubnet := range mgr.sharedSubnets {
		if subnets := sharedSubnet.getSharedSubnets(subnetID); subnets != nil {
			return subnets
		}
	}
	return []SubnetID{}
}
