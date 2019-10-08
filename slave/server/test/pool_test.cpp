#include <kea/server/pool.h>
#include <gtest/gtest.h>

#include <iostream>
#include <vector>
#include <sstream>

using namespace kea;
using namespace kea::dhcp;
using namespace kea::server;

namespace kea {

TEST(Pool4Test, constructor_first_last) {

    // let's construct 192.0.2.1-192.0.2.255 pool
    Pool4 pool1(IPAddress("192.0.2.1"), IPAddress("192.0.2.255"));

    EXPECT_EQ(IPAddress("192.0.2.1"), pool1.getFirstAddress());
    EXPECT_EQ(IPAddress("192.0.2.255"), pool1.getLastAddress());

    // This is Pool4, IPv6 addresses do not belong here
    EXPECT_THROW(Pool4(IPAddress("2001:db8::1"),
                       IPAddress("192.168.0.5")), BadValue);
    EXPECT_THROW(Pool4(IPAddress("192.168.0.2"),
                       IPAddress("2001:db8::1")), BadValue);

    // Should throw. Range should be 192.0.2.1-192.0.2.2, not
    // the other way around.
    EXPECT_THROW(Pool4(IPAddress("192.0.2.2"), IPAddress("192.0.2.1")),
                 BadValue);
}

TEST(Pool4Test, constructor_prefix_len) {

    // let's construct 2001:db8:1::/96 pool
    Pool4 pool1(IPAddress("192.0.2.0"), 25);

    EXPECT_EQ("192.0.2.0", pool1.getFirstAddress().str());
    EXPECT_EQ("192.0.2.127", pool1.getLastAddress().str());

    // No such thing as /33 prefix
    EXPECT_THROW(Pool4(IPAddress("192.0.2.1"), 33), BadValue);

    // /0 prefix does not make sense
    EXPECT_THROW(Pool4(IPAddress("192.0.2.0"), 0), BadValue);

    // This is Pool6, IPv4 addresses do not belong here
    EXPECT_THROW(Pool4(IPAddress("2001:db8::1"), 20), BadValue);
}

TEST(Pool4Test, in_range) {
   Pool pool1(IPAddress("192.0.2.10"), IPAddress("192.0.2.20"));

   EXPECT_FALSE(pool1.inRange(IPAddress("192.0.2.0")));
   EXPECT_TRUE(pool1.inRange(IPAddress("192.0.2.10")));
   EXPECT_TRUE(pool1.inRange(IPAddress("192.0.2.17")));
   EXPECT_TRUE(pool1.inRange(IPAddress("192.0.2.20")));
   EXPECT_FALSE(pool1.inRange(IPAddress("192.0.2.21")));
   EXPECT_FALSE(pool1.inRange(IPAddress("192.0.2.255")));
   EXPECT_FALSE(pool1.inRange(IPAddress("255.255.255.255")));
   EXPECT_FALSE(pool1.inRange(IPAddress("0.0.0.0")));
}

// Checks if the number of possible leases in range is reported correctly.
TEST(Pool4Test, leasesCount) {
    Pool pool1(IPAddress("192.0.2.10"), IPAddress("192.0.2.20"));
    EXPECT_EQ(11, pool1.getCapacity());

    Pool pool2(IPAddress("192.0.2.0"), IPAddress("192.0.2.255"));
    EXPECT_EQ(256, pool2.getCapacity());

    Pool pool3(IPAddress("192.168.0.0"), IPAddress("192.168.255.255"));
    EXPECT_EQ(65536, pool3.getCapacity());

    Pool pool4(IPAddress("10.0.0.0"), IPAddress("10.255.255.255"));
    EXPECT_EQ(16777216, pool4.getCapacity());
}

typedef std::unique_ptr<Pool4> Pool4Ptr;

// This test creates 100 pools and verifies that their IDs are unique.
TEST(Pool4Test, unique_id) {

    const int num_pools = 100;
    std::vector<Pool4Ptr> pools;

    for (int i = 0; i < num_pools; ++i) {
        pools.push_back(Pool4Ptr(new Pool4(IPAddress("192.0.2.0"),
                                           IPAddress("192.0.2.255"))));
    }

    for (int i = 0; i < num_pools; ++i) {
        for (int j = i + 1; j < num_pools; ++j) {
            if (pools[i]->getId() == pools[j]->getId()) {
                FAIL() << "Pool-ids must be unique";
            }
        }
    }
}

// Simple check if toText returns reasonable values
TEST(Pool4Test,toText) {
    Pool4 pool1(IPAddress("192.0.2.7"), IPAddress("192.0.2.17"));
    EXPECT_EQ("type=V4, 192.0.2.7-192.0.2.17", pool1.toText());

    Pool4 pool2(IPAddress("192.0.2.128"), 28);
    EXPECT_EQ("type=V4, 192.0.2.128-192.0.2.143", pool2.toText());
}
};
