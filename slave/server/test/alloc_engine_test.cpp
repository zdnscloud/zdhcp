#include <kea/dhcp++/pkt4.h>
#include <kea/server/alloc_engine.h>
#include <kea/server/hosts_in_mem.h>
#include <kea/server/testutil/lease_compare.h>
#include <kea/server/testutil/alloc_engine_test.h>

using namespace std;
using namespace kea::server;
using namespace kea::server::testutil;
using namespace kea::dhcp;

namespace kea {

using HWAddrPtr = std::unique_ptr<HWAddr>;
using ClientIdPtr = std::unique_ptr<ClientId>;
using HostPtr = std::unique_ptr<Host>;

TEST_F(AllocEngine4Test, constructor) {
    std::unique_ptr<AllocEngine> x;
    ASSERT_NO_THROW(x.reset(new AllocEngine(AllocEngine::ALLOC_ITERATIVE, 100, lease_mgr_.get(), host_mgr_.get())));
    EXPECT_NO_THROW(x->getAllocator(Lease::TYPE_V4));
    EXPECT_THROW(x->getAllocator(Lease::TYPE_NA), BadValue);
    EXPECT_THROW(x->getAllocator(Lease::TYPE_TA), BadValue);
    EXPECT_THROW(x->getAllocator(Lease::TYPE_PD), BadValue);
}


TEST_F(AllocEngine4Test, simpleAlloc4) {
    std::unique_ptr<AllocEngine> engine; 
    ASSERT_NO_THROW(engine = createAllocateEngine(100));
    ASSERT_TRUE(engine.get());

    ClientContext4& ctx = getContext(false, IPAddress("192.0.2.100"), "somehost.example.com.");
    Lease4Ptr lease = engine->allocateLease4(ctx);
    EXPECT_FALSE(ctx.old_lease_.get());
    ASSERT_TRUE(lease.get());
    checkLease4(lease);

    Lease4Ptr from_mgr = lease_mgr_->getLease4(lease->addr_);
    ASSERT_TRUE(from_mgr.get());

    detailCompareLease(lease, from_mgr);
    lease_mgr_->deleteLease(lease->addr_);
}

TEST_F(AllocEngine4Test, fakeAlloc4) {
    std::unique_ptr<AllocEngine> engine = createAllocateEngine(100);
    ClientContext4& ctx = getContext(true, IPAddress("0.0.0.0"), "somehost.example.com.");
    Lease4Ptr lease = engine->allocateLease4(ctx);
    EXPECT_FALSE(ctx.old_lease_.get());
    ASSERT_TRUE(lease.get());
    checkLease4(lease, true);

    //discovery stage lease will be insert into db, with a very short life time
    Lease4Ptr from_mgr = lease_mgr_->getLease4(lease->addr_);
    ASSERT_TRUE(from_mgr.get());
    ASSERT_EQ(from_mgr.get()->valid_lft_, LEASE_VALID_DURING_DISCOVER);
    lease_mgr_->deleteLease(lease->addr_);
}

TEST_F(AllocEngine4Test, allocWithValidHint4) {
    std::unique_ptr<AllocEngine> engine = createAllocateEngine(100);
    ClientContext4& ctx = getContext(false, IPAddress("192.0.2.105"), "host.example.com.");
    Lease4Ptr lease = engine->allocateLease4(ctx);
    ASSERT_TRUE(lease.get());
    EXPECT_FALSE(ctx.old_lease_.get());
    EXPECT_EQ(lease->addr_.str(), "192.0.2.105");
    checkLease4(lease);
    Lease4Ptr from_mgr = lease_mgr_->getLease4(lease->addr_);
    ASSERT_TRUE(from_mgr.get());

    detailCompareLease(lease, from_mgr);
    lease_mgr_->deleteLease(lease->addr_);
}


TEST_F(AllocEngine4Test, allocWithUsedHint4) {
    uint8_t hwaddr2_data[] = { 0, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe};
    std::unique_ptr<HWAddr> hwaddr2(new HWAddr(hwaddr2_data, sizeof(hwaddr2_data), HTYPE_ETHER));
    uint8_t clientid2[] = { 8, 7, 6, 5, 4, 3, 2, 1 };
    time_t now = time(NULL);
    IPAddress used_addr("192.0.2.106");
    Lease4Ptr used(new Lease4(used_addr, std::move(hwaddr2), clientid2, sizeof(clientid2), 1, 2, 3, now, subnet_->getID()));
    ASSERT_TRUE(lease_mgr_->addLease(*used));

    std::unique_ptr<AllocEngine> engine = createAllocateEngine(100);
    ClientContext4& ctx = getContext(true, IPAddress("192.0.2.106"));
    Lease4Ptr lease = engine->allocateLease4(ctx);
    EXPECT_FALSE(ctx.old_lease_.get());
    ASSERT_TRUE(lease.get());
    EXPECT_NE(used_addr.str(), lease->addr_.str());

    ASSERT_EQ(lease->valid_lft_, LEASE_VALID_DURING_DISCOVER);
    checkLease4(lease, true);

    lease_mgr_->deleteLease(lease->addr_);
    lease_mgr_->deleteLease(used_addr);
    Lease4Ptr from_mgr = lease_mgr_->getLease4(lease->addr_);
    ASSERT_FALSE(from_mgr.get());
}

TEST_F(AllocEngine4Test, allocBogusHint4) {
    std::unique_ptr<AllocEngine> engine = createAllocateEngine(100);
    ClientContext4& ctx = getContext(true, IPAddress("10.1.1.1"));
    Lease4Ptr lease = engine->allocateLease4(ctx);
    ASSERT_TRUE(lease.get());
    EXPECT_FALSE(ctx.old_lease_.get());
    EXPECT_NE("10.1.1.1", lease->addr_.str());
    checkLease4(lease, true);

    Lease4Ptr from_mgr = lease_mgr_->getLease4(lease->addr_);
    EXPECT_TRUE(from_mgr.get());
    lease_mgr_->deleteLease(from_mgr->addr_);
}

TEST_F(AllocEngine4Test, allocateLease4Nulls) {
    std::unique_ptr<AllocEngine> engine = createAllocateEngine(100);
    ClientContext4& ctx = getContext(false, IPAddress("0.0.0.0"));
    //request without hint return null
    Lease4Ptr lease = engine->allocateLease4(ctx);
    EXPECT_FALSE(lease.get());

    ClientContext4 ctx2(nullptr, clientid_.get(), nullptr, IPAddress("0.0.0.0"), "", false);
    std::unique_ptr<Pkt4> pkt(new Pkt4(DHCPREQUEST, 1234));
    ctx2.query_ = pkt.get();
    lease = engine->allocateLease4(ctx2);
    EXPECT_FALSE(lease.get());
    EXPECT_FALSE(ctx2.old_lease_.get());

    clientid_.reset();
    ClientContext4& ctx3 = getContext(false, IPAddress("192.0.2.106"));
    lease = engine->allocateLease4(ctx3);
    ASSERT_TRUE(lease.get());
    EXPECT_FALSE(ctx3.old_lease_.get());
    checkLease4(lease);
    Lease4Ptr from_mgr = lease_mgr_->getLease4(lease->addr_);
    ASSERT_TRUE(from_mgr.get());

    detailCompareLease(lease, from_mgr);
    lease_mgr_->deleteLease(from_mgr->addr_);
}

TEST_F(AllocEngine4Test, smallPool4) {
    std::unique_ptr<AllocEngine> engine = createAllocateEngine(100);

    IPAddress addr("192.0.2.17");
    subnet_.reset(new Subnet4(IPAddress("192.0.2.0"), 24, 1, 2, 3));
    subnet_->addPool(std::unique_ptr<Pool4>(new Pool4(addr, addr)));

    ClientContext4& ctx = getContext(true, addr);
    Lease4Ptr lease = engine->allocateLease4(ctx);
    ASSERT_TRUE(lease.get());
    EXPECT_FALSE(ctx.old_lease_.get());
    EXPECT_EQ("192.0.2.17", lease->addr_.str());
    checkLease4(lease, true);
    Lease4Ptr from_mgr = lease_mgr_->getLease4(lease->addr_);
    ASSERT_TRUE(from_mgr.get());
    detailCompareLease(lease, from_mgr);

    uint8_t hwaddr2_data[] = { 0, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe};
    hwaddr_.reset(new HWAddr(hwaddr2_data, sizeof(hwaddr2_data), HTYPE_ETHER));
    uint8_t clientid2_data[] = { 8, 7, 6, 5, 4, 3, 2, 1 };
    clientid_.reset(new ClientId(&clientid2_data[0], sizeof(clientid2_data)));
    time_t now = time(NULL);
    ClientContext4& ctx2 = getContext(true, IPAddress("0.0.0.0"));
    Lease4Ptr lease2 = engine->allocateLease4(ctx2);
    EXPECT_FALSE(lease2.get());
    EXPECT_FALSE(ctx.old_lease_.get());

    lease_mgr_->deleteLease(addr);
}

TEST_F(AllocEngine4Test, discoverReuseExpiredLease4) {
    std::unique_ptr<AllocEngine> engine = createAllocateEngine(100);

    IPAddress addr("192.0.2.15");
    subnet_.reset(new Subnet4(IPAddress("192.0.2.0"), 24, 1, 2, 3));
    subnet_->addPool(std::unique_ptr<Pool4>(new Pool4(addr, addr)));

    uint8_t hwaddr2_data[] = { 0, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe};
    std::unique_ptr<HWAddr> hwaddr2(new HWAddr(hwaddr2_data, sizeof(hwaddr2_data), HTYPE_ETHER));
    uint8_t clientid2[] = { 8, 7, 6, 5, 4, 3, 2, 1 };
    time_t now = time(NULL) - 500;
    Lease4Ptr lease(new Lease4(addr, std::move(hwaddr2), clientid2, sizeof(clientid2), 495, 100, 200, now, subnet_->getID()));

    Lease4 original_lease(*lease);
    ASSERT_TRUE(lease->expired());
    ASSERT_TRUE(lease_mgr_->addLease(*lease));

    ClientContext4& ctx1 = getContext(true, IPAddress("0.0.0.0"));
    lease = engine->allocateLease4(ctx1);
    ASSERT_TRUE(lease.get());
    EXPECT_EQ(addr, lease->addr_);
    ASSERT_TRUE(ctx1.old_lease_.get());
    EXPECT_EQ(original_lease.addr_, ctx1.old_lease_->addr_);
    checkLease4(lease);

    ClientContext4& ctx2 = getContext(true, addr);
    lease = engine->allocateLease4(ctx2);
    ASSERT_TRUE(lease.get());
    EXPECT_EQ(addr, lease->addr_);
    ASSERT_TRUE(ctx2.old_lease_.get());
    EXPECT_EQ(ctx2.old_lease_->addr_, original_lease.addr_);
    lease_mgr_->deleteLease(addr);
}

TEST_F(AllocEngine4Test, requestReuseExpiredLease4) {
    std::unique_ptr<AllocEngine> engine = createAllocateEngine(100);
    IPAddress addr("192.0.2.105");
    uint8_t hwaddr2_data[] = { 0, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe};
    std::unique_ptr<HWAddr> hwaddr2(new HWAddr(hwaddr2_data, sizeof(hwaddr2_data), HTYPE_ETHER));
    uint8_t clientid2[] = { 8, 7, 6, 5, 4, 3, 2, 1 };
    time_t now = time(NULL) - 500;
    Lease4Ptr lease(new Lease4(addr, std::move(hwaddr2), clientid2, sizeof(clientid2), 495, 100, 200, now, subnet_->getID()));

    Lease4 original_lease(*lease);
    ASSERT_TRUE(lease->expired());
    ASSERT_TRUE(lease_mgr_->addLease(*lease));

    ClientContext4& ctx = getContext(false, addr);
    lease = engine->allocateLease4(ctx);
    ASSERT_TRUE(lease.get());
    EXPECT_EQ(addr, lease->addr_);
    Lease4Ptr from_mgr = lease_mgr_->getLease4(addr);
    ASSERT_TRUE(from_mgr.get());

    detailCompareLease(lease, from_mgr);

    ASSERT_TRUE(ctx.old_lease_.get());
    lease_mgr_->deleteLease(addr);
}

TEST_F(AllocEngine4Test, discoverReuseDeclinedLease4) {
    std::unique_ptr<AllocEngine> engine = createAllocateEngine(100);
    ClientContext4& ctx = getContext(true, IPAddress("10.1.1.1"));

    IPAddress addr("192.0.2.15");
    subnet_.reset(new Subnet4(IPAddress("192.0.2.0"), 24, 1, 2, 3));
    subnet_->addPool(std::unique_ptr<Pool4>(new Pool4(addr, addr)));

    Lease4Ptr declined = generateDeclinedLease("192.0.2.15", 100, -10);

    Lease4Ptr assigned;
    testReuseLease4(*engine, declined, "0.0.0.0", true, SHOULD_PASS, assigned);

    ASSERT_TRUE(assigned.get());
    EXPECT_EQ(addr, assigned->addr_);

    testReuseLease4(*engine, declined, "192.0.2.15", true, SHOULD_PASS, assigned);

    ASSERT_TRUE(assigned.get());
    EXPECT_EQ(addr, assigned->addr_);
    lease_mgr_->deleteLease(addr);
}

TEST_F(AllocEngine4Test, requestReuseDeclinedLease4) {
    std::unique_ptr<AllocEngine> engine = createAllocateEngine(100);

    IPAddress addr("192.0.2.15");
    subnet_.reset(new Subnet4(IPAddress("192.0.2.0"), 24, 1, 2, 3));
    subnet_->addPool(std::unique_ptr<Pool4>(new Pool4(addr, addr)));

    Lease4Ptr declined = generateDeclinedLease("192.0.2.15", 100, -10);
    Lease4Ptr assigned;
    testReuseLease4(*engine, declined, "192.0.2.15", false, SHOULD_PASS, assigned);
    ASSERT_TRUE(assigned.get());
    EXPECT_EQ(addr, assigned->addr_);
    Lease4Ptr from_mgr = lease_mgr_->getLease4(addr);
    ASSERT_TRUE(from_mgr.get());
    detailCompareLease(assigned, from_mgr);
    lease_mgr_->deleteLease(addr);
}

// This test checks that the Allocation Engine correcly identifies the
// existing client's lease in the lease database, using the client
// identifier and HW address.
TEST_F(AllocEngine4Test, identifyClientLease) {
    IPAddress addr("192.0.2.101");
    Lease4Ptr lease(new Lease4(addr, 
                               HWAddrPtr(new HWAddr(*hwaddr_)), 
                               ClientIdPtr(new ClientId(*clientid_)), 100, 30, 60, time(nullptr), subnet_->getID()));
    lease_mgr_->addLease(*lease);

    std::unique_ptr<AllocEngine> engine = createAllocateEngine(100);
    ClientContext4& ctx = getContext(true, IPAddress("0.0.0.0"));
    Lease4Ptr identified_lease = engine->allocateLease4(ctx);
    ASSERT_TRUE(identified_lease.get());
    EXPECT_EQ("192.0.2.101", identified_lease->addr_.str());

    uint8_t mac[] = { 0, 2, 22, 33, 44, 55};
    hwaddr_.reset(new HWAddr(mac, sizeof(mac), HTYPE_ETHER));
    ClientContext4& ctx2 = getContext(true, IPAddress("0.0.0.0"));
    identified_lease = engine->allocateLease4(ctx2);
    ASSERT_TRUE(identified_lease.get());
    EXPECT_EQ("192.0.2.101", identified_lease->addr_.str());

    restoreHWAddr();
    clientid_.reset(new ClientId(vector<uint8_t>(8, 0x45))); 
    ClientContext4& ctx3 = getContext(true, IPAddress("0.0.0.0"));
    identified_lease = engine->allocateLease4(ctx3);
    ASSERT_TRUE(identified_lease.get());
    EXPECT_NE(identified_lease->addr_.str(), "192.0.2.101");
    lease_mgr_->deleteLease(identified_lease->addr_);

    clientid_.reset(nullptr); 
    ClientContext4& ctx4 = getContext(true, IPAddress("0.0.0.0"));
    identified_lease = engine->allocateLease4(ctx4);
    ASSERT_TRUE(identified_lease.get());
    EXPECT_EQ("192.0.2.101", identified_lease->addr_.str());

    hwaddr_.reset(new HWAddr(mac, sizeof(mac), HTYPE_ETHER));
    clientid_.reset(nullptr); 
    ClientContext4& ctx5 = getContext(true, IPAddress("0.0.0.0"));
    identified_lease = engine->allocateLease4(ctx5);
    ASSERT_TRUE(identified_lease.get());
    EXPECT_NE(identified_lease->addr_.str(), "192.0.2.101");
    lease_mgr_->deleteLease(identified_lease->addr_);

    lease->client_id_.reset();
    ASSERT_NO_THROW(lease_mgr_->updateLease4(*lease));

    restoreHWAddr();
    restoreClientId();
    ClientContext4& ctx6 = getContext(true, IPAddress("0.0.0.0"));
    identified_lease = engine->allocateLease4(ctx6);
    ASSERT_TRUE(identified_lease.get());
    EXPECT_EQ("192.0.2.101", identified_lease->addr_.str());

    clientid_.reset();
    ClientContext4& ctx7 = getContext(true, IPAddress("0.0.0.0"));
    identified_lease = engine->allocateLease4(ctx7);
    ASSERT_TRUE(identified_lease.get());
    EXPECT_EQ("192.0.2.101", identified_lease->addr_.str());

    hwaddr_.reset(new HWAddr(mac, sizeof(mac), HTYPE_ETHER));
    restoreClientId();
    ClientContext4& ctx8 = getContext(true, IPAddress("0.0.0.0"));
    identified_lease = engine->allocateLease4(ctx8);
    ASSERT_TRUE(identified_lease.get());
    EXPECT_NE(identified_lease->addr_.str(), "192.0.2.101");
    lease_mgr_->deleteLease(identified_lease->addr_);
    lease_mgr_->deleteLease(addr);
}

TEST_F(AllocEngine4Test, requestOtherClientLease) {
    IPAddress addr1("192.0.2.101");
    Lease4Ptr lease1(new Lease4(addr1, 
                               HWAddrPtr(new HWAddr(*hwaddr_)), 
                               ClientIdPtr(new ClientId(*clientid_)), 100, 30, 60, time(nullptr), subnet_->getID()));

    uint8_t mac[] = { 0, 2, 22, 33, 44, 55};
    IPAddress addr2("192.0.2.102");
    Lease4Ptr lease2(new Lease4(addr2, 
                               HWAddrPtr(new HWAddr(mac, sizeof(mac), HTYPE_ETHER)), 
                               ClientIdPtr(new ClientId(vector<uint8_t>(8, 0x45))), 100, 30, 60, time(nullptr), subnet_->getID()));

    lease_mgr_->addLease(*lease1);
    lease_mgr_->addLease(*lease2);

    std::unique_ptr<AllocEngine> engine = createAllocateEngine(100);
    ClientContext4& ctx = getContext(false, addr2);
    Lease4Ptr new_lease = engine->allocateLease4(ctx);
    ASSERT_FALSE(new_lease.get());

    ctx.fake_allocation_ = true;
    new_lease = engine->allocateLease4(ctx);
    ASSERT_TRUE(new_lease.get());
    EXPECT_EQ("192.0.2.101", new_lease->addr_.str());
    lease_mgr_->deleteLease(addr1);
    lease_mgr_->deleteLease(addr2);
    lease_mgr_->deleteLease(new_lease->addr_);
}

// This test checks the behavior of the allocation engine in the following
// scenario:
// - Client has no lease in the database.
// - Client has a reservation.
// - Client sends DHCPREQUEST with a requested IP address
// - Server returns DHCPNAK when requested IP address is different than
//   the reserved address. Note that the allocation engine returns NULL
//   to indicate to the server that it should send DHCPNAK.
// - Server allocates a reserved address to the client when the client requests
// this address using requested IP address option.
TEST_F(AllocEngine4Test, reservedAddressHint) {
    host_mgr_.reset(new HostsInMem());
    host_mgr_->add(HostPtr(new Host(&hwaddr_->hwaddr_[0], hwaddr_->hwaddr_.size(),
                          Host::IDENT_HWADDR, subnet_->getID(),IPAddress("192.0.2.123"))));

    std::unique_ptr<AllocEngine> engine = createAllocateEngine(100);

    IPAddress addr1("192.0.2.234");
    ClientContext4& ctx = getContext(false, addr1);
    engine->findReservation(ctx);
    Lease4Ptr lease = engine->allocateLease4(ctx);
    ASSERT_FALSE(lease.get());
    ASSERT_FALSE(ctx.old_lease_.get());

    IPAddress addr2("192.0.2.123");
    ClientContext4& ctx2 = getContext(false, addr2);
    engine->findReservation(ctx2);
    lease = engine->allocateLease4(ctx2);
    ASSERT_TRUE(lease.get());
    EXPECT_EQ("192.0.2.123", lease->addr_.str());

    // Make sure that the lease has been committed to the lease database.
    // And that the committed lease is equal to the one returned.
    Lease4Ptr from_mgr = lease_mgr_->getLease4(lease->addr_);
    ASSERT_TRUE(from_mgr.get());
    detailCompareLease(lease, from_mgr);

    EXPECT_FALSE(ctx2.old_lease_.get());
    lease_mgr_->deleteLease(addr1);
    lease_mgr_->deleteLease(addr2);
}

// This test checks the behavior of the allocation engine in the following
// scenario:
// - Client has no lease in the database.
// - Client has a reservation.
// - Client sends DHCPDISCOVER with a requested IP address as a hint.
// - Server offers a reserved address, even though it is different than the
// requested address.
TEST_F(AllocEngine4Test, reservedAddressHintFakeAllocation) {
    host_mgr_.reset(new HostsInMem());
    host_mgr_->add(HostPtr(new Host(&hwaddr_->hwaddr_[0], hwaddr_->hwaddr_.size(),
                          Host::IDENT_HWADDR, subnet_->getID(),IPAddress("192.0.2.123"))));


    std::unique_ptr<AllocEngine> engine = createAllocateEngine(100);
    IPAddress addr("192.0.2.234");
    ClientContext4& ctx = getContext(true, addr);
    engine->findReservation(ctx);
    Lease4Ptr lease = engine->allocateLease4(ctx);
    ASSERT_TRUE(lease.get());
    EXPECT_EQ("192.0.2.123", lease->addr_.str());
    EXPECT_FALSE(ctx.old_lease_.get());
    lease_mgr_->deleteLease(addr);
    lease_mgr_->deleteLease(IPAddress("192.0.2.123"));
}
// This test checks that the behavior of the allocation engine in the following
// scenario:
// - Client has a lease for the address from the dynamic pool in the database.
// - Client has a reservation for a different address than the one for which
// the client has a lease.
// - Client sends DHCPREQUEST, asking for the reserved address (as it has been
// offered to it when it sent DHCPDISCOVER).
// - Server allocates a reserved address and removes the lease for the address
// previously allocated to the client.
TEST_F(AllocEngine4Test, reservedAddressExistingLease) {
    host_mgr_.reset(new HostsInMem());
    host_mgr_->add(HostPtr(new Host(&hwaddr_->hwaddr_[0], hwaddr_->hwaddr_.size(),
                          Host::IDENT_HWADDR, subnet_->getID(),IPAddress("192.0.2.123"))));

    IPAddress addr("192.0.2.101");
    Lease4Ptr lease(new Lease4(addr, 
                               HWAddrPtr(new HWAddr(*hwaddr_)), 
                               ClientIdPtr(new ClientId(*clientid_)), 100, 30, 60, time(nullptr), subnet_->getID()));
    lease_mgr_->addLease(*lease);

    std::unique_ptr<AllocEngine> engine = createAllocateEngine(100);
    ClientContext4& ctx = getContext(false, IPAddress("192.0.2.123"));
    engine->findReservation(ctx);
    Lease4Ptr allocated_lease = engine->allocateLease4(ctx);
    ASSERT_TRUE(allocated_lease.get());
    EXPECT_EQ("192.0.2.123", allocated_lease->addr_.str());

    Lease4Ptr from_mgr = lease_mgr_->getLease4(allocated_lease->addr_);
    ASSERT_TRUE(from_mgr.get());
    detailCompareLease(allocated_lease, from_mgr);

    ASSERT_TRUE(ctx.old_lease_.get());
    EXPECT_EQ("192.0.2.101", ctx.old_lease_->addr_.str());
    detailCompareLease(ctx.old_lease_, lease);

    lease_mgr_->deleteLease(addr);
    lease_mgr_->deleteLease(IPAddress("192.0.2.123"));
}

// This test checks that the behavior of the allocation engine in the following
// scenario:
// - Client A has a lease in the database.
// - Client B has a reservation for the address in use by client A.
// - Client B sends a DHCPREQUEST requesting the allocation of the reserved
// lease (in use by client A).
// - Server determines that the reserved address is in use by a different client
// and returns DHCPNAK to client B.
TEST_F(AllocEngine4Test, reservedAddressHijacked) {
    host_mgr_.reset(new HostsInMem());
    host_mgr_->add(HostPtr(new Host(&hwaddr_->hwaddr_[0], hwaddr_->hwaddr_.size(),
                          Host::IDENT_HWADDR, subnet_->getID(),IPAddress("192.0.2.123"))));

    IPAddress addr("192.0.2.123");
    uint8_t mac[] = { 0, 2, 22, 33, 44, 55};
    Lease4Ptr lease(new Lease4(addr, 
                               HWAddrPtr(new HWAddr(mac, sizeof(mac), HTYPE_ETHER)), 
                               ClientIdPtr(), 100, 30, 60, time(nullptr), subnet_->getID()));
    lease_mgr_->addLease(*lease);
 

    std::unique_ptr<AllocEngine> engine = createAllocateEngine(100);
    ClientContext4& ctx = getContext(false, addr);
    engine->findReservation(ctx);
    Lease4Ptr allocated_lease = engine->allocateLease4(ctx);
    ASSERT_FALSE(allocated_lease.get());
    EXPECT_FALSE(ctx.old_lease_.get());

    Lease4Ptr from_mgr = lease_mgr_->getLease4(lease->addr_);
    ASSERT_TRUE(from_mgr.get());
    detailCompareLease(lease, from_mgr);


    ClientContext4& ctx2 = getContext(false, IPAddress("0.0.0.0"));
    engine->findReservation(ctx2);
    allocated_lease = engine->allocateLease4(ctx2);
    ASSERT_FALSE(allocated_lease.get());
    EXPECT_FALSE(ctx2.old_lease_.get());

    lease_mgr_->deleteLease(addr);
}

// This test checks that the behavior of the allocation engine in the following
// scenario:
// - Client A has a lease in the database.
// - Client B has a reservation for the address in use by client A.
// - Client B sends a DHCPDISCOVER.
// - Server determines that the reserved address is in use by a different client
//   so it offers an address from the dynamic pool.
TEST_F(AllocEngine4Test, reservedAddressHijackedFakeAllocation) {
    host_mgr_.reset(new HostsInMem());
    host_mgr_->add(HostPtr(new Host(&hwaddr_->hwaddr_[0], hwaddr_->hwaddr_.size(),
                          Host::IDENT_HWADDR, subnet_->getID(),IPAddress("192.0.2.123"))));

    IPAddress addr("192.0.2.123");
    uint8_t mac[] = { 0, 2, 22, 33, 44, 55};
    Lease4Ptr lease(new Lease4(addr, 
                               HWAddrPtr(new HWAddr(mac, sizeof(mac), HTYPE_ETHER)), 
                               ClientIdPtr(), 100, 30, 60, time(nullptr), subnet_->getID()));
    lease_mgr_->addLease(*lease);
 
    std::unique_ptr<AllocEngine> engine = createAllocateEngine(100);
    ClientContext4& ctx = getContext(true, addr);
    engine->findReservation(ctx);
    Lease4Ptr allocated_lease = engine->allocateLease4(ctx);

    ASSERT_TRUE(allocated_lease.get());
    EXPECT_FALSE(ctx.old_lease_.get());
    EXPECT_NE(allocated_lease->addr_.str(), "192.0.2.123");
    EXPECT_TRUE(subnet_->inPool(Lease::TYPE_V4, allocated_lease->addr_));
    lease_mgr_->deleteLease(allocated_lease->addr_);

    // Do the same test. But, this time do not specify any address to be
    // allocated.
    ClientContext4& ctx2 = getContext(true, IPAddress("0.0.0.0"));
    engine->findReservation(ctx2);
    allocated_lease = engine->allocateLease4(ctx2);
    ASSERT_TRUE(allocated_lease.get());
    EXPECT_NE(allocated_lease->addr_.str(), "192.0.2.123");
    EXPECT_TRUE(subnet_->inPool(Lease::TYPE_V4, allocated_lease->addr_));
    EXPECT_FALSE(ctx2.old_lease_.get());
    lease_mgr_->deleteLease(allocated_lease->addr_);
    lease_mgr_->deleteLease(addr);
}

// This test checks that the behavior of the allocation engine in the following
// scenario:
// - Client has a reservation.
// - Client has a lease in the database for a different address than reserved.
// - Client sends a DHCPREQUEST and asks for a different address than reserved,
// and different than it has in a database.
// - Server doesn't allocate the reserved address to the client because the
// client asked for the different address.
//
// Note that in this case the client should get the DHCPNAK and should fall back
// to the DHCPDISCOVER.
TEST_F(AllocEngine4Test, reservedAddressExistingLeaseInvalidHint) {
    host_mgr_.reset(new HostsInMem());
    host_mgr_->add(HostPtr(new Host(&hwaddr_->hwaddr_[0], hwaddr_->hwaddr_.size(),
                          Host::IDENT_HWADDR, subnet_->getID(),IPAddress("192.0.2.123"))));

    IPAddress addr("192.0.2.101");
    Lease4Ptr lease(new Lease4(addr, 
                               HWAddrPtr(new HWAddr(*hwaddr_)), 
                               ClientIdPtr(), 100, 30, 60, time(nullptr), subnet_->getID()));
    lease_mgr_->addLease(*lease);
 
    std::unique_ptr<AllocEngine> engine = createAllocateEngine(100);
    ClientContext4& ctx = getContext(false, IPAddress("192.0.2.102"));
    engine->findReservation(ctx);
    Lease4Ptr allocated_lease = engine->allocateLease4(ctx);
    ASSERT_FALSE(allocated_lease.get());
    ASSERT_FALSE(ctx.old_lease_.get());

    ClientContext4& ctx2 = getContext(false, addr);
    engine->findReservation(ctx2);
    allocated_lease = engine->allocateLease4(ctx2);
    ASSERT_FALSE(allocated_lease.get());
    ASSERT_FALSE(ctx2.old_lease_.get());

    lease_mgr_->deleteLease(addr);
}

// This test checks that the behavior of the allocation engine in the following
// scenario:
// - Client has a lease in the database.
// - Client has a reservation for a different address than the one for which it
// has a lease.
// - Client sends a DHCPDISCOVER and asks for a different address than reserved
//   and different from which it has a lease for.
// - Server ignores the client's hint and offers a reserved address.
TEST_F(AllocEngine4Test, reservedAddressExistingLeaseFakeAllocation) {
    host_mgr_.reset(new HostsInMem());
    host_mgr_->add(HostPtr(new Host(&hwaddr_->hwaddr_[0], hwaddr_->hwaddr_.size(),
                          Host::IDENT_HWADDR, subnet_->getID(),IPAddress("192.0.2.123"))));

    IPAddress addr("192.0.2.101");
    Lease4Ptr lease(new Lease4(addr, 
                               HWAddrPtr(new HWAddr(*hwaddr_)), 
                               ClientIdPtr(new ClientId(*clientid_)), 100, 30, 60, time(nullptr), subnet_->getID()));
    lease_mgr_->addLease(*lease);
 

    std::unique_ptr<AllocEngine> engine = createAllocateEngine(100);
    ClientContext4& ctx = getContext(true, IPAddress("192.0.2.102"));
    engine->findReservation(ctx);
    Lease4Ptr allocated_lease = engine->allocateLease4(ctx);
    ASSERT_TRUE(allocated_lease.get());
    EXPECT_EQ("192.0.2.123", allocated_lease->addr_.str());
    ASSERT_TRUE(ctx.old_lease_.get());
    EXPECT_EQ("192.0.2.101", ctx.old_lease_->addr_.str());
    lease_mgr_->deleteLease(IPAddress("192.0.2.123"));

    ClientContext4& ctx2 = getContext(true, addr);
    engine->findReservation(ctx2);
    allocated_lease = engine->allocateLease4(ctx2);
    ASSERT_TRUE(allocated_lease.get());
    EXPECT_EQ("192.0.2.123", allocated_lease->addr_.str());
    ASSERT_TRUE(ctx2.old_lease_.get());
    EXPECT_EQ("192.0.2.101", ctx2.old_lease_->addr_.str());

    lease_mgr_->deleteLease(addr);
    lease_mgr_->deleteLease(IPAddress("192.0.2.123"));
}

// This test checks that the behavior of the allocation engine in the following
// scenario:
// - Client has a reservation.
// - Client has a lease for a different address than reserved.
// - Client sends a DHCPREQUEST to allocate a lease.
// - The server determines that the client has a reservation for the
// different address than it is currently using and should assign
// a reserved address and remove the previous lease.
TEST_F(AllocEngine4Test, reservedAddressExistingLeaseNoHint) {
}

// This test checks that the behavior of the allocation engine in the following
// scenario:
// - Client has a reservation.
// - Client has a lease for a different address than reserved.
// - Client sends a DHCPDISCOVER with no hint.
// - Server determines that there is a reservation for the client and that
//   the reserved address should be offered when the client sends a
//   DHCPDISCOVER.
TEST_F(AllocEngine4Test, reservedAddressExistingLeaseNoHintFakeAllocation) {
    host_mgr_.reset(new HostsInMem());
    host_mgr_->add(HostPtr(new Host(&hwaddr_->hwaddr_[0], hwaddr_->hwaddr_.size(),
                          Host::IDENT_HWADDR, subnet_->getID(),IPAddress("192.0.2.123"))));

    IPAddress addr("192.0.2.101");
    Lease4Ptr lease(new Lease4(addr, 
                               HWAddrPtr(new HWAddr(*hwaddr_)), 
                               ClientIdPtr(new ClientId(*clientid_)), 100, 30, 60, time(nullptr), subnet_->getID()));
    lease_mgr_->addLease(*lease);

    std::unique_ptr<AllocEngine> engine = createAllocateEngine(100);
    ClientContext4& ctx = getContext(true, IPAddress("0.0.0.0"));
    engine->findReservation(ctx);
    Lease4Ptr allocated_lease = engine->allocateLease4(ctx);
    ASSERT_TRUE(allocated_lease.get());
    EXPECT_EQ("192.0.2.123", allocated_lease->addr_.str());
    ASSERT_TRUE(ctx.old_lease_.get());
    Lease4Ptr from_mgr = lease_mgr_->getLease4(lease->addr_);
    ASSERT_TRUE(from_mgr.get());
    detailCompareLease(lease, from_mgr);

    lease_mgr_->deleteLease(IPAddress("192.0.2.101"));
    lease_mgr_->deleteLease(IPAddress("192.0.2.123"));
}

/*
// This test checks that the behavior of the allocation engine in the following
// scenario:
// - Client A has a lease for the address.
// - Client B has a reservation for the same address that the Client A is using.
// - Client B requests allocation of the reserved address.
// - Server returns DHCPNAK to the client to indicate that the requested address
// can't be allocated.
// - Client A renews the lease.
// - Server determines that the lease that the Client A is trying to renew
// is for the address reserved for Client B. Therefore, the server returns
// DHCPNAK to force the client to return to the server dkeaovery.
// - The Client A sends DHCPDISCOVER.
// - The server offers an address to the Client A, which is different than
// the address reserved for Client B.
// - The Client A requests allocation of the offered address.
// - The server allocates the new address to Client A.
// - The Client B sends DHCPDISCOVER to the server.
// - The server offers a reserved address to the Client B.
// - The Client B requests the offered address.
// - The server allocates the reserved address to the Client B.
TEST_F(AllocEngine4Test, reservedAddressConflictResolution) {
    uint8_t mac[] = { 0, 2, 22, 33, 44, 55};
    host_mgr_.reset(new HostsInMem());
    host_mgr_->add(HostPtr(new Host(mac, sizeof(mac),
                          Host::IDENT_HWADDR, subnet_->getID(),IPAddress("192.0.2.101"))));

    IPAddress addr("192.0.2.101");
    Lease4Ptr lease(new Lease4(addr, 
                               HWAddrPtr(new HWAddr(*hwaddr_)), 
                               ClientIdPtr(new ClientId(*clientid_)), 100, 30, 60, time(nullptr), subnet_->getID()));
    lease_mgr_->addLease(*lease);
 
    std::unique_ptr<AllocEngine> engine = createAllocateEngine(100);
    clientid_.reset(nullptr);
    hwaddr_.reset(new HWAddr(mac, sizeof(mac), HTYPE_ETHER));
    ClientContext4& ctx = getContext(false, addr);
    engine->findReservation(ctx);
    Lease4Ptr offered_lease = engine->allocateLease4(ctx);
    ASSERT_FALSE(offered_lease.get());

    restoreClientId();
    restoreHWAddr();
    ClientContext4& ctx2 = getContext(false, addr);
    engine->findReservation(ctx2);
    offered_lease = engine->allocateLease4(ctx2);
    ASSERT_FALSE(offered_lease.get());
    ASSERT_FALSE(ctx2.old_lease_.get());

    ClientContext4& ctx3 = getContext(true, addr);
    engine->findReservation(ctx3);
    offered_lease = engine->allocateLease4(ctx3);
    ASSERT_TRUE(offered_lease.get());
    EXPECT_NE(offered_lease->addr_.str(), "192.0.2.101");

    ClientContext4& ctx4 = getContext(false, offered_lease->addr_);
    engine->findReservation(ctx4);
    offered_lease = engine->allocateLease4(ctx4);
    ASSERT_TRUE(offered_lease.get());
    EXPECT_NE(offered_lease->addr_.str(), "192.0.2.101");
    IPAddress offered_addr(offered_lease->addr_);

    clientid_.reset(nullptr);
    hwaddr_.reset(new HWAddr(mac, sizeof(mac), HTYPE_ETHER));
    ClientContext4& ctx5 = getContext(true, IPAddress("0.0.0.0"));
    engine->findReservation(ctx5);
    offered_lease = engine->allocateLease4(ctx5);
    ASSERT_TRUE(offered_lease.get());
    EXPECT_EQ(offered_lease->addr_.str(), "192.0.2.101");

    ClientContext4& ctx6 = getContext(false, IPAddress("192.0.2.101"));
    engine->findReservation(ctx6);
    offered_lease = engine->allocateLease4(ctx6);
    ASSERT_TRUE(offered_lease.get());
    EXPECT_EQ(offered_lease->addr_.str(), "192.0.2.101");

    lease_mgr_->deleteLease(IPAddress("192.0.2.101"));
    lease_mgr_->deleteLease(offered_addr);
}
*/

// This test checks that the address is not assigned from the dynamic
// pool if it has been reserved for another client.
TEST_F(AllocEngine4Test, reservedAddressVsDynamicPool) {
    host_mgr_.reset(new HostsInMem());
    uint8_t mac[] = { 0, 2, 22, 33, 44, 55};
    host_mgr_->add(HostPtr(new Host(mac, sizeof(mac),
                          Host::IDENT_HWADDR, subnet_->getID(),IPAddress("192.0.2.100"))));

    IPAddress addr("192.0.2.100");
    std::unique_ptr<AllocEngine> engine = createAllocateEngine(100);
    clientid_.reset(nullptr);
    ClientContext4& ctx = getContext(true, addr);
    engine->findReservation(ctx);
    Lease4Ptr offered_lease = engine->allocateLease4(ctx);
    ASSERT_TRUE(offered_lease.get());
    EXPECT_NE(offered_lease->addr_.str(), "192.0.2.100");
    Lease4Ptr from_mgr = lease_mgr_->getLease4(offered_lease->addr_);
    ASSERT_TRUE(from_mgr.get());
    lease_mgr_->deleteLease(offered_lease->addr_);
}

// This test checks that the client requesting an address which is
// reserved for another client will get no lease or a different
// address will be assigned if the client is sending a DHCPDISCOVER.
TEST_F(AllocEngine4Test, reservedAddressHintUsedByOtherClient) {
    uint8_t mac[] = { 0, 2, 22, 33, 44, 55};
    host_mgr_.reset(new HostsInMem());
    host_mgr_->add(HostPtr(new Host(mac, sizeof(mac),
                          Host::IDENT_HWADDR, subnet_->getID(),IPAddress("192.0.2.100"))));

    IPAddress addr("192.0.2.100");
    std::unique_ptr<AllocEngine> engine = createAllocateEngine(100);
    clientid_.reset(nullptr);
    ClientContext4& ctx = getContext(false, addr);
    engine->findReservation(ctx);
    Lease4Ptr offered_lease = engine->allocateLease4(ctx);
    ASSERT_FALSE(offered_lease.get());


    ClientContext4& ctx2 = getContext(true, addr);
    engine->findReservation(ctx2);
    offered_lease = engine->allocateLease4(ctx2);
    ASSERT_TRUE(offered_lease.get());
    EXPECT_NE(offered_lease->addr_.str(), "192.0.2.100");
    lease_mgr_->deleteLease(offered_lease->addr_);
}

// This test checks that the allocation engine refuses to allocate an
// address when the pool is exhausted, and the only available
// address is reserved for a different client.
TEST_F(AllocEngine4Test, reservedAddressShortPool) {
    initSubnet(IPAddress("192.0.2.100"), IPAddress("192.0.2.100"));

    host_mgr_.reset(new HostsInMem());
    uint8_t mac[] = { 0, 2, 22, 33, 44, 55};
    host_mgr_->add(HostPtr(new Host(mac, sizeof(mac),
                          Host::IDENT_HWADDR, subnet_->getID(),IPAddress("192.0.2.100"))));

    std::unique_ptr<AllocEngine> engine = createAllocateEngine(100);
    clientid_.reset(nullptr);
    ClientContext4& ctx = getContext(true, IPAddress("0.0.0.0"));
    // Create short pool with only one address.
    // Reserve the address for a different client.
    engine->findReservation(ctx);
    Lease4Ptr offered_lease = engine->allocateLease4(ctx);
    ASSERT_FALSE(offered_lease.get());

    host_mgr_.reset(new HostsInMem());
    engine = createAllocateEngine(100);
    ClientContext4& ctx2 = getContext(true, IPAddress("0.0.0.0"));
    engine->findReservation(ctx2);
    offered_lease = engine->allocateLease4(ctx2);
    ASSERT_TRUE(offered_lease.get());
    EXPECT_EQ("192.0.2.100", offered_lease->addr_.str());
    lease_mgr_->deleteLease(offered_lease->addr_);
}

// This test checks that the AllocEngine allocates an address from the
// dynamic pool if the client's reservation is made for a hostname but
// not for an address.
TEST_F(AllocEngine4Test, reservedHostname) {
    host_mgr_.reset(new HostsInMem());
    uint8_t mac[] = { 0, 2, 22, 33, 44, 55};
    host_mgr_->add(HostPtr(new Host(mac, sizeof(mac),
                          Host::IDENT_HWADDR, subnet_->getID(),IPAddress("0.0.0.0"), "foo.example.org")));

    std::unique_ptr<AllocEngine> engine = createAllocateEngine(100);
    IPAddress addr("192.0.2.109");
    clientid_.reset(nullptr);
    ClientContext4& ctx = getContext(true, addr, "foo.example.org");
    engine->findReservation(ctx);
    Lease4Ptr offered_lease = engine->allocateLease4(ctx);
    ASSERT_TRUE(offered_lease.get());
    ASSERT_EQ(offered_lease->addr_.str(), addr.str());

    ctx.requested_address_ = addr;
    ctx.fake_allocation_ = false;
    offered_lease = engine->allocateLease4(ctx);
    ASSERT_TRUE(offered_lease.get());
    EXPECT_EQ("192.0.2.109", offered_lease->addr_.str());
    lease_mgr_->deleteLease(addr);
}

/*
// This test checks that the AllocEngine::findReservation method finds
// and returns host reservation for the DHCPv4 client using the data from
// the client context. If the host reservation can't be found, it sets
// the value of NULL in the host_ field of the client context.
TEST_F(AllocEngine4Test, findReservation) {
    AllocEngine engine(AllocEngine::ALLOC_ITERATIVE, 100, lease_mgr_.get(), host_mgr_.get());
    ClientContext4 ctx(subnet_.get(), clientid_.get(), hwaddr_.get(), IPAddress("0.0.0.0"), "", false);
    ASSERT_NO_THROW(engine.findReservation(ctx));
    EXPECT_FALSE(ctx.host_);

    // Create a reservation for the client.
    host_mgr_.reset(new HostsInMem());
    host_mgr_->add(HostPtr(new Host(&hwaddr_->hwaddr_[0], hwaddr_->hwaddr_.size(),
                          Host::IDENT_HWADDR, subnet_->getID(),IPAddress("192.0.2.100"))));
    AllocEngine engine2(AllocEngine::ALLOC_ITERATIVE, 100, lease_mgr_.get(), host_mgr_.get());
    ASSERT_NO_THROW(engine2.findReservation(ctx));
    EXPECT_TRUE(ctx.host_);
    EXPECT_EQ(ctx.host_->getIPv4Reservation(), IPAddress("192.0.2.100"));
    subnet_->setHostReservationMode(Subnet::HR_DISABLED);
    ASSERT_NO_THROW(engine2.findReservation(ctx));
    EXPECT_TRUE(ctx.host_);
    EXPECT_EQ(ctx.host_->getIPv4Reservation(), IPAddress("192.0.2.100"));
    subnet_->setHostReservationMode(Subnet::HR_OUT_OF_POOL);
    ASSERT_NO_THROW(engine2.findReservation(ctx));
    EXPECT_TRUE(ctx.host_);
    EXPECT_EQ(ctx.host_->getIPv4Reservation(), IPAddress("192.0.2.100"));


    host_mgr_.reset(new HostsInMem());
    host_mgr_->add(HostPtr(new Host(&clientid_->getClientId()[0], clientid_->getClientId().size(),
                          Host::IDENT_DUID, subnet_->getID(),IPAddress("192.0.2.101"))));
    AllocEngine engine3(AllocEngine::ALLOC_ITERATIVE, 100, lease_mgr_.get(), host_mgr_.get());
    ASSERT_NO_THROW(engine3.findReservation(ctx));
    EXPECT_TRUE(ctx.host_);
    EXPECT_EQ(ctx.host_->getIPv4Reservation(), IPAddress("192.0.2.101"));

    ctx.subnet_ = nullptr;
    ASSERT_NO_THROW(engine.findReservation(ctx));
    EXPECT_FALSE(ctx.host_);

    ctx.subnet_ = subnet_.get();
    host_mgr_.reset(new HostsInMem());
    host_mgr_->add(HostPtr(new Host(&hwaddr_->hwaddr_[0], hwaddr_->hwaddr_.size(),
                          Host::IDENT_HWADDR, subnet_->getID() + 1,IPAddress("192.0.2.100"))));
    AllocEngine engine4(AllocEngine::ALLOC_ITERATIVE, 100, lease_mgr_.get(), host_mgr_.get());
    ASSERT_NO_THROW(engine4.findReservation(ctx));
    EXPECT_FALSE(ctx.host_);
}

TEST_F(AllocEngine4Test, IterativeAllocator) {
    std::unique_ptr<NakedAllocEngine::Allocator> alloc(new NakedAllocEngine::IterativeAllocator(Lease::TYPE_V4));

    for (int i = 0; i < 1000; ++i) {
        IPAddress candidate = alloc->pickAddress(subnet_.get(), clientid_.get(), IPAddress("0.0.0.0"));
        EXPECT_TRUE(subnet_->inPool(Lease::TYPE_V4, candidate));
    }
}


TEST_F(AllocEngine4Test, IterativeAllocator_manyPools4) {
    NakedAllocEngine::IterativeAllocator alloc(Lease::TYPE_V4);

    // Let's start from 2, as there is 2001:db8:1::10 - 2001:db8:1::20 pool already.
    for (int i = 2; i < 10; ++i) {
        stringstream min, max;

        min << "192.0.2." << i * 10 + 1;
        max << "192.0.2." << i * 10 + 9;

        Pool4Ptr pool(new Pool4(IPAddress(min.str()),
                                IPAddress(max.str())));
        // cout << "Adding pool: " << min.str() << "-" << max.str() << endl;
        subnet_->addPool(pool);
    }

    int total = 10 + 8 * 9; // first pool (.100 - .109) has 10 addresses in it,
                            // there are 8 extra pools with 9 addresses in each.

    // Let's keep picked addresses here and check their uniqueness.
    std::set<IPAddress> generated_addrs;
    int cnt = 0;
    while (++cnt) {
        IPAddress candidate = alloc.pickAddress(subnet_, clientid_, IPAddress("0.0.0.0"));
        EXPECT_TRUE(subnet_->inPool(Lease::TYPE_V4, candidate));

        // One way to easily verify that the iterative allocator really works is
        // to uncomment the following line and observe its output that it
        // covers all defined subnets.
        // cout << candidate.toText() << endl;

        if (generated_addrs.find(candidate) == generated_addrs.end()) {
            // We haven't had this
            generated_addrs.insert(candidate);
        } else {
            // We have seen this address before. That should mean that we
            // iterated over all addresses.
            if (generated_addrs.size() == total) {
                // We have exactly the number of address in all pools
                break;
            }
            ADD_FAILURE() << "Too many or not enough unique addresses generated.";
            break;
        }

        if ( cnt>total ) {
            ADD_FAILURE() << "Too many unique addresses generated.";
            break;
        }
    }
}
*/
};
