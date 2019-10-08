#include <kea/dhcp++/duid.h>
#include <kea/server/lease.h>
#include <kea/util/pointer_util.h>
#include <gtest/gtest.h>
#include <vector>
#include <sstream>

using namespace kea;
using namespace kea::dhcp;
using namespace kea::server;

namespace kea {

/// Hardware address used by different tests.
const uint8_t HWADDR[] = {0x08, 0x00, 0x2b, 0x02, 0x3f, 0x4e};
/// Client id used by different tests.
const uint8_t CLIENTID[] = {0x17, 0x34, 0xe2, 0xff, 0x09, 0x92, 0x54};
/// Valid lifetime value used by different tests.
const uint32_t VALID_LIFETIME = 500;
/// Subnet ID used by different tests.
const uint32_t SUBNET_ID = 42;
/// IAID value used by different tests.
const uint32_t IAID = 7;

Lease4 createLease4(const std::string& hostname, const bool fqdn_fwd,
                    const bool fqdn_rev) {
    Lease4 lease;
    lease.hostname_ = hostname;
    lease.fqdn_fwd_ = fqdn_fwd;
    lease.fqdn_rev_ = fqdn_rev;
    return (lease);
}

std::unique_ptr<HWAddr> getDefaultHWAddr() {
    return (std::unique_ptr<HWAddr>(new HWAddr(HWADDR, sizeof(HWADDR), HTYPE_ETHER)));
}

std::unique_ptr<ClientId> getDefaultClientId() {
    return (std::unique_ptr<ClientId>(new ClientId(CLIENTID, sizeof(CLIENTID))));
}

TEST(Lease4Test, constructor) {
    const time_t current_time = time(NULL);

    const uint32_t ADDRESS[] = {
        0x00000000, 0x01020304, 0x7fffffff, 0x80000000, 0x80000001, 0xffffffff
    };

    for (int i = 0; i < sizeof(ADDRESS) / sizeof(ADDRESS[0]); ++i) {
        Lease4 lease(IPAddress::fromLong(ADDRESS[i]), getDefaultHWAddr(), getDefaultClientId(), VALID_LIFETIME, 0, 0,
                     current_time, SUBNET_ID, true, true,
                     "hostname.example.com.");

        EXPECT_EQ(ADDRESS[i], lease.addr_.asV4().toLong());
        EXPECT_TRUE(util::equalValues(getDefaultHWAddr(), lease.hwaddr_));
        EXPECT_TRUE(util::equalValues(getDefaultClientId(), lease.client_id_));
        EXPECT_EQ(0, lease.t1_);
        EXPECT_EQ(0, lease.t2_);
        EXPECT_EQ(VALID_LIFETIME, lease.valid_lft_);
        EXPECT_EQ(current_time, lease.cltt_);
        EXPECT_EQ(SUBNET_ID, lease.subnet_id_);
        EXPECT_EQ("hostname.example.com.", lease.hostname_);
        EXPECT_TRUE(lease.fqdn_fwd_);
        EXPECT_TRUE(lease.fqdn_rev_);
        EXPECT_EQ(Lease::STATE_DEFAULT, lease.state_);
    }
}


TEST(Lease4Test, copyConstructor) {
    const time_t current_time = time(NULL);

    Lease4 lease(IPAddress::fromLong(0xffffffff), getDefaultHWAddr(), getDefaultClientId(), VALID_LIFETIME, 0, 0, current_time,
                 SUBNET_ID);
    lease.state_ = Lease::STATE_DECLINED;
    Lease4 copied_lease(lease);

    EXPECT_TRUE(lease == copied_lease);
    EXPECT_FALSE(lease.client_id_ == copied_lease.client_id_);
    EXPECT_TRUE(*lease.hwaddr_ == *copied_lease.hwaddr_);
    EXPECT_FALSE(lease.hwaddr_ == copied_lease.hwaddr_);

    lease.hwaddr_.reset();
    Lease4 copied_lease2(lease);
    EXPECT_TRUE(lease == copied_lease2);
}

TEST(Lease4Test, operatorAssign) {

    const time_t current_time = time(NULL);
    Lease4 lease(IPAddress::fromLong(0xffffffff), getDefaultHWAddr(), getDefaultClientId(), VALID_LIFETIME, 0, 0, current_time,
                 SUBNET_ID);
    lease.state_ = Lease::STATE_DECLINED;

    Lease4 copied_lease;
    copied_lease = lease;

    EXPECT_TRUE(lease == copied_lease);
    EXPECT_FALSE(lease.client_id_ == copied_lease.client_id_);
    EXPECT_TRUE(*lease.hwaddr_ == *copied_lease.hwaddr_);
    EXPECT_FALSE(lease.hwaddr_ == copied_lease.hwaddr_);
    lease.hwaddr_.reset();
    copied_lease = lease;
    EXPECT_TRUE(lease == copied_lease);
}

TEST(Lease4Test, leaseBelongsToClient) {
    std::unique_ptr<ClientId> matching_client_id(ClientId::fromText("01:02:03:04"));
    std::unique_ptr<ClientId> diff_client_id(ClientId::fromText("01:02:03:05"));
    std::unique_ptr<ClientId> null_client_id;
    std::unique_ptr<HWAddr> matching_hw = HWAddr::fromText("00:01:02:03:04:05", HTYPE_ETHER);
    std::unique_ptr<HWAddr> diff_hw = HWAddr::fromText("00:01:02:03:04:06", HTYPE_ETHER);
    std::unique_ptr<HWAddr> null_hw;

    Lease4 lease(IPAddress("192.0.2.1"), 
                 HWAddr::fromText("00:01:02:03:04:05", HTYPE_ETHER), 
                 ClientId::fromText("01:02:03:04"),
                 60, time(NULL), 0, 0, 1);

    EXPECT_TRUE(lease.belongsToClient(matching_hw.get(), matching_client_id.get()));
    EXPECT_FALSE(lease.belongsToClient(matching_hw.get(), diff_client_id.get()));
    EXPECT_TRUE(lease.belongsToClient(matching_hw.get(), null_client_id.get()));
    EXPECT_TRUE(lease.belongsToClient(diff_hw.get(), matching_client_id.get()));
    EXPECT_FALSE(lease.belongsToClient(diff_hw.get(), diff_client_id.get()));
    EXPECT_FALSE(lease.belongsToClient(diff_hw.get(), null_client_id.get()));
    EXPECT_TRUE(lease.belongsToClient(null_hw.get(), matching_client_id.get()));
    EXPECT_FALSE(lease.belongsToClient(null_hw.get(), diff_client_id.get()));
    EXPECT_FALSE(lease.belongsToClient(null_hw.get(), null_client_id.get()));


    lease.client_id_.reset(nullptr);;
    EXPECT_TRUE(lease.belongsToClient(matching_hw.get(), matching_client_id.get()));
    EXPECT_TRUE(lease.belongsToClient(matching_hw.get(), diff_client_id.get()));
    EXPECT_TRUE(lease.belongsToClient(matching_hw.get(), null_client_id.get()));
    EXPECT_FALSE(lease.belongsToClient(diff_hw.get(), matching_client_id.get()));
    EXPECT_FALSE(lease.belongsToClient(diff_hw.get(), diff_client_id.get()));
    EXPECT_FALSE(lease.belongsToClient(diff_hw.get(), null_client_id.get()));
    EXPECT_FALSE(lease.belongsToClient(null_hw.get(), matching_client_id.get()));
    EXPECT_FALSE(lease.belongsToClient(null_hw.get(), diff_client_id.get()));
    EXPECT_FALSE(lease.belongsToClient(null_hw.get(), null_client_id.get()));

    lease.client_id_ = ClientId::fromText("01:02:03:04");
    lease.hwaddr_.reset(nullptr);
    EXPECT_TRUE(lease.belongsToClient(matching_hw.get(), matching_client_id.get()));
    EXPECT_FALSE(lease.belongsToClient(matching_hw.get(), diff_client_id.get()));
    EXPECT_FALSE(lease.belongsToClient(matching_hw.get(), null_client_id.get()));
    EXPECT_TRUE(lease.belongsToClient(diff_hw.get(), matching_client_id.get()));
    EXPECT_FALSE(lease.belongsToClient(diff_hw.get(), diff_client_id.get()));
    EXPECT_FALSE(lease.belongsToClient(diff_hw.get(), null_client_id.get()));
    EXPECT_TRUE(lease.belongsToClient(null_hw.get(), matching_client_id.get()));
    EXPECT_FALSE(lease.belongsToClient(null_hw.get(), diff_client_id.get()));
    EXPECT_FALSE(lease.belongsToClient(null_hw.get(), null_client_id.get()));
}

TEST(Lease4Test, operatorEquals) {

    const uint32_t ADDRESS = 0x01020304;
    const time_t current_time = time(NULL);

    Lease4 lease1(IPAddress::fromLong(ADDRESS), getDefaultHWAddr(), getDefaultClientId(), VALID_LIFETIME, current_time, 0,
                  0, SUBNET_ID);

    Lease4 lease2(IPAddress::fromLong(ADDRESS), getDefaultHWAddr(), getDefaultClientId(), VALID_LIFETIME, current_time,
                  0, 0, SUBNET_ID);
    EXPECT_TRUE(lease1 == lease2);
    EXPECT_FALSE(lease1 != lease2);

    lease1.addr_ = IPAddress::fromLong(ADDRESS + 1);
    EXPECT_FALSE(lease1 == lease2);
    EXPECT_TRUE(lease1 != lease2);
    lease1.addr_ = lease2.addr_;
    EXPECT_TRUE(lease1 == lease2);  // Check that the reversion has made the
    EXPECT_FALSE(lease1 != lease2); // ... leases equal

    std::vector<uint8_t> clientid_vec = getDefaultClientId()->getClientId();
    ++clientid_vec[0];
    lease1.client_id_.reset(new ClientId(clientid_vec));
    EXPECT_FALSE(lease1 == lease2);
    EXPECT_TRUE(lease1 != lease2);
    --clientid_vec[0];
    lease1.client_id_.reset(new ClientId(clientid_vec));
    EXPECT_TRUE(lease1 == lease2);  // Check that the reversion has made the
    EXPECT_FALSE(lease1 != lease2); // ... leases equal

    ++lease1.t1_;
    EXPECT_FALSE(lease1 == lease2);
    EXPECT_TRUE(lease1 != lease2);
    lease1.t1_ = lease2.t1_;
    EXPECT_TRUE(lease1 == lease2);  // Check that the reversion has made the
    EXPECT_FALSE(lease1 != lease2); // ... leases equal

    ++lease1.t2_;
    EXPECT_FALSE(lease1 == lease2);
    EXPECT_TRUE(lease1 != lease2);
    lease1.t2_ = lease2.t2_;
    EXPECT_TRUE(lease1 == lease2);  // Check that the reversion has made the
    EXPECT_FALSE(lease1 != lease2); // ... leases equal

    ++lease1.valid_lft_;
    EXPECT_FALSE(lease1 == lease2);
    EXPECT_TRUE(lease1 != lease2);
    lease1.valid_lft_ = lease2.valid_lft_;
    EXPECT_TRUE(lease1 == lease2);  // Check that the reversion has made the
    EXPECT_FALSE(lease1 != lease2); // ... leases equal

    ++lease1.cltt_;
    EXPECT_FALSE(lease1 == lease2);
    EXPECT_TRUE(lease1 != lease2);
    lease1.cltt_ = lease2.cltt_;
    EXPECT_TRUE(lease1 == lease2);  // Check that the reversion has made the
    EXPECT_FALSE(lease1 != lease2); // ... leases equal

    ++lease1.subnet_id_;
    EXPECT_FALSE(lease1 == lease2);
    EXPECT_TRUE(lease1 != lease2);
    lease1.subnet_id_ = lease2.subnet_id_;
    EXPECT_TRUE(lease1 == lease2);  // Check that the reversion has made the
    EXPECT_FALSE(lease1 != lease2); // ... leases equal

    lease1.hostname_ += std::string("Something random");
    EXPECT_FALSE(lease1 == lease2);
    EXPECT_TRUE(lease1 != lease2);
    lease1.hostname_ = lease2.hostname_;
    EXPECT_TRUE(lease1 == lease2);  // Check that the reversion has made the
    EXPECT_FALSE(lease1 != lease2); // ... leases equal

    lease1.fqdn_fwd_ = !lease1.fqdn_fwd_;
    EXPECT_FALSE(lease1 == lease2);
    EXPECT_TRUE(lease1 != lease2);
    lease1.fqdn_fwd_ = lease2.fqdn_fwd_;
    EXPECT_TRUE(lease1 == lease2);  // Check that the reversion has made the
    EXPECT_FALSE(lease1 != lease2); // ... leases equal

    lease1.fqdn_rev_ = !lease1.fqdn_rev_;
    EXPECT_FALSE(lease1 == lease2);
    EXPECT_TRUE(lease1 != lease2);
    lease1.fqdn_rev_ = lease2.fqdn_rev_;
    EXPECT_TRUE(lease1 == lease2);  // Check that the reversion has made the
    EXPECT_FALSE(lease1 != lease2); // ... leases equal

    lease1.state_  += 1;
    EXPECT_FALSE(lease1 == lease2);
    EXPECT_TRUE(lease1 != lease2);
    lease2.state_ += 1;
    EXPECT_TRUE(lease1 == lease2);  // Check that the reversion has made the
    EXPECT_FALSE(lease1 != lease2); // ... leases equal
}

TEST(Lease4Test, getClientIdVector) {
    Lease4 lease;
    ASSERT_FALSE(lease.client_id_);
    EXPECT_TRUE(lease.getClientIdVector().empty());

    lease.client_id_ = getDefaultClientId();
    std::vector<uint8_t> returned_vec = lease.getClientIdVector();
    EXPECT_TRUE(returned_vec == getDefaultClientId()->getClientId());
}

TEST(Lease4Test, hasIdenticalFqdn) {
    Lease4 lease = createLease4("myhost.example.com.", true, true);
    EXPECT_TRUE(lease.hasIdenticalFqdn(createLease4("myhost.example.com.",
                                                     true, true)));
    EXPECT_FALSE(lease.hasIdenticalFqdn(createLease4("other.example.com.",
                                                     true, true)));
    EXPECT_FALSE(lease.hasIdenticalFqdn(createLease4("myhost.example.com.",
                                                     false, true)));
    EXPECT_FALSE(lease.hasIdenticalFqdn(createLease4("myhost.example.com.",
                                                     true, false)));
    EXPECT_FALSE(lease.hasIdenticalFqdn(createLease4("myhost.example.com.",
                                                     false, false)));
    EXPECT_FALSE(lease.hasIdenticalFqdn(createLease4("other.example.com.",
                                                     false, false)));
}

TEST(Lease4Test, toText) {
    const time_t current_time = 12345678;
    Lease4 lease(IPAddress("192.0.2.3"), getDefaultHWAddr(), getDefaultClientId(), 3600, 123, 456, current_time, 789);

    std::stringstream expected;
    expected << "Address:       192.0.2.3\n"
             << "Valid life:    3600\n"
             << "T1:            123\n"
             << "T2:            456\n"
             << "Cltt:          12345678\n"
             << "Hardware addr: " << getDefaultHWAddr()->toText(false) << "\n"
             << "Client id:     " << getDefaultClientId()->toText() << "\n"
             << "Subnet ID:     789\n"
             << "State:         default\n";

    EXPECT_EQ(expected.str(), lease.toText());

    // Now let's try with a lease without hardware address and client identifier.
    lease.hwaddr_.reset();
    lease.client_id_.reset();
    expected.str("");
    expected << "Address:       192.0.2.3\n"
             << "Valid life:    3600\n"
             << "T1:            123\n"
             << "T2:            456\n"
             << "Cltt:          12345678\n"
             << "Hardware addr: (none)\n"
             << "Client id:     (none)\n"
             << "Subnet ID:     789\n"
             << "State:         default\n";
    EXPECT_EQ(expected.str(), lease.toText());
}

// Verify that decline() method properly clears up specific fields.
TEST(Lease4Test, decline) {

    const time_t current_time = 12345678;
    Lease4 lease(IPAddress("192.0.2.3"), getDefaultHWAddr(), getDefaultClientId(), 3600, 123,
                 456, current_time, 789);
    lease.hostname_="foo.example.org";
    lease.fqdn_fwd_ = true;
    lease.fqdn_rev_ = true;

    time_t now = time(NULL);
    lease.decline(123);
    ASSERT_TRUE(lease.hwaddr_.get() != nullptr);
    EXPECT_EQ("", lease.hwaddr_->toText(false));
    EXPECT_TRUE(lease.client_id_.get() == nullptr);
    EXPECT_EQ(0, lease.t1_);
    EXPECT_EQ(0, lease.t2_);

    EXPECT_TRUE(now <= lease.cltt_);
    EXPECT_TRUE(lease.cltt_ <= now + 1);
    EXPECT_EQ("", lease.hostname_);
    EXPECT_FALSE(lease.fqdn_fwd_);
    EXPECT_FALSE(lease.fqdn_rev_);
    EXPECT_EQ(Lease::STATE_DECLINED, lease.state_);
    EXPECT_EQ(123, lease.valid_lft_);
}

// Verify that the lease states are correctly returned in the textual format.
TEST(Lease4Test, stateToText) {
    EXPECT_EQ("default", Lease4::statesToText(Lease::STATE_DEFAULT));
    EXPECT_EQ("declined", Lease4::statesToText(Lease::STATE_DECLINED));
    EXPECT_EQ("expired-reclaimed", Lease4::statesToText(Lease::STATE_EXPIRED_RECLAIMED));
}


};
