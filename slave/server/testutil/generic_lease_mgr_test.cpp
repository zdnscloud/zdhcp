#include <kea/server/testutil/generic_lease_mgr_test.h>
#include <kea/server/testutil/lease_compare.h>
#include <kea/server/database_connection.h>
#include <folly/IPAddress.h>
#include <gtest/gtest.h>
#include <sstream>

using namespace std;
using namespace folly;

namespace kea {
namespace server {
namespace testutil {

// IPv4 and IPv6 addresses used in the tests
const char* ADDRESS4[] = {
    "192.0.2.0", "192.0.2.1", "192.0.2.2", "192.0.2.3",
    "192.0.2.4", "192.0.2.5", "192.0.2.6", "192.0.2.7",
    NULL
};
const char* ADDRESS6[] = {
    "2001:db8::0", "2001:db8::1", "2001:db8::2", "2001:db8::3",
    "2001:db8::4", "2001:db8::5", "2001:db8::6", "2001:db8::7",
    NULL
};

// Lease types that correspond to ADDRESS6 leases
static const Lease::Type LEASETYPE6[] = {
    Lease::TYPE_NA, Lease::TYPE_TA, Lease::TYPE_PD, Lease::TYPE_NA,
    Lease::TYPE_TA, Lease::TYPE_PD, Lease::TYPE_NA, Lease::TYPE_TA
};

GenericLeaseMgrTest::GenericLeaseMgrTest()
    : lmptr_(NULL) {
    // Initialize address strings and IPAddresses
    for (int i = 0; ADDRESS4[i] != NULL; ++i) {
        string addr(ADDRESS4[i]);
        straddress4_.push_back(addr);
        IPAddress ioaddr(addr);
        ioaddress4_.push_back(ioaddr);
    }
}

GenericLeaseMgrTest::~GenericLeaseMgrTest() {
}

LeaseMgr::Lease4Ptr
GenericLeaseMgrTest::initializeLease4(std::string address) {
    LeaseMgr::Lease4Ptr lease(new Lease4());

    // Set the address of the lease
    lease->addr_ = IPAddress(address);

    // Initialize unused fields.
    lease->t1_ = 0;                             // Not saved
    lease->t2_ = 0;                             // Not saved

    // Set other parameters.  For historical reasons, address 0 is not used.
    if (address == straddress4_[0]) {
        lease->hwaddr_.reset(new HWAddr(vector<uint8_t>(6, 0x08), HTYPE_ETHER));
        lease->client_id_ = std::unique_ptr<ClientId>(new ClientId(vector<uint8_t>(8, 0x42)));
        lease->valid_lft_ = 8677;
        lease->cltt_ = 168256;
        lease->subnet_id_ = 23;
        lease->fqdn_rev_ = true;
        lease->fqdn_fwd_ = false;
        lease->hostname_ = "myhost.example.com.";

    } else if (address == straddress4_[1]) {
        lease->hwaddr_.reset(new HWAddr(vector<uint8_t>(6, 0x19), HTYPE_ETHER));
        lease->client_id_ = std::unique_ptr<ClientId>(
            new ClientId(vector<uint8_t>(8, 0x53)));
        lease->valid_lft_ = 3677;
        lease->cltt_ = 123456;
        lease->subnet_id_ = 73;
        lease->fqdn_rev_ = true;
        lease->fqdn_fwd_ = true;
        lease->hostname_ = "myhost.example.com.";

    } else if (address == straddress4_[2]) {
        lease->hwaddr_.reset(new HWAddr(vector<uint8_t>(6, 0x2a), HTYPE_ETHER));
        lease->client_id_ = std::unique_ptr<ClientId>(new ClientId(vector<uint8_t>(8, 0x64)));
        lease->valid_lft_ = 5412;
        lease->cltt_ = 234567;
        lease->subnet_id_ = 73;                         // Same as lease 1
        lease->fqdn_rev_ = false;
        lease->fqdn_fwd_ = false;
        lease->hostname_ = "";

    } else if (address == straddress4_[3]) {
        // Hardware address same as lease 1.
        lease->hwaddr_.reset(new HWAddr(vector<uint8_t>(6, 0x19), HTYPE_ETHER));
        lease->client_id_ = std::unique_ptr<ClientId>(
            new ClientId(vector<uint8_t>(8, 0x75)));

        // The times used in the next tests are deliberately restricted - we
        // should be able to cope with valid lifetimes up to 0xffffffff.
        //  However, this will lead to overflows.
        // @TODO: test overflow conditions when code has been fixed
        lease->valid_lft_ = 7000;
        lease->cltt_ = 234567;
        lease->subnet_id_ = 37;
        lease->fqdn_rev_ = true;
        lease->fqdn_fwd_ = true;
        lease->hostname_ = "otherhost.example.com.";

    } else if (address == straddress4_[4]) {
        lease->hwaddr_.reset(new HWAddr(vector<uint8_t>(6, 0x4c), HTYPE_ETHER));
        // Same ClientId as straddr4_[1]
        lease->client_id_ = std::unique_ptr<ClientId>(
            new ClientId(vector<uint8_t>(8, 0x53)));    // Same as lease 1
        lease->valid_lft_ = 7736;
        lease->cltt_ = 222456;
        lease->subnet_id_ = 85;
        lease->fqdn_rev_ = true;
        lease->fqdn_fwd_ = true;
        lease->hostname_ = "otherhost.example.com.";

    } else if (address == straddress4_[5]) {
        // Same as lease 1
        lease->hwaddr_.reset(new HWAddr(vector<uint8_t>(6, 0x19), HTYPE_ETHER));
        // Same ClientId and IAID as straddress4_1
        lease->client_id_ = std::unique_ptr<ClientId>(
            new ClientId(vector<uint8_t>(8, 0x53)));    // Same as lease 1
        lease->valid_lft_ = 7832;
        lease->cltt_ = 227476;
        lease->subnet_id_ = 175;
        lease->fqdn_rev_ = false;
        lease->fqdn_fwd_ = false;
        lease->hostname_ = "otherhost.example.com.";
    } else if (address == straddress4_[6]) {
        lease->hwaddr_.reset(new HWAddr(vector<uint8_t>(6, 0x6e), HTYPE_ETHER));
        // Same ClientId as straddress4_1
        lease->client_id_ = std::unique_ptr<ClientId>(
            new ClientId(vector<uint8_t>(8, 0x53)));    // Same as lease 1
        lease->valid_lft_ = 1832;
        lease->cltt_ = 627476;
        lease->subnet_id_ = 112;
        lease->fqdn_rev_ = false;
        lease->fqdn_fwd_ = true;
        lease->hostname_ = "myhost.example.com.";

    } else if (address == straddress4_[7]) {
        lease->hwaddr_.reset(new HWAddr(vector<uint8_t>(), HTYPE_ETHER)); // Empty
        lease->client_id_ = std::unique_ptr<ClientId>();              // Empty
        lease->valid_lft_ = 7975;
        lease->cltt_ = 213876;
        lease->subnet_id_ = 19;
        lease->fqdn_rev_ = true;
        lease->fqdn_fwd_ = true;
        lease->hostname_ = "myhost.example.com.";

    } else {
        // Unknown address, return an empty pointer.
        lease.reset();
    }

    return (std::move(lease));
}

template <typename T>
void GenericLeaseMgrTest::checkLeasesDifferent(const std::vector<T>& leases) const {

    // Check they were created
    for (size_t i = 0; i < leases.size(); ++i) {
        ASSERT_TRUE(leases[i].get() != nullptr);
    }

    // Check they are different
    for (size_t i = 0; i < (leases.size() - 1); ++i) {
        for (size_t j = (i + 1); j < leases.size(); ++j) {
            stringstream s;
            s << "Comparing leases " << i << " & " << j << " for equality";
            SCOPED_TRACE(s.str());
            EXPECT_TRUE(*leases[i] != *leases[j]);
        }
    }
}

void
GenericLeaseMgrTest::createLeases4(vector<LeaseMgr::Lease4Ptr>& leases) {
    for (size_t i = 0; i < straddress4_.size(); ++i) {
        leases.push_back(initializeLease4(straddress4_[i]));
    }
    EXPECT_EQ(8, leases.size());
    checkLeasesDifferent(leases);
}

void
GenericLeaseMgrTest::testGetLease4ClientId() {
    // Let's initialize a specific lease ...
    LeaseMgr::Lease4Ptr lease = initializeLease4(straddress4_[1]);
    EXPECT_TRUE(lmptr_->addLease(*lease));
    LeaseMgr::Lease4Collection returned; 
    lmptr_->getLease4(*lease->client_id_, returned);

    ASSERT_EQ(1, returned.size());
    // We should retrieve our lease...
    detailCompareLease(lease, returned[0]);
    lease = initializeLease4(straddress4_[2]);
    returned.clear();
    lmptr_->getLease4(*lease->client_id_, returned);

    ASSERT_EQ(0, returned.size());
}

void
GenericLeaseMgrTest::testGetLease4NullClientId() {
    LeaseMgr::Lease4Ptr leaseA = initializeLease4(straddress4_[4]);
    ClientId client_id(*leaseA->client_id_);
    leaseA->client_id_ = std::unique_ptr<ClientId>();
    ASSERT_TRUE(lmptr_->addLease(*leaseA));

    LeaseMgr::Lease4Collection returned;
    lmptr_->getLease4(client_id, returned);
    // Shouldn't have our previous lease ...
    ASSERT_TRUE(returned.empty());

    LeaseMgr::Lease4Ptr leaseB = initializeLease4(straddress4_[0]);
    ASSERT_TRUE(lmptr_->addLease(*leaseB));
    returned.clear();
    lmptr_->getLease4(client_id, returned);
    ASSERT_TRUE(returned.empty());

    LeaseMgr::Lease4Ptr leaseC = initializeLease4(straddress4_[5]);
    leaseC->client_id_.reset();
    ASSERT_TRUE(lmptr_->addLease(*leaseC));
    returned.clear();
    lmptr_->getLease4(client_id, returned);
    ASSERT_TRUE(returned.empty());

    lmptr_->getLease4(*leaseB->client_id_, returned);
    ASSERT_EQ(1, returned.size());
}

void
GenericLeaseMgrTest::testLease4NullClientId() {
    vector<LeaseMgr::Lease4Ptr> leases;
    createLeases4(leases);

    // Let's clear client-id pointers
    leases[1]->client_id_ = std::unique_ptr<ClientId>();
    leases[2]->client_id_ = std::unique_ptr<ClientId>();
    leases[3]->client_id_ = std::unique_ptr<ClientId>();

    // Start the tests.  Add three leases to the database, read them back and
    // check they are what we think they are.
    EXPECT_TRUE(lmptr_->addLease(*leases[1]));
    EXPECT_TRUE(lmptr_->addLease(*leases[2]));
    EXPECT_TRUE(lmptr_->addLease(*leases[3]));
    lmptr_->commit();

    // Reopen the database to ensure that they actually got stored.
    reopen();

    LeaseMgr::Lease4Ptr l_returned = lmptr_->getLease4(ioaddress4_[1]);
    ASSERT_TRUE(l_returned.get() != nullptr);
    detailCompareLease(leases[1], l_returned);

    l_returned = lmptr_->getLease4(ioaddress4_[2]);
    ASSERT_TRUE(l_returned.get() != nullptr);
    detailCompareLease(leases[2], l_returned);

    l_returned = lmptr_->getLease4(ioaddress4_[3]);
    ASSERT_TRUE(l_returned.get() != nullptr);
    detailCompareLease(leases[3], l_returned);

    // Check that we can't add a second lease with the same address
    EXPECT_FALSE(lmptr_->addLease(*leases[1]));

    // Check that we can get the lease by HWAddr
    HWAddr tmp(*leases[2]->hwaddr_);
    LeaseMgr::Lease4Collection returned;
    lmptr_->getLease4(tmp, returned);
    ASSERT_EQ(1, returned.size());
    detailCompareLease(leases[2], *returned.begin());

    l_returned = lmptr_->getLease4(tmp, leases[2]->subnet_id_);
    ASSERT_TRUE(l_returned.get() != nullptr);
    detailCompareLease(leases[2], l_returned);

    // Check that we can update the lease
    // Modify some fields in lease 1 (not the address) and update it.
    ++leases[1]->subnet_id_;
    leases[1]->valid_lft_ *= 2;
    lmptr_->updateLease4(*leases[1]);

    // ... and check that the lease is indeed updated
    l_returned = lmptr_->getLease4(ioaddress4_[1]);
    ASSERT_TRUE(l_returned.get() != nullptr);
    detailCompareLease(leases[1], l_returned);

    // Delete a lease, check that it's gone, and that we can't delete it
    // a second time.
    EXPECT_TRUE(lmptr_->deleteLease(ioaddress4_[1]));
    l_returned = lmptr_->getLease4(ioaddress4_[1]);
    EXPECT_TRUE(l_returned.get() == nullptr);
    EXPECT_FALSE(lmptr_->deleteLease(ioaddress4_[1]));

    // Check that the second address is still there.
    l_returned = lmptr_->getLease4(ioaddress4_[2]);
    ASSERT_TRUE(l_returned.get() != nullptr);
    detailCompareLease(leases[2], l_returned);
}

void
GenericLeaseMgrTest::testGetLease4HWAddr1() {
    LeaseMgr::Lease4Ptr leaseA = initializeLease4(straddress4_[5]);
    HWAddr hwaddrA(*leaseA->hwaddr_);
    HWAddr hwaddrB(vector<uint8_t>(6, 0x80), HTYPE_ETHER);

    EXPECT_TRUE(lmptr_->addLease(*leaseA));

    // we should not have a lease, with this MAC Addr
    LeaseMgr::Lease4Collection returned;
    lmptr_->getLease4(hwaddrB, returned);
    ASSERT_EQ(0, returned.size());

    // But with this one
    returned.clear();
    lmptr_->getLease4(hwaddrA, returned);
    ASSERT_EQ(1, returned.size());
}

void
GenericLeaseMgrTest::testGetLease4HWAddr2() {
    vector<LeaseMgr::Lease4Ptr> leases;
    createLeases4(leases);
    for (size_t i = 0; i < leases.size(); ++i) {
        EXPECT_TRUE(lmptr_->addLease(*leases[i]));
    }

    HWAddr tmp(*leases[1]->hwaddr_);
    LeaseMgr::Lease4Collection returned;
    lmptr_->getLease4(tmp, returned);
    ASSERT_EQ(3, returned.size());

    vector<string> addresses;
    for (auto& lease : returned) {
        addresses.push_back(lease->addr_.str());
    }
    sort(addresses.begin(), addresses.end());
    EXPECT_EQ(straddress4_[1], addresses[0]);
    EXPECT_EQ(straddress4_[3], addresses[1]);
    EXPECT_EQ(straddress4_[5], addresses[2]);

    // Repeat test with just one expected match
    returned.clear();
    lmptr_->getLease4(*leases[2]->hwaddr_, returned);
    ASSERT_EQ(1, returned.size());
    detailCompareLease(leases[2], *returned.begin());

    // Check that an empty vector is valid
    EXPECT_TRUE(leases[7]->hwaddr_->hwaddr_.empty());
    returned.clear();
    lmptr_->getLease4(*leases[7]->hwaddr_, returned);
    ASSERT_EQ(1, returned.size());
    detailCompareLease(leases[7], *returned.begin());

    vector<uint8_t> invalid(6, 0);
    returned.clear();
    lmptr_->getLease4(invalid, returned);
    EXPECT_EQ(0, returned.size());
}

void
GenericLeaseMgrTest::testGetLease4ClientIdHWAddrSubnetId() {
    LeaseMgr::Lease4Ptr leaseA = initializeLease4(straddress4_[4]);
    LeaseMgr::Lease4Ptr leaseB = initializeLease4(straddress4_[5]);
    LeaseMgr::Lease4Ptr leaseC = initializeLease4(straddress4_[6]);

    leaseC->client_id_.reset();
    HWAddr hwaddrA(*leaseA->hwaddr_);
    HWAddr hwaddrB(*leaseB->hwaddr_);
    HWAddr hwaddrC(*leaseC->hwaddr_);
    EXPECT_TRUE(lmptr_->addLease(*leaseA));
    EXPECT_TRUE(lmptr_->addLease(*leaseB));
    EXPECT_TRUE(lmptr_->addLease(*leaseC));
    LeaseMgr::Lease4Ptr lease = lmptr_->getLease4(*leaseA->client_id_, hwaddrA, leaseA->subnet_id_);
    detailCompareLease(lease, leaseA);
    // Retrieve the other lease.
    lease = lmptr_->getLease4(*leaseB->client_id_, hwaddrB, leaseB->subnet_id_);
    detailCompareLease(lease, leaseB);
    lease = lmptr_->getLease4(hwaddrC, leaseC->subnet_id_);
    detailCompareLease(lease, leaseC);

    lease = lmptr_->getLease4(*leaseA->client_id_, hwaddrB, leaseA->subnet_id_);
    EXPECT_TRUE(lease.get() == nullptr);
    lease = lmptr_->getLease4(*leaseA->client_id_, hwaddrA, leaseB->subnet_id_);
    EXPECT_TRUE(lease.get() == nullptr);
}

void
GenericLeaseMgrTest::testMaxDate4() {
    vector<LeaseMgr::Lease4Ptr> leases;
    createLeases4(leases);

    // Set valid_lft_ to 1 day, cllt_ to max time. This should make expire
    // time too large to store.
    leases[1]->valid_lft_ = 24*60*60;
    leases[1]->cltt_ = DatabaseConnection::MAX_DB_TIME;

    ASSERT_THROW(lmptr_->addLease(*leases[1]), DbOperationError);

    leases[1]->valid_lft_ = 0;
    EXPECT_TRUE(lmptr_->addLease(*leases[1]));
    LeaseMgr::Lease4Ptr l_returned = lmptr_->getLease4(ioaddress4_[1]);
    ASSERT_TRUE(l_returned.get() != nullptr);
    detailCompareLease(leases[1], l_returned);
}

void
GenericLeaseMgrTest::testBasicLease4() {
    vector<LeaseMgr::Lease4Ptr> leases; 
    createLeases4(leases);

    EXPECT_TRUE(lmptr_->addLease(*leases[1]));
    EXPECT_TRUE(lmptr_->addLease(*leases[2]));
    EXPECT_TRUE(lmptr_->addLease(*leases[3]));
    lmptr_->commit();

    reopen(V4);

    LeaseMgr::Lease4Ptr l_returned = lmptr_->getLease4(ioaddress4_[1]);
    ASSERT_TRUE(l_returned.get() != nullptr);
    detailCompareLease(leases[1], l_returned);

    l_returned = lmptr_->getLease4(ioaddress4_[2]);
    ASSERT_TRUE(l_returned.get() != nullptr);
    detailCompareLease(leases[2], l_returned);

    l_returned = lmptr_->getLease4(ioaddress4_[3]);
    ASSERT_TRUE(l_returned.get() != nullptr);
    detailCompareLease(leases[3], l_returned);

    EXPECT_FALSE(lmptr_->addLease(*leases[1]));

    EXPECT_TRUE(lmptr_->deleteLease(ioaddress4_[1]));
    l_returned = lmptr_->getLease4(ioaddress4_[1]);
    EXPECT_TRUE(l_returned.get() == nullptr);
    EXPECT_FALSE(lmptr_->deleteLease(ioaddress4_[1]));

    l_returned = lmptr_->getLease4(ioaddress4_[2]);
    ASSERT_TRUE(l_returned.get() != nullptr);
    detailCompareLease(leases[2], l_returned);

    reopen(V4);
    l_returned = lmptr_->getLease4(ioaddress4_[1]);
    EXPECT_TRUE(l_returned.get() == nullptr);

    l_returned = lmptr_->getLease4(ioaddress4_[2]);
    ASSERT_TRUE(l_returned.get() != nullptr);
    detailCompareLease(leases[2], l_returned);

    l_returned = lmptr_->getLease4(ioaddress4_[3]);
    ASSERT_TRUE(l_returned.get() != nullptr);
    detailCompareLease(leases[3], l_returned);

    leases[2]->hostname_ = "memfile.example.com.";
    leases[2]->fqdn_rev_ = true;
    ASSERT_NO_THROW(lmptr_->updateLease4(*leases[2]));

    reopen(V4);

    l_returned = lmptr_->getLease4(ioaddress4_[2]);
    ASSERT_TRUE(l_returned.get() != nullptr);
    detailCompareLease(leases[2], l_returned);

    l_returned = lmptr_->getLease4(ioaddress4_[3]);
    ASSERT_TRUE(l_returned.get() != nullptr);
    detailCompareLease(leases[3], l_returned);
}

void
GenericLeaseMgrTest::testLease4InvalidHostname() {
    vector<LeaseMgr::Lease4Ptr> leases;
    createLeases4(leases);

    leases[1]->hostname_.assign(255, 'a');
    ASSERT_TRUE(lmptr_->addLease(*leases[1]));

    LeaseMgr::Lease4Ptr l_returned = lmptr_->getLease4(ioaddress4_[1]);
    detailCompareLease(leases[1], l_returned);

    EXPECT_TRUE(lmptr_->deleteLease(ioaddress4_[1]));

    leases[1]->hostname_.assign(256, 'a');
    EXPECT_THROW(lmptr_->addLease(*leases[1]), DbOperationError);
}

void
GenericLeaseMgrTest::testGetLease4HWAddrSize() {
    vector<LeaseMgr::Lease4Ptr> leases;
    createLeases4(leases);

    for (uint8_t i = 0; i <= HWAddr::MAX_HWADDR_LEN; ++i) {
        leases[1]->hwaddr_->hwaddr_.resize(i, i);
        EXPECT_TRUE(lmptr_->addLease(*leases[1]));
        LeaseMgr::Lease4Collection returned;
        lmptr_->getLease4(*leases[1]->hwaddr_, returned);
        ASSERT_EQ(1, returned.size());
        detailCompareLease(leases[1], *returned.begin());
        (void) lmptr_->deleteLease(leases[1]->addr_);
    }

    leases[1]->hwaddr_->hwaddr_.resize(HWAddr::MAX_HWADDR_LEN + 100, 42);
    EXPECT_THROW(lmptr_->addLease(*leases[1]), kea::server::DbOperationError);
}

void
GenericLeaseMgrTest::testGetLease4HWAddrSubnetId() {
    vector<LeaseMgr::Lease4Ptr> leases;
    createLeases4(leases);
    for (size_t i = 0; i < leases.size(); ++i) {
        EXPECT_TRUE(lmptr_->addLease(*leases[i]));
    }

    LeaseMgr::Lease4Ptr returned = lmptr_->getLease4(*leases[1]->hwaddr_, leases[1]->subnet_id_);
    detailCompareLease(leases[1], returned);

    returned = lmptr_->getLease4(*leases[1]->hwaddr_, leases[1]->subnet_id_ + 1);
    EXPECT_TRUE(returned.get() == nullptr);

    vector<uint8_t> invalid_hwaddr(15, 0x77);
    returned = lmptr_->getLease4(HWAddr(invalid_hwaddr, HTYPE_ETHER),
                                 leases[1]->subnet_id_);
    EXPECT_TRUE(returned.get() == nullptr);

    returned = lmptr_->getLease4(HWAddr(invalid_hwaddr, HTYPE_ETHER),
                                 leases[1]->subnet_id_ + 1);
    EXPECT_TRUE(returned.get() == nullptr);

    EXPECT_TRUE(lmptr_->deleteLease(leases[2]->addr_));
    leases[1]->addr_ = leases[2]->addr_;
    EXPECT_TRUE(lmptr_->addLease(*leases[1]));
    EXPECT_THROW(returned = lmptr_->getLease4(*leases[1]->hwaddr_,
                                              leases[1]->subnet_id_),
                 kea::server::MultipleRecords);


}

void
GenericLeaseMgrTest::testGetLease4HWAddrSubnetIdSize() {
    vector<LeaseMgr::Lease4Ptr> leases; 
    createLeases4(leases);

    for (uint8_t i = 0; i <= HWAddr::MAX_HWADDR_LEN; ++i) {
        leases[1]->hwaddr_->hwaddr_.resize(i, i);
        EXPECT_TRUE(lmptr_->addLease(*leases[1]));
        LeaseMgr::Lease4Ptr returned = lmptr_->getLease4(*leases[1]->hwaddr_,
                                               leases[1]->subnet_id_);
        detailCompareLease(leases[1], returned);
        (void) lmptr_->deleteLease(leases[1]->addr_);
    }

    leases[1]->hwaddr_->hwaddr_.resize(HWAddr::MAX_HWADDR_LEN + 100, 42);
    EXPECT_THROW(lmptr_->addLease(*leases[1]), kea::server::DbOperationError);
}

void
GenericLeaseMgrTest::testGetLease4ClientId2() {
    vector<LeaseMgr::Lease4Ptr> leases; 
    createLeases4(leases);
    for (size_t i = 0; i < leases.size(); ++i) {
        EXPECT_TRUE(lmptr_->addLease(*leases[i]));
    }

    LeaseMgr::Lease4Collection returned;
    lmptr_->getLease4(*leases[1]->client_id_, returned);

    ASSERT_EQ(4, returned.size());

    vector<string> addresses;
    for (auto& lease : returned) {
        addresses.push_back(lease->addr_.str());
    }
    sort(addresses.begin(), addresses.end());
    EXPECT_EQ(straddress4_[1], addresses[0]);
    EXPECT_EQ(straddress4_[4], addresses[1]);
    EXPECT_EQ(straddress4_[5], addresses[2]);
    EXPECT_EQ(straddress4_[6], addresses[3]);

    returned.clear();
    lmptr_->getLease4(*leases[3]->client_id_, returned);
    ASSERT_EQ(1, returned.size());
    detailCompareLease(leases[3], *returned.begin());

    // Check that client-id is NULL
    EXPECT_TRUE(leases[7]->client_id_.get() == nullptr);
    HWAddr tmp(*leases[7]->hwaddr_);
    returned.clear();
    lmptr_->getLease4(tmp, returned);
    ASSERT_EQ(1, returned.size());
    detailCompareLease(leases[7], *returned.begin());

    // Try to get something with invalid client ID
    const uint8_t invalid_data[] = {0, 0, 0};
    ClientId invalid(invalid_data, sizeof(invalid_data));
    returned.clear();
    lmptr_->getLease4(invalid, returned);
    EXPECT_EQ(0, returned.size());
}

void
GenericLeaseMgrTest::testGetLease4ClientIdSize() {
    vector<LeaseMgr::Lease4Ptr> leases; 
    createLeases4(leases);

    int client_id_max = ClientId::MAX_CLIENT_ID_LEN;
    EXPECT_EQ(128, client_id_max);

    int client_id_min = ClientId::MIN_CLIENT_ID_LEN;
    EXPECT_EQ(2, client_id_min); // See RFC2132, section 9.14

    for (uint8_t i = client_id_min; i <= client_id_max; i += 16) {
        vector<uint8_t> clientid_vec(i, i);
        leases[1]->client_id_.reset(new ClientId(clientid_vec));
        EXPECT_TRUE(lmptr_->addLease(*leases[1]));
        LeaseMgr::Lease4Collection returned;
        lmptr_->getLease4(*leases[1]->client_id_, returned);
        ASSERT_TRUE(returned.size() == 1);
        detailCompareLease(leases[1], *returned.begin());
        (void) lmptr_->deleteLease(leases[1]->addr_);
    }
}

void
GenericLeaseMgrTest::testGetLease4ClientIdSubnetId() {
    vector<LeaseMgr::Lease4Ptr> leases; 
    createLeases4(leases);
    for (size_t i = 0; i < leases.size(); ++i) {
        EXPECT_TRUE(lmptr_->addLease(*leases[i]));
    }

    LeaseMgr::Lease4Ptr returned = lmptr_->getLease4(*leases[1]->client_id_,
                                           leases[1]->subnet_id_);
    detailCompareLease(leases[1], returned);

    returned = lmptr_->getLease4(*leases[1]->client_id_,
                                 leases[1]->subnet_id_ + 1);
    EXPECT_TRUE(returned.get() == nullptr);

    const uint8_t invalid_data[] = {0, 0, 0};
    ClientId invalid(invalid_data, sizeof(invalid_data));
    returned = lmptr_->getLease4(invalid, leases[1]->subnet_id_);
    EXPECT_TRUE(returned.get() == nullptr);

    // Try for a match to an unknown hardware address and an unknown
    // subnet ID.
    returned = lmptr_->getLease4(invalid, leases[1]->subnet_id_ + 1);
    EXPECT_TRUE(returned.get() == nullptr);
}


void
GenericLeaseMgrTest::testUpdateLease4() {
    vector<LeaseMgr::Lease4Ptr> leases; 
    createLeases4(leases);
    for (size_t i = 0; i < leases.size(); ++i) {
        EXPECT_TRUE(lmptr_->addLease(*leases[i]));
    }

    ++leases[1]->subnet_id_;
    leases[1]->valid_lft_ *= 2;
    leases[1]->hostname_ = "modified.hostname.";
    leases[1]->fqdn_fwd_ = !leases[1]->fqdn_fwd_;
    leases[1]->fqdn_rev_ = !leases[1]->fqdn_rev_;;
    lmptr_->updateLease4(*leases[1]);

    LeaseMgr::Lease4Ptr l_returned = lmptr_->getLease4(ioaddress4_[1]);
    detailCompareLease(leases[1], l_returned);

    ++leases[1]->subnet_id_;
    leases[1]->cltt_ += 6;
    lmptr_->updateLease4(*leases[1]);

    l_returned = lmptr_->getLease4(ioaddress4_[1]);
    detailCompareLease(leases[1], l_returned);

    lmptr_->updateLease4(*leases[1]);
    l_returned = lmptr_->getLease4(ioaddress4_[1]);
    detailCompareLease(leases[1], l_returned);

    leases[1]->hostname_.assign(256, 'a');
    EXPECT_THROW(lmptr_->updateLease4(*leases[1]), kea::server::DbOperationError);

    lmptr_->deleteLease(ioaddress4_[2]);
    EXPECT_THROW(lmptr_->updateLease4(*leases[2]), kea::server::NoSuchLease);
}


void
GenericLeaseMgrTest::testRecreateLease4() {
    vector<LeaseMgr::Lease4Ptr> leases; 
    createLeases4(leases);
    LeaseMgr::Lease4Ptr lease(new Lease4(*leases[0]));

    EXPECT_TRUE(lmptr_->addLease(*lease));
    lmptr_->commit();

    LeaseMgr::Lease4Ptr l_returned = lmptr_->getLease4(ioaddress4_[0]);
    detailCompareLease(lease, l_returned);

    EXPECT_TRUE(lmptr_->deleteLease(ioaddress4_[0]));
    EXPECT_FALSE(lmptr_->getLease4(ioaddress4_[0]));

    ++lease->subnet_id_;
    ++lease->valid_lft_;
    lease->fqdn_fwd_ = !lease->fqdn_fwd_;
    ASSERT_NE(*lease, *leases[0]);
    EXPECT_TRUE(lmptr_->addLease(*lease));
    lmptr_->commit();

    reopen(V4);

    l_returned = lmptr_->getLease4(ioaddress4_[0]);
    detailCompareLease(lease, l_returned);
}

void
GenericLeaseMgrTest::testVersion(int major, int minor) {
    EXPECT_EQ(major, lmptr_->getVersion().first);
    EXPECT_EQ(minor, lmptr_->getVersion().second);
}

void
GenericLeaseMgrTest::testGetExpiredLeases4() {
    vector<LeaseMgr::Lease4Ptr> leases; 
    createLeases4(leases);
    ASSERT_GE(leases.size(), 6);

    time_t current_time = time(NULL);

    for (size_t i = 0; i < leases.size(); ++i) {
        if (i % 2 == 0) {
            leases[i]->cltt_ = current_time - leases[i]->valid_lft_ - 10 - i;
        } else {
            leases[i]->cltt_ = current_time;
        }
        ASSERT_TRUE(lmptr_->addLease(*leases[i]));
    }

    LeaseMgr::Lease4Collection expired_leases;
    ASSERT_NO_THROW(lmptr_->getExpiredLeases4(1000, expired_leases));
    ASSERT_EQ(static_cast<size_t>(leases.size() / 2), expired_leases.size());

    for (auto lease = expired_leases.rbegin();
         lease != expired_leases.rend(); ++lease) {
        int index = static_cast<int>(std::distance(expired_leases.rbegin(), lease));
        ASSERT_LE(2 * index, leases.size());
        EXPECT_EQ(leases[2 * index]->addr_, (*lease)->addr_);
    }

    current_time = time(NULL);
    expired_leases.clear();

    for (int i = 0; i < leases.size(); ++i) {
        if (i % 2 == 0) {
            leases[i]->cltt_ = current_time - leases[i]->valid_lft_ - 1000 + i;
        } else {
            leases[i]->cltt_ = current_time + 100;
        }
        ASSERT_NO_THROW(lmptr_->updateLease4(*leases[i]));
    }

    ASSERT_NO_THROW(lmptr_->getExpiredLeases4(0, expired_leases));
    ASSERT_EQ(static_cast<size_t>(leases.size() / 2), expired_leases.size());

    for (auto lease = expired_leases.begin();
         lease != expired_leases.end(); ++lease) {
        int index = static_cast<int>(std::distance(expired_leases.begin(), lease));
        ASSERT_LE(2 * index, leases.size());
        EXPECT_EQ(leases[2 * index]->addr_, (*lease)->addr_);
    }


    std::vector<LeaseMgr::Lease4Ptr> expired_leases2;
    ASSERT_NO_THROW(lmptr_->getExpiredLeases4(2, expired_leases2));
    ASSERT_EQ(2, expired_leases2.size());

    for (auto lease = expired_leases2.begin();
         lease != expired_leases2.end(); ++lease) {
        int index = static_cast<int>(std::distance(expired_leases2.begin(), lease));
        ASSERT_LE(2 * index, leases.size());
        EXPECT_EQ(leases[2 * index]->addr_, (*lease)->addr_);
    }

    for (int i = 0; i < expired_leases.size(); ++i) {
        if (i % 2 != 0) {
            expired_leases[i]->state_ = Lease::STATE_EXPIRED_RECLAIMED;
        }
        ASSERT_NO_THROW(lmptr_->updateLease4(*expired_leases[i]));
    }

    expired_leases2.clear();
    ASSERT_NO_THROW(lmptr_->getExpiredLeases4(0, expired_leases2));
    ASSERT_EQ(static_cast<size_t>(expired_leases.size() / 2), expired_leases2.size());

    for (auto lease = expired_leases2.begin();
         lease != expired_leases2.end(); ++lease) {
        int index = static_cast<int>(std::distance(expired_leases2.begin(), lease));
        EXPECT_EQ(expired_leases[2 * index]->addr_, (*lease)->addr_);
    }
}


void
GenericLeaseMgrTest::testDeleteExpiredReclaimedLeases4() {
    vector<LeaseMgr::Lease4Ptr> leases; 
    createLeases4(leases);
    ASSERT_GE(leases.size(), 6);

    time_t current_time = time(NULL);

    for (size_t i = 0; i < leases.size(); ++i) {
        if (i % 2 == 0) {
            leases[i]->cltt_ = current_time - leases[i]->valid_lft_ - i * 10;
            leases[i]->state_ = Lease::STATE_EXPIRED_RECLAIMED;
        } else {
            leases[i]->cltt_ = current_time;
        }
        ASSERT_TRUE(lmptr_->addLease(*leases[i]));
    }

    const uint32_t lease_affinity_time = 15;
    uint64_t deleted_num;
    uint64_t should_delete_num = 0;
    ASSERT_NO_THROW(
        deleted_num = lmptr_->deleteExpiredReclaimedLeases4(lease_affinity_time)
    );

    for (size_t i = 0; i < leases.size(); ++i) {
        LeaseMgr::Lease4Ptr lease = lmptr_->getLease4(leases[i]->addr_);
        if (leases[i]->stateExpiredReclaimed() &&
            ((leases[i]->getExpirationTime() + lease_affinity_time) < current_time)) {
            EXPECT_TRUE(lease.get() == nullptr) << "The following lease should have been"
                " deleted: " << leases[i]->toText();
            ++should_delete_num;
        } else {
            EXPECT_TRUE(lease.get() != nullptr) << "The following lease shouldn't have been"
                " deleted: " << leases[i]->toText();
        }
    }
    EXPECT_EQ(deleted_num, should_delete_num);
    ASSERT_NO_THROW(
        deleted_num = lmptr_->deleteExpiredReclaimedLeases4(lease_affinity_time)
    );
    EXPECT_EQ(0, deleted_num);

    reopen(V4);

    for (size_t i = 0; i < leases.size(); ++i) {
        if (leases[i]->hwaddr_ && leases[i]->hwaddr_->hwaddr_.empty()) {
            continue;
        }
        LeaseMgr::Lease4Ptr lease = lmptr_->getLease4(leases[i]->addr_);

        if (leases[i]->stateExpiredReclaimed() &&
            ((leases[i]->getExpirationTime() + lease_affinity_time) < current_time)) {
            EXPECT_TRUE(lease.get() == nullptr) << "The following lease should have been"
                " deleted: " << leases[i]->toText();

        } else {
            EXPECT_TRUE(lease.get() != nullptr) << "The following lease shouldn't have been"
                " deleted: " << leases[i]->toText();
        }
    }
}


void
GenericLeaseMgrTest::testGetDeclinedLeases4() {
    vector<LeaseMgr::Lease4Ptr> leases; 
    createLeases4(leases);
    ASSERT_GE(leases.size(), 8);

    time_t current_time = time(NULL);
    for (size_t i = 0; i < leases.size(); ++i) {
        if (i < leases.size()/2) {
            leases[i]->decline(1000);
        }

        if (i % 2 == 0) {
            leases[i]->cltt_ = current_time - leases[i]->valid_lft_ - 10 - i;
        } else {
            leases[i]->cltt_ = current_time;
        }

        ASSERT_TRUE(lmptr_->addLease(*leases[i]));
    }

    // The leases are:
    // 0 - declined, expired
    // 1 - declined, not expired
    // 2 - declined, expired
    // 3 - declined, not expired
    // 4 - default, expired
    // 5 - default, not expired
    // 6 - default, expired
    // 7 - default, not expired

    LeaseMgr::Lease4Collection expired_leases;
    ASSERT_NO_THROW(lmptr_->getExpiredLeases4(1000, expired_leases));
    ASSERT_EQ(static_cast<size_t>(leases.size() / 2), expired_leases.size());

    unsigned int declined_state = 0;
    unsigned int default_state = 0;
    for (auto lease = expired_leases.rbegin();
         lease != expired_leases.rend(); ++lease) {
        int index = static_cast<int>(std::distance(expired_leases.rbegin(), lease));
        EXPECT_EQ(leases[2 * index]->addr_, (*lease)->addr_);

        if ((*lease)->state_ == Lease::STATE_DEFAULT) {
            default_state++;
        } else if ((*lease)->state_ == Lease::STATE_DECLINED) {
            declined_state++;
        }
    }

    EXPECT_NE(0, declined_state);
    EXPECT_NE(0, default_state);

    current_time = time(NULL);
    expired_leases.clear();

    leases.clear();
    createLeases4(leases);
    for (int i = 0; i < leases.size(); ++i) {
        if (i >= leases.size()/2) {
            leases[i]->decline(1000);
        }

        if (i % 2 == 0) {
            leases[i]->cltt_ = current_time - leases[i]->valid_lft_ - 1000 + i;
        } else {
            leases[i]->cltt_ = current_time + 100;
        }
        ASSERT_NO_THROW(lmptr_->updateLease4(*leases[i]));
    }

    ASSERT_NO_THROW(lmptr_->getExpiredLeases4(0, expired_leases));
    ASSERT_EQ(static_cast<size_t>(leases.size() / 2), expired_leases.size());

    declined_state = 0;
    default_state = 0;
    for (auto lease = expired_leases.begin();
         lease != expired_leases.end(); ++lease) {
        int index = static_cast<int>(std::distance(expired_leases.begin(), lease));
        EXPECT_EQ(leases[2 * index]->addr_, (*lease)->addr_);

        if ((*lease)->state_ == Lease::STATE_DEFAULT) {
            default_state++;
        } else if ((*lease)->state_ == Lease::STATE_DECLINED) {
            declined_state++;
        }
    }

    EXPECT_NE(0, declined_state);
    EXPECT_NE(0, default_state);

    expired_leases.clear();
    ASSERT_NO_THROW(lmptr_->getExpiredLeases4(2, expired_leases));

    ASSERT_EQ(2, expired_leases.size());

    for (auto lease = expired_leases.begin();
         lease != expired_leases.end(); ++lease) {
        int index = static_cast<int>(std::distance(expired_leases.begin(), lease));
        EXPECT_EQ(leases[2 * index]->addr_, (*lease)->addr_);
    }
}

}; 
};
};
