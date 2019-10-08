#include <kea/dhcp++/duid.h>
#include <kea/dhcp++/hwaddr.h>
#include <kea/server/host.h>
#include <kea/server/subnet_id.h>
#include <kea/server/hosts_in_mem.h>

#include <gtest/gtest.h>
#include <sstream>
#include <set>

using namespace kea;
using namespace kea::dhcp;
using namespace kea::server;

namespace kea {

typedef std::unique_ptr<HWAddr> HWAddrPtr;
typedef std::unique_ptr<DUID> DuidPtr;
typedef std::unique_ptr<Host> HostPtr;

class HostsInMemTest : public ::testing::Test {
public:
    HostsInMemTest();

    IPAddress increase(const IPAddress& address, const uint8_t num) const;

    std::vector<HWAddrPtr> hwaddrs_;
    std::vector<DuidPtr> duids_;
    std::vector<IPAddress> addressesa_;
    std::vector<IPAddress> addressesb_;
};

HostsInMemTest::HostsInMemTest() {
    const uint8_t mac_template[] = {
        0x01, 0x02, 0x0A, 0xBB, 0x03, 0x00
    };
    for (unsigned i = 0; i < 50; ++i) {
        std::vector<uint8_t> vec(mac_template,
                                 mac_template + sizeof(mac_template));
        vec[vec.size() - 1] = i;
        hwaddrs_.push_back(HWAddrPtr(new HWAddr(vec, HTYPE_ETHER)));
    }

    const uint8_t duid_template[] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x00
    };
    for (unsigned i = 0; i < 50; ++i) {
        std::vector<uint8_t> vec(duid_template,
                                 duid_template + sizeof(mac_template));
        vec[vec.size() - 1] = i;
        duids_.push_back(DuidPtr(new DUID(vec)));
    }

    const uint32_t addra_template = 0xc0000205; // 192.0.2.5
    const uint32_t addrb_template = 0xc00a020a; // 192.10.2.10
    for (int i = 0; i < 50; ++i) {
        IPAddress addra = IPAddress::fromLong(addra_template + i);
        addressesa_.push_back(addra);
        IPAddress addrb = IPAddress::fromLong(addrb_template + i);
        addressesb_.push_back(addrb);
    }
}

IPAddress
HostsInMemTest::increase(const IPAddress& address, const uint8_t num) const {
    return (IPAddress::fromLongHBO(address.asV4().toLongHBO() + num));
}

TEST_F(HostsInMemTest, getAllNonRepeatingHosts) {
    HostsInMem cfg;
    for (int i = 0; i < 25; ++i) {
        cfg.add(HostPtr(new Host(hwaddrs_[i]->toText(false),
                                 "hw-address",
                                 SubnetID(i % 10 + 1),
                                 addressesa_[i])));

        cfg.add(HostPtr(new Host(duids_[i]->toText(), "duid",
                                 SubnetID(i % 5 + 1),
                                 addressesb_[i])));

    }

    for (int i = 24; i >= 0; --i) {
        HostCollection hosts = cfg.getAll(hwaddrs_[i].get(), duids_[i + 25].get());
        ASSERT_EQ(1, hosts.size());
        EXPECT_EQ(i % 10 + 1, hosts[0]->getIPv4SubnetID());
        EXPECT_EQ(addressesa_[i].str(),
                  hosts[0]->getIPv4Reservation().str());

        hosts = cfg.getAll(hwaddrs_[i + 25].get(), duids_[i].get());
        ASSERT_EQ(1, hosts.size());
        EXPECT_EQ(i % 5 + 1, hosts[0]->getIPv4SubnetID());
        EXPECT_EQ(addressesb_[i].str(),
                  hosts[0]->getIPv4Reservation().str());
    }

    for (int i = 49; i >= 25; --i) {
        EXPECT_TRUE(cfg.getAll(hwaddrs_[i].get()).empty());
        EXPECT_TRUE(cfg.getAll(nullptr, duids_[i].get()).empty());
    }
}

TEST_F(HostsInMemTest, getAllRepeatingHosts) {
    HostsInMem cfg;
    for (int i = 0; i < 25; ++i) {
        cfg.add(HostPtr(new Host(hwaddrs_[i]->toText(false),
                                 "hw-address",
                                 SubnetID(1),
                                 addressesa_[i])));
        cfg.add(HostPtr(new Host(hwaddrs_[i]->toText(false),
                                 "hw-address",
                                 SubnetID(2),
                                 addressesb_[i])));

        cfg.add(HostPtr(new Host(duids_[i]->toText(), "duid",
                                 SubnetID(1),
                                 addressesb_[i])));
        cfg.add(HostPtr(new Host(duids_[i]->toText(), "duid",
                                 SubnetID(2),
                                 addressesa_[i])));
    }

    for (int i = 0; i < 25; ++i) {
        HostCollection hosts = cfg.getAll(hwaddrs_[i].get(), duids_[i + 25].get());
        ASSERT_EQ(2, hosts.size());
        EXPECT_EQ(1, hosts[0]->getIPv4SubnetID());
        EXPECT_EQ(addressesa_[i].str(), hosts[0]->getIPv4Reservation().str());
        EXPECT_EQ(2, hosts[1]->getIPv4SubnetID());
        EXPECT_EQ(addressesb_[i].str(), hosts[1]->getIPv4Reservation().str());

        hosts = cfg.getAll(hwaddrs_[i + 25].get(), duids_[i].get());
        ASSERT_EQ(2, hosts.size());
        EXPECT_EQ(1, hosts[0]->getIPv4SubnetID());
        EXPECT_EQ(2, hosts[1]->getIPv4SubnetID());
    }

    for (int i = 25; i < 50; ++i) {
        EXPECT_TRUE(cfg.getAll(hwaddrs_[i].get()).empty());
        EXPECT_TRUE(cfg.getAll(nullptr, duids_[i].get()).empty());
    }
}

