#include <kea/server/host.h>
#include <gtest/gtest.h>

using namespace kea;
using namespace kea::dhcp;
using namespace kea::server;

namespace kea{

TEST(HostTest, createFromDUIDString) {
    std::unique_ptr<Host> host;
    ASSERT_NO_THROW(host.reset(new Host("a1:b2:c3:d4:e5:06", "duid",
                                        SubnetID(10),
                                        IPAddress("192.0.2.5"),
                                        "me.example.org")));

    // DUID should be set to non-null value.
    const DUID* duid = host->getDuid();
    ASSERT_TRUE(duid);

    EXPECT_EQ("a1:b2:c3:d4:e5:06", duid->toText());

    // Hardware address must be null if DUID is in use.
    EXPECT_FALSE(host->getHWAddress());

    EXPECT_EQ(10, host->getIPv4SubnetID());
    EXPECT_EQ("192.0.2.5", host->getIPv4Reservation().str());
    EXPECT_EQ("me.example.org", host->getHostname());

    // Use invalid DUID.
    EXPECT_THROW(Host("bogus", "duid", SubnetID(1), 
                      IPAddress("192.0.2.3"), "somehost.example.org"),
                 kea::BadValue);

    // Empty DUID is also not allowed.
    EXPECT_THROW(Host("", "duid", SubnetID(1),
                      IPAddress("192.0.2.3"), "somehost.example.org"),
                 kea::BadValue);
}

// This test verifies that it is possible to create Host object using
// hardware address in the binary format.
TEST(HostTest, createFromHWAddrBinary) {
    std::unique_ptr<Host> host;
    // Prepare the hardware address in binary format.
    const uint8_t hwaddr_data[] = {
        0xaa, 0xab, 0xca, 0xda, 0xbb, 0xee
    };
    ASSERT_NO_THROW(host.reset(new Host(hwaddr_data,
                                        sizeof(hwaddr_data),
                                        Host::IDENT_HWADDR,
                                        SubnetID(1),
                                        IPAddress("192.0.2.3"),
                                        "somehost.example.org")));
    // Hardware address should be non-null.
    const HWAddr* hwaddr = host->getHWAddress();
    ASSERT_TRUE(hwaddr);

    EXPECT_EQ("hwtype=1 aa:ab:ca:da:bb:ee", hwaddr->toText());

    // DUID should be null if hardware address is in use.
    EXPECT_FALSE(host->getDuid());
    EXPECT_EQ(1, host->getIPv4SubnetID());
    EXPECT_EQ("192.0.2.3", host->getIPv4Reservation().str());
    EXPECT_EQ("somehost.example.org", host->getHostname());
}

TEST(HostTest, createFromDuidBinary) {
    std::unique_ptr<Host> host;
    const uint8_t duid_data[] = {
        1, 2, 3, 4, 5, 6
    };
    ASSERT_NO_THROW(host.reset(new Host(duid_data,
                                        sizeof(duid_data),
                                        Host::IDENT_DUID,
                                        SubnetID(10),
                                        IPAddress("192.0.2.5"),
                                        "me.example.org")));
    const DUID* duid = host->getDuid();
    ASSERT_TRUE(duid);

    EXPECT_EQ("01:02:03:04:05:06", duid->toText());

    // Hardware address should be null if DUID is in use.
    EXPECT_FALSE(host->getHWAddress());
    EXPECT_EQ(10, host->getIPv4SubnetID());
    EXPECT_EQ("192.0.2.5", host->getIPv4Reservation().str());
    EXPECT_EQ("me.example.org", host->getHostname());
}

// Test that it is possible to replace an identifier for a particular
// Host instance (HW address -> DUID and vice versa) with a new
// indentifier in the textual format.
TEST(HostTest, setIdentifierString) {
    std::unique_ptr<Host> host;
    ASSERT_NO_THROW(host.reset(new Host("01:02:03:04:05:06", "hw-address",
                                        SubnetID(1),
                                        IPAddress("192.0.2.3"),
                                        "me.example.com")));
    // Initially, there should be a HW address, but not a DUID set.
    ASSERT_TRUE(host->getHWAddress());
    ASSERT_FALSE(host->getDuid());

    // Now, use a DUID as identifier.
    ASSERT_NO_THROW(host->setIdentifier("aabbccddee", "duid"));

    // Verify that the DUID is correct.
    auto duid = host->getDuid();
    ASSERT_TRUE(duid);
    EXPECT_EQ("aa:bb:cc:dd:ee", duid->toText());
    // HW address should be not set.
    EXPECT_FALSE(host->getHWAddress());

    // Now, let's do another way around.

    ASSERT_NO_THROW(host->setIdentifier("09:08:07:06:05:04", "hw-address"));

    // Verify that HW address is correct.
    auto hw_addr = host->getHWAddress();
    ASSERT_TRUE(hw_addr);
    EXPECT_EQ("hwtype=1 09:08:07:06:05:04", hw_addr->toText());
    // DUID should be not set.
    EXPECT_FALSE(host->getDuid());
}

// Test that it is possible to replace an identifier for a particular
// Host instance (HW address -> DUID and vice versa) with the new
// identifier in the binary format.
TEST(HostTest, setIdentifierBinary) {
    std::unique_ptr<Host> host;
    ASSERT_NO_THROW(host.reset(new Host("01:02:03:04:05:06", "hw-address",
                                        SubnetID(1),
                                        IPAddress("192.0.2.3"),
                                        "me.example.com")));
    // Initially, there should be a HW address, but not a DUID set.
    ASSERT_TRUE(host->getHWAddress());
    ASSERT_FALSE(host->getDuid());

    // Now, use a DUID as identifier.
    const uint8_t duid_data[] = {
        0xaa, 0xbb, 0xcc, 0xdd, 0xee
    };
    ASSERT_NO_THROW(host->setIdentifier(duid_data, sizeof(duid_data),
                                        Host::IDENT_DUID));

    // Verify that the DUID is correct.
    auto duid = host->getDuid();
    ASSERT_TRUE(duid);
    EXPECT_EQ("aa:bb:cc:dd:ee", duid->toText());
    // HW address should be not set.
    EXPECT_FALSE(host->getHWAddress());

    // Now, let's do another way around.

    const uint8_t hwaddr_data[] = {
        9, 8, 7, 6, 5, 4
    };
    ASSERT_NO_THROW(host->setIdentifier(hwaddr_data, sizeof(hwaddr_data),
                                        Host::IDENT_HWADDR));

    // Verify that HW address is correct.
    auto hw_addr = host->getHWAddress();
    ASSERT_TRUE(hw_addr);
    EXPECT_EQ("hwtype=1 09:08:07:06:05:04", hw_addr->toText());
    // DUID should be not set.
    EXPECT_FALSE(host->getDuid());
}

TEST(HostTest, setValues) {
    std::unique_ptr<Host> host;
    ASSERT_NO_THROW(host.reset(new Host("01:02:03:04:05:06", "hw-address",
                                        SubnetID(1),
                                        IPAddress("192.0.2.3"),
                                        "some-host.example.org")));

    ASSERT_EQ(1, host->getIPv4SubnetID());
    ASSERT_EQ("192.0.2.3", host->getIPv4Reservation().str());
    ASSERT_EQ("some-host.example.org", host->getHostname());

    host->setIPv4SubnetID(SubnetID(123));
    host->setIPv4Reservation(IPAddress("10.0.0.1"));
    host->setHostname("other-host.example.org");

    EXPECT_EQ(123, host->getIPv4SubnetID());
    EXPECT_EQ("10.0.0.1", host->getIPv4Reservation().str());
    EXPECT_EQ("other-host.example.org", host->getHostname());

    host->removeIPv4Reservation();
    EXPECT_EQ(IPAddress::fromLong(0), host->getIPv4Reservation());

    EXPECT_THROW(host->setIPv4Reservation(IPAddress("2001:db8:1::1")),
                 kea::BadValue);

    EXPECT_THROW(host->setIPv4Reservation(IPAddress::fromLong(0)),
                 kea::BadValue);
}

// Test that Host constructors initialize client classes from string.
TEST(HostTest, clientClassesFromConstructor) {
    std::unique_ptr<Host> host;
    // Prepare the hardware address in binary format.
    const uint8_t hwaddr_data[] = {
        0xaa, 0xab, 0xca, 0xda, 0xbb, 0xee
    };

    // Try the "from binary" constructor.
    ASSERT_NO_THROW(host.reset(new Host(hwaddr_data,
                                        sizeof(hwaddr_data),
                                        Host::IDENT_HWADDR,
                                        SubnetID(1),
                                        IPAddress("192.0.2.3"),
                                        "somehost.example.org",
                                        "alpha, , beta")));

    EXPECT_TRUE(host->getClientClasses4().contains("alpha"));
    EXPECT_TRUE(host->getClientClasses4().contains("beta"));
    EXPECT_FALSE(host->getClientClasses4().contains("gamma"));

    // Try the "from string" constructor.
    ASSERT_NO_THROW(host.reset(new Host("01:02:03:04:05:06", "hw-address",
                                        SubnetID(1),
                                        IPAddress("192.0.2.3"),
                                        "somehost.example.org",
                                        "alpha, beta, gamma")));

    EXPECT_TRUE(host->getClientClasses4().contains("alpha"));
    EXPECT_TRUE(host->getClientClasses4().contains("beta"));
    EXPECT_TRUE(host->getClientClasses4().contains("gamma"));
}

TEST(HostTest, addClientClasses) {
    std::unique_ptr<Host> host;
    ASSERT_NO_THROW(host.reset(new Host("01:02:03:04:05:06", "hw-address",
                                        SubnetID(1),
                                        IPAddress("192.0.2.3"))));

    EXPECT_FALSE(host->getClientClasses4().contains("foo"));
    EXPECT_FALSE(host->getClientClasses4().contains("bar"));

    host->addClientClass4("foo");
    EXPECT_TRUE(host->getClientClasses4().contains("foo"));
    EXPECT_FALSE(host->getClientClasses4().contains("bar"));

    host->addClientClass4("bar");
    EXPECT_TRUE(host->getClientClasses4().contains("foo"));
    EXPECT_TRUE(host->getClientClasses4().contains("bar"));
}

TEST(HostTest, getIdentifierAsText) {
    Host host1("01:02:03:04:05:06", "hw-address",
               SubnetID(1),
               IPAddress("192.0.2.3"));
    EXPECT_EQ("hwaddr=01:02:03:04:05:06", host1.getIdentifierAsText());

    Host host2("0a:0b:0c:0d:0e:0f:ab:cd:ef", "duid",
               SubnetID(1),
               IPAddress("192.0.2.3"));
    EXPECT_EQ("duid=0a:0b:0c:0d:0e:0f:ab:cd:ef",
              host2.getIdentifierAsText());
}

TEST(HostTest, toText) {
    std::unique_ptr<Host> host;
    ASSERT_NO_THROW(host.reset(new Host("01:02:03:04:05:06", "hw-address",
                                        SubnetID(1),
                                        IPAddress("192.0.2.3"),
                                        "myhost.example.com")));

    // Make sure that the output is correct,
    EXPECT_EQ("hwaddr=01:02:03:04:05:06 ipv4_subnet_id=1"
              " hostname=myhost.example.com"
              " ipv4_reservation=192.0.2.3",
              host->toText());

    // Reset some of the data and make sure that the output is affected.
    host->setHostname("");
    host->removeIPv4Reservation();
    host->setIPv4SubnetID(0);

    EXPECT_EQ("hwaddr=01:02:03:04:05:06"
              " hostname=(empty) ipv4_reservation=(no)",
              host->toText());
    // Create host identified by DUID, instead of HWADDR, with a very
    // basic configuration.
    ASSERT_NO_THROW(host.reset(new Host("11:12:13:14:15", "duid",
                                        SubnetID(0),
                                        IPAddress("0.0.0.0"),
                                        "myhost")));

    EXPECT_EQ("duid=11:12:13:14:15 hostname=myhost ipv4_reservation=(no)", host->toText());

    // Add some classes.
    host->addClientClass4("modem");
    host->addClientClass4("router");

    EXPECT_EQ("duid=11:12:13:14:15 hostname=myhost ipv4_reservation=(no)"
              " dhcp4_class0=modem dhcp4_class1=router",
              host->toText());

}

};
