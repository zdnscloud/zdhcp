package util

import (
	"net"
	"testing"

	ut "cement/unittest"
)

func TestAddrUtil(t *testing.T) {
	ip := IPv4FromString("1.1.1.1")
	ut.Assert(t, ip != nil, "1.1.1.1 is valid ip")

	ip = IPv4FromString("1222.1.1.1")
	if ip != nil {
		ut.Assert(t, false, "from string should compared with nil")
	}

	ut.Equal(t, IPv4FromString("1222.1.1.1"), net.IP(nil))
	ut.Equal(t, IPv4FromString("1222.1xx.1.1"), net.IP(nil))
	ut.Equal(t, IPv4FromString("1222.1.1.1.1"), net.IP(nil))

	ut.Equal(t, IPv4FromString(" 2.2.2.2 ").String(), "2.2.2.2")

	ip = IPv4FromString("0.0.0.1")
	ut.Equal(t, IPv4ToLongHBO(ip), uint32(1))

	AddIPv4(ip, 1)
	ut.Equal(t, IPv4ToLongHBO(ip), uint32(2))

	ip1 := IPv4FromString("1.1.1.1")
	ip2 := IPv4FromString("2.2.2.2")
	ut.Equal(t, CompareIPv4(ip1, ip2), -1)

}
