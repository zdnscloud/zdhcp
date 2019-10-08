package util

import (
	"encoding/binary"
	"net"
	"strings"
)

func IPv4FromString(ipstr string) net.IP {
	return net.ParseIP(strings.TrimSpace(ipstr)).To4()
}

func IPv4FromLong(val uint32) net.IP {
	addr := make([]byte, 4)
	binary.LittleEndian.PutUint32(addr, val)
	return net.IP(addr)
}

func IPv4FromLongHBO(val uint32) net.IP {
	addr := make([]byte, 4)
	binary.BigEndian.PutUint32(addr, val)
	return net.IP(addr)
}

func IPv4ToLong(ip net.IP) uint32 {
	return binary.LittleEndian.Uint32(ip)
}

func IPv4ToLongHBO(ip net.IP) uint32 {
	return binary.BigEndian.Uint32(ip)
}

func AddIPv4(ip net.IP, delta uint32) {
	val := IPv4ToLongHBO(ip) + delta
	binary.BigEndian.PutUint32(ip, val)
}

func CompareIPv4(ip1, ip2 net.IP) int {
	ip1Val := IPv4ToLongHBO(ip1)
	ip2Val := IPv4ToLongHBO(ip2)
	if ip1Val > ip2Val {
		return 1
	} else if ip1Val == ip2Val {
		return 0
	} else {
		return -1
	}
}