TEST_F(HostsInMemTest, getAll4ByAddress) {
    HostsInMem cfg;
    for (int i = 0; i < 25; ++i) {
        cfg.add(HostPtr(new Host(hwaddrs_[i]->toText(false),
                                 "hw-address",
                                 SubnetID(1 + i),
                                 IPAddress("192.0.2.5"))));
        cfg.add(HostPtr(new Host(duids_[i]->toText(),
                                 "duid",
                                 SubnetID(1 + i),
                                 IPAddress("192.0.2.10"))));
    }

    HostCollection hosts = cfg.getAll4(IPAddress("192.0.2.10"));
    std::set<uint32_t> subnet_ids;
    for (auto host : hosts) {
        subnet_ids.insert(host->getIPv4SubnetID());
    }
    ASSERT_EQ(25, subnet_ids.size());
    EXPECT_EQ(1, *subnet_ids.begin());
    EXPECT_EQ(25, *subnet_ids.rbegin());
}

TEST_F(HostsInMemTest, get4) {
    HostsInMem cfg;
    for (unsigned i = 0; i < 25; ++i) {
        cfg.add(HostPtr(new Host(hwaddrs_[i]->toText(false),
                                 "hw-address",
                                 SubnetID(1 + i % 2),
                                 increase(IPAddress("192.0.2.5"), i))));

        cfg.add(HostPtr(new Host(duids_[i]->toText(), "duid",
                                 SubnetID(1 + i % 2),
                                 increase(IPAddress("192.0.2.100"), i))));
    }

    for (unsigned i = 0; i < 25; ++i) {
        const Host* host = cfg.get4(SubnetID(1 + i % 2), hwaddrs_[i].get(),
                                duids_[i + 25].get());
        ASSERT_TRUE(host);
        EXPECT_EQ(1 + i % 2, host->getIPv4SubnetID());
        EXPECT_EQ(increase(IPAddress("192.0.2.5"), i),
                  host->getIPv4Reservation());

        host = cfg.get4(SubnetID(1 + i % 2), hwaddrs_[i + 25].get(), duids_[i].get());
        ASSERT_TRUE(host);
        EXPECT_EQ(1 + i % 2, host->getIPv4SubnetID());
        EXPECT_EQ(increase(IPAddress("192.0.2.100"), i),
                  host->getIPv4Reservation());

    }

    //EXPECT_THROW(cfg.get4(SubnetID(1), hwaddrs_[0].get(), duids_[0].get()), DuplicateHost);
}

TEST_F(HostsInMemTest, add4AlreadyReserved) {
    HostsInMem cfg;

    HostPtr host1 = HostPtr(new Host(hwaddrs_[0]->toText(false),
                                     "hw-address",
                                     SubnetID(1),
                                     IPAddress("192.0.2.1")));
    EXPECT_NO_THROW(cfg.add(std::move(host1)));

    HostPtr host2 = HostPtr(new Host(hwaddrs_[1]->toText(false),
                                     "hw-address",
                                     SubnetID(1),
                                     IPAddress("192.0.2.1")));

    EXPECT_THROW(cfg.add(std::move(host2)), ReservedAddress);
}

TEST_F(HostsInMemTest, zeroSubnetIDs) {
    HostsInMem cfg;
    ASSERT_THROW(cfg.add(HostPtr(new Host(hwaddrs_[0]->toText(false),
                                          "hw-address",
                                          SubnetID(0),
                                          IPAddress("10.0.0.1")))),
                 BadValue);
}

TEST_F(HostsInMemTest, duplicatesSubnet4HWAddr) {
    HostsInMem cfg;
    ASSERT_NO_THROW(cfg.add(HostPtr(new Host(hwaddrs_[0]->toText(false),
                                             "hw-address",
                                             SubnetID(10),
                                             IPAddress("10.0.0.1")))));

    EXPECT_THROW(cfg.add(HostPtr(new Host(hwaddrs_[0]->toText(false),
                                          "hw-address",
                                          SubnetID(10),
                                          IPAddress("10.0.0.10")))),
                 DuplicateHost);

    EXPECT_NO_THROW(cfg.add(HostPtr(new Host(hwaddrs_[0]->toText(false),
                                             "hw-address",
                                             SubnetID(11),
                                             IPAddress("10.0.0.10")))));
}

TEST_F(HostsInMemTest, duplicatesSubnet4DUID) {
    HostsInMem cfg;
    ASSERT_NO_THROW(cfg.add(HostPtr(new Host(duids_[0]->toText(),
                                             "duid",
                                             SubnetID(10),
                                             IPAddress("10.0.0.1")))));

    EXPECT_THROW(cfg.add(HostPtr(new Host(duids_[0]->toText(),
                                          "duid",
                                          SubnetID(10),
                                          IPAddress("10.0.0.10")))),
                 DuplicateHost);

    EXPECT_NO_THROW(cfg.add(HostPtr(new Host(duids_[0]->toText(),
                                             "duid",
                                             SubnetID(11),
                                             IPAddress("10.0.0.10")))));
}
} // end of anonymous namespace
