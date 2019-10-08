package kea

import (
	"testing"

	ut "cement/unittest"
)

func TestSharedSubnet(t *testing.T) {
	sharedSubnetMgr := newSharedSubnetMgr()
	sharedSubnetMgr.addSharedSubnet("s1", []SubnetID{SubnetID(3171095), SubnetID(3171096)})

	for i := 0; i < 10; i++ {
		ut.Equal(t, sharedSubnetMgr.GetSubnetsSharedWith(SubnetID(3171095)), []SubnetID{SubnetID(3171096)})
		ut.Equal(t, sharedSubnetMgr.GetSubnetsSharedWith(SubnetID(3171096)), []SubnetID{SubnetID(3171095)})
		ut.Equal(t, sharedSubnetMgr.GetSubnetsSharedWith(SubnetID(3171097)), []SubnetID{})
	}
}
