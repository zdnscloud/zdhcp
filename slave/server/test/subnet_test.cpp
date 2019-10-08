#include <kea/dhcp++/option.h>
#include <kea/server/subnet.h>
#include <kea/exceptions/exceptions.h>

#include <gtest/gtest.h>
#include <limits>

// don't import the entire boost namespace.  It will unexpectedly hide uint8_t
// for some systems.
using namespace kea;
using namespace kea::dhcp;
using namespace kea::server;

namespace kea {

typedef std::unique_ptr<Subnet4> Subnet4Ptr;
typedef std::unique_ptr<Pool> PoolPtr;

TEST(Subnet4Test, constructor) {
    EXPECT_NO_THROW(Subnet4 subnet1(IPAddress("192.0.2.2"), 16, 1, 2, 3, 10));
    EXPECT_THROW(Subnet4 subnet2(IPAddress("192.0.2.0"), 33, 1, 2, 3), BadValue); // invalid prefix length
}

TEST(Subnet4Test, subnetID) {
    Subnet4Ptr subnet(new Subnet4(IPAddress("192.0.2.0"), 24, 1000, 2000,
                                  3000));
    SubnetID id0 = subnet->getID();
    subnet.reset(new Subnet4(IPAddress("192.0.3.0"), 24, 1000, 2000,
                             3000));
    SubnetID id1 = subnet->getID();
    EXPECT_NE(id0, id1);
    subnet.reset(new Subnet4(IPAddress("192.0.4.0"), 24, 1000, 2000,
                             3000, id1));
    EXPECT_EQ(id1, subnet->getID());
}

TEST(Subnet4Test, inRange) {
    Subnet4 subnet(IPAddress("192.0.2.1"), 24, 1000, 2000, 3000);

    EXPECT_EQ(1000, subnet.getT1());
    EXPECT_EQ(2000, subnet.getT2());
    EXPECT_EQ(3000, subnet.getValid());

    EXPECT_EQ("0.0.0.0", subnet.getRelayInfo().addr_.str());

    EXPECT_FALSE(subnet.inRange(IPAddress("192.0.0.0")));
    EXPECT_TRUE(subnet.inRange(IPAddress("192.0.2.0")));
    EXPECT_TRUE(subnet.inRange(IPAddress("192.0.2.1")));
    EXPECT_TRUE(subnet.inRange(IPAddress("192.0.2.255")));
    EXPECT_FALSE(subnet.inRange(IPAddress("192.0.3.0")));
    EXPECT_FALSE(subnet.inRange(IPAddress("0.0.0.0")));
    EXPECT_FALSE(subnet.inRange(IPAddress("255.255.255.255")));
}

TEST(Subnet4Test, relay) {
    Subnet4 subnet(IPAddress("192.0.2.1"), 24, 1000, 2000, 3000);
    EXPECT_EQ("0.0.0.0", subnet.getRelayInfo().addr_.str());
    subnet.setRelayInfo(IPAddress("192.0.123.45"));
    EXPECT_EQ("192.0.123.45", subnet.getRelayInfo().addr_.str());
}

TEST(Subnet4Test, siaddr) {
    Subnet4 subnet(IPAddress("192.0.2.1"), 24, 1000, 2000, 3000);

    EXPECT_EQ("0.0.0.0", subnet.getSiaddr().str());
    EXPECT_NO_THROW(subnet.setSiaddr(IPAddress("1.2.3.4")));
    EXPECT_EQ("1.2.3.4", subnet.getSiaddr().str());
    EXPECT_THROW(subnet.setSiaddr(IPAddress("2001:db8::1")), BadValue);
}

TEST(Subnet4Test, matchClientId) {
    Subnet4 subnet(IPAddress("192.0.2.1"), 24, 1000, 2000, 3000);
    EXPECT_TRUE(subnet.getMatchClientId());
    subnet.setMatchClientId(false);
    EXPECT_FALSE(subnet.getMatchClientId());

    subnet.setMatchClientId(true);
    EXPECT_TRUE(subnet.getMatchClientId());
}

TEST(Subnet4Test, Pool4InSubnet4) {
    Subnet4Ptr subnet(new Subnet4(IPAddress("192.1.2.0"), 24, 1, 2, 3));
    PoolPtr pool1(new Pool4(IPAddress("192.1.2.0"), 25));
    PoolPtr pool2(new Pool4(IPAddress("192.1.2.128"), 26));
    PoolPtr pool3(new Pool4(IPAddress("192.1.2.192"), 30));

    Pool *testPool = new Pool4(IPAddress("192.1.2.0"), 25);
    EXPECT_NO_THROW(subnet->addPool(PoolPtr(testPool)));
    const Pool* mypool = subnet->getAnyPool(Lease::TYPE_V4);
    EXPECT_TRUE(mypool != nullptr);
    EXPECT_EQ(mypool->toText(), pool1->toText());

    EXPECT_NO_THROW(subnet->addPool(std::move(pool2)));
    EXPECT_NO_THROW(subnet->addPool(PoolPtr(new Pool4(IPAddress("192.1.2.192"), 30))));

    EXPECT_NO_THROW(mypool = subnet->getAnyPool(Lease::TYPE_V4));
    EXPECT_EQ(mypool->toText(), pool1->toText());
    EXPECT_NO_THROW(mypool = subnet->getPool(Lease::TYPE_V4, IPAddress("192.1.2.195")));
    EXPECT_EQ(mypool->toText(), pool3->toText());
}

TEST(Subnet4Test, getCapacity) {

    Subnet4Ptr subnet(new Subnet4(IPAddress("192.1.2.0"), 24, 1, 2, 3));

    EXPECT_EQ(0, subnet->getPoolCapacity(Lease::TYPE_V4));

    PoolPtr pool1(new Pool4(IPAddress("192.1.2.0"), 25));
    subnet->addPool(std::move(pool1));
    EXPECT_EQ(128, subnet->getPoolCapacity(Lease::TYPE_V4));

    PoolPtr pool2(new Pool4(IPAddress("192.1.2.128"), 26));
    subnet->addPool(std::move(pool2));
    EXPECT_EQ(192, subnet->getPoolCapacity(Lease::TYPE_V4));

    PoolPtr pool3(new Pool4(IPAddress("192.1.2.192"), 30));
    subnet->addPool(std::move(pool3));
    EXPECT_EQ(196, subnet->getPoolCapacity(Lease::TYPE_V4));
}

TEST(Subnet4Test, Subnet4_Pool4_checks) {
    Subnet4Ptr subnet(new Subnet4(IPAddress("192.0.2.0"), 8, 1, 2, 3));

    PoolPtr pool1(new Pool4(IPAddress("192.255.0.0"), 16));
    subnet->addPool(std::move(pool1));

    // this one is larger than the subnet!
    PoolPtr pool2(new Pool4(IPAddress("193.0.0.0"), 24));
    EXPECT_THROW(subnet->addPool(std::move(pool2)), BadValue);

    // this one is totally out of blue
    PoolPtr pool3(new Pool4(IPAddress("1.2.3.4"), 16));
    EXPECT_THROW(subnet->addPool(std::move(pool3)), BadValue);
}

// Tests whether Subnet4 object is able to store and process properly
// information about allowed client class (a single class).
TEST(Subnet4Test, clientClasses) {
    // Create the V4 subnet.
    Subnet4Ptr subnet(new Subnet4(IPAddress("192.0.2.0"), 8, 1, 2, 3));

    kea::dhcp::ClientClasses no_class;

    kea::dhcp::ClientClasses foo_class;
    foo_class.insert("foo");

    kea::dhcp::ClientClasses bar_class;
    bar_class.insert("bar");

    // This client belongs to foo, bar and baz classes.
    kea::dhcp::ClientClasses three_classes;
    three_classes.insert("foo");
    three_classes.insert("bar");
    three_classes.insert("baz");

    // No class restrictions defined, any client should be supported
    EXPECT_TRUE(subnet->clientSupported(no_class));
    EXPECT_TRUE(subnet->clientSupported(foo_class));
    EXPECT_TRUE(subnet->clientSupported(bar_class));
    EXPECT_TRUE(subnet->clientSupported(three_classes));

    // Let's allow only clients belongning to "bar" class.
    subnet->allowClientClass("bar");

    EXPECT_FALSE(subnet->clientSupported(no_class));
    EXPECT_FALSE(subnet->clientSupported(foo_class));
    EXPECT_TRUE(subnet->clientSupported(bar_class));
    EXPECT_TRUE(subnet->clientSupported(three_classes));
}

TEST(Subnet4Test, clientClassesMultiple) {
    Subnet4Ptr subnet(new Subnet4(IPAddress("192.0.2.0"), 8, 1, 2, 3));

    kea::dhcp::ClientClasses no_class;

    kea::dhcp::ClientClasses foo_class;
    foo_class.insert("foo");

    // This client belongs to bar only. I like that client.
    kea::dhcp::ClientClasses bar_class;
    bar_class.insert("bar");

    // No class restrictions defined, any client should be supported
    EXPECT_TRUE(subnet->clientSupported(no_class));
    EXPECT_TRUE(subnet->clientSupported(foo_class));
    EXPECT_TRUE(subnet->clientSupported(bar_class));

    subnet->allowClientClass("bar");
    subnet->allowClientClass("foo");

    EXPECT_FALSE(subnet->clientSupported(no_class));
    EXPECT_TRUE(subnet->clientSupported(foo_class));
    EXPECT_TRUE(subnet->clientSupported(bar_class));
}

TEST(Subnet4Test, inRangeinPool) {
    Subnet4Ptr subnet(new Subnet4(IPAddress("192.0.0.0"), 8, 1, 2, 3));

    // this one is in subnet
    PoolPtr pool1(new Pool4(IPAddress("192.2.0.0"), 16));
    subnet->addPool(std::move(pool1));

    // 192.1.1.1 belongs to the subnet...
    EXPECT_TRUE(subnet->inRange(IPAddress("192.1.1.1")));

    // ... but it does not belong to any pool within
    EXPECT_FALSE(subnet->inPool(Lease::TYPE_V4, IPAddress("192.1.1.1")));

    // the last address that is in range, but out of pool
    EXPECT_TRUE(subnet->inRange(IPAddress("192.1.255.255")));
    EXPECT_FALSE(subnet->inPool(Lease::TYPE_V4, IPAddress("192.1.255.255")));

    // the first address that is in range, in pool
    EXPECT_TRUE(subnet->inRange(IPAddress("192.2.0.0")));
    EXPECT_TRUE (subnet->inPool(Lease::TYPE_V4, IPAddress("192.2.0.0")));

    // let's try something in the middle as well
    EXPECT_TRUE(subnet->inRange(IPAddress("192.2.3.4")));
    EXPECT_TRUE (subnet->inPool(Lease::TYPE_V4, IPAddress("192.2.3.4")));

    // the last address that is in range, in pool
    EXPECT_TRUE(subnet->inRange(IPAddress("192.2.255.255")));
    EXPECT_TRUE (subnet->inPool(Lease::TYPE_V4, IPAddress("192.2.255.255")));

    // the first address that is in range, but out of pool
    EXPECT_TRUE(subnet->inRange(IPAddress("192.3.0.0")));
    EXPECT_FALSE(subnet->inPool(Lease::TYPE_V4, IPAddress("192.3.0.0")));
}

// This test checks if the str() method returns text representation
TEST(Subnet4Test, toText) {
    Subnet4Ptr subnet(new Subnet4(IPAddress("192.0.2.0"), 24, 1, 2, 3));
    EXPECT_EQ("192.0.2.0/24", subnet->toText());
}

// This test checks if the get() method returns proper parameters
TEST(Subnet4Test, get) {
    Subnet4Ptr subnet(new Subnet4(IPAddress("192.0.2.0"), 28, 1, 2, 3));
    EXPECT_EQ("192.0.2.0", subnet->get().first.str());
    EXPECT_EQ(28, subnet->get().second);
}


// Checks if last allocated address/prefix is stored/retrieved properly
TEST(Subnet4Test, lastAllocated) {
    IPAddress addr("192.0.2.17");

    IPAddress last("192.0.2.255");

    Subnet4Ptr subnet(new Subnet4(IPAddress("192.0.2.0"), 24, 1, 2, 3));

    // Check initial conditions (all should be set to the last address in range)
    EXPECT_EQ(last.str(), subnet->getLastAllocated(Lease::TYPE_V4).str());

    // Now set last allocated for IA
    EXPECT_NO_THROW(subnet->setLastAllocated(Lease::TYPE_V4, addr));
    EXPECT_EQ(addr.str(), subnet->getLastAllocated(Lease::TYPE_V4).str());

    // No, you can't set the last allocated IPv6 address in IPv4 subnet
    EXPECT_THROW(subnet->setLastAllocated(Lease::TYPE_TA, addr), BadValue);
    EXPECT_THROW(subnet->setLastAllocated(Lease::TYPE_TA, addr), BadValue);
    EXPECT_THROW(subnet->setLastAllocated(Lease::TYPE_PD, addr), BadValue);
}

// Checks if the V4 is the only allowed type for Pool4 and if getPool()
// is working properly.
TEST(Subnet4Test, PoolType) {
    Subnet4Ptr subnet(new Subnet4(IPAddress("192.2.0.0"), 16, 1, 2, 3));

    PoolPtr pool1(new Pool4(IPAddress("192.2.1.0"), 24));
    PoolPtr pool2(new Pool4(IPAddress("192.2.2.0"), 24));

    EXPECT_EQ(nullptr, subnet->getAnyPool(Lease::TYPE_V4));

    EXPECT_THROW(subnet->getAnyPool(Lease::TYPE_NA), BadValue);
    EXPECT_THROW(subnet->getAnyPool(Lease::TYPE_TA), BadValue);
    EXPECT_THROW(subnet->getAnyPool(Lease::TYPE_PD), BadValue);

    EXPECT_NO_THROW(subnet->addPool(PoolPtr(new Pool4(IPAddress("192.2.1.0"), 24))));

    // If there's only one IA pool, get that pool (without and with hint)
    EXPECT_EQ(pool1->toText(), subnet->getAnyPool(Lease::TYPE_V4)->toText());
    EXPECT_EQ(pool1->toText(), subnet->getPool(Lease::TYPE_V4, IPAddress("192.0.1.167"))->toText());

    EXPECT_NO_THROW(subnet->addPool(PoolPtr(new Pool4(IPAddress("192.2.2.0"), 24))));
    EXPECT_EQ(pool1->toText(), subnet->getAnyPool(Lease::TYPE_V4)->toText());

    // Try with valid hints
    EXPECT_EQ(pool1->toText(), subnet->getPool(Lease::TYPE_V4, IPAddress("192.2.1.5"))->toText());
    EXPECT_EQ(pool2->toText(), subnet->getPool(Lease::TYPE_V4, IPAddress("192.2.2.254"))->toText());

    // Try with bogus hints (hints should be ingored)
    EXPECT_EQ(pool1->toText(), subnet->getPool(Lease::TYPE_V4, IPAddress("10.1.1.1"))->toText());
}

};
