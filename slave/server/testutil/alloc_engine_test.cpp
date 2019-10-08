#include <kea/dhcp++/duid.h>
#include <kea/dhcp++/dhcp4.h>
#include <kea/dhcp++/pkt4.h>
#include <kea/server/lease_mgr.h>
#include <kea/server/pgsql_lease_mgr.h>
#include <kea/server/hosts_in_mem.h>

#include <kea/server/testutil/alloc_engine_test.h>

#include <iostream>
#include <sstream>
#include <algorithm>
#include <set>
#include <time.h>

namespace kea {
namespace server {
namespace testutil {

static const uint8_t DEFAULT_MAC[] = { 0, 1, 22, 33, 44, 55};
static const vector<uint8_t> DEFAULT_CLIENT_ID(8, 0x44);

AllocEngine4Test::AllocEngine4Test() {
    std::string conn_str = "type=postgresql name=zdns host=localhost user=zdns password=zdns";
    lease_mgr_.reset(new PgSqlLeaseMgr(DatabaseConnection::parse(conn_str)));
    subnet_mgr_.reset(new SubnetMgr());
    host_mgr_.reset(new HostsInMem());

    restoreClientId();
    restoreHWAddr();

    initSubnet(IPAddress("192.0.2.100"), IPAddress("192.0.2.109"));
}


void
AllocEngine4Test::restoreClientId() {
    clientid_.reset(new ClientId(DEFAULT_CLIENT_ID));
}

void
AllocEngine4Test::restoreHWAddr() {
    hwaddr_.reset(new HWAddr(DEFAULT_MAC, sizeof(DEFAULT_MAC), HTYPE_ETHER));
}


void
AllocEngine4Test::testReuseLease4(AllocEngine& engine,
                                  Lease4Ptr& existing_lease,
                                  const std::string& addr,
                                  const bool fake_allocation,
                                  ExpectedResult exp_result,
                                  Lease4Ptr& result) {
    if (existing_lease) {
        lease_mgr_->deleteLease(existing_lease->addr_);
        ASSERT_TRUE(lease_mgr_->addLease(*existing_lease));
    }

    ClientContext4 ctx(subnet_.get(), 
                       clientid_.get(), 
                       hwaddr_.get(), 
                       IPAddress(addr), 
                       "", 
                       fake_allocation);
    if (fake_allocation) {
        ctx.query_ = new Pkt4(DHCPDISCOVER, 1234);
    } else {
        ctx.query_ = new Pkt4(DHCPREQUEST, 1234);
    }
    result = engine.allocateLease4(ctx);

    switch (exp_result) {
    case SHOULD_PASS:
        ASSERT_TRUE(result.get());
        checkLease4(result);
        break;

    case SHOULD_FAIL:
        ASSERT_FALSE(result.get());
        break;
    }
}

Lease4Ptr
AllocEngine4Test::generateDeclinedLease(const std::string& addr,
                                        time_t probation_period,
                                        int32_t expired) {
    std::unique_ptr<HWAddr> hwaddr(new HWAddr());
    time_t now = time(NULL);
    Lease4Ptr declined(new Lease4(IPAddress(addr), 
                                  std::unique_ptr<HWAddr>(new HWAddr()),
                                  std::unique_ptr<ClientId>(), 
                                  495,
                                  100, 
                                  200, 
                                  now, 
                                  subnet_->getID()));
    declined->decline(probation_period);
    declined->cltt_ = now - probation_period + expired;
    return (std::move(declined));
}

void
AllocEngine4Test::initSubnet(const IPAddress& pool_start,
                             const IPAddress& pool_end) {
    subnet_ = SubnetMgr::Subnet4Ptr(new Subnet4(IPAddress("192.0.2.0"), 24, 1, 2, 3));
    subnet_->addPool(std::unique_ptr<Pool4>(new Pool4(pool_start, pool_end)));
}


ClientContext4& 
AllocEngine4Test::getContext(bool fake_allocation,
                             const IPAddress& addr, 
                             const std::string& host_name) {
    ctx_.reset(new ClientContext4(subnet_.get(), clientid_.get(), hwaddr_.get(), addr, host_name, fake_allocation));
    pkt_.reset(new Pkt4(fake_allocation? DHCPDISCOVER : DHCPREQUEST, 1234));
    ctx_->query_ = pkt_.get();
    return *ctx_;
}

std::unique_ptr<AllocEngine> 
AllocEngine4Test::createAllocateEngine(int retry_count) {
     return std::unique_ptr<AllocEngine>(new AllocEngine(AllocEngine::ALLOC_ITERATIVE, retry_count, lease_mgr_.get(), host_mgr_.get()));
}

void 
AllocEngine4Test::checkLease4(const Lease4Ptr& lease, bool ignore_valid_lft_check) {
    EXPECT_EQ(lease->subnet_id_, subnet_->getID());
    EXPECT_TRUE(subnet_->inRange(lease->addr_));
    EXPECT_TRUE(subnet_->inPool(Lease::TYPE_V4, lease->addr_));

    if (ignore_valid_lft_check == false) {
        EXPECT_EQ(subnet_->getValid(), lease->valid_lft_);
    }

    EXPECT_EQ(subnet_->getT1(), lease->t1_);
    EXPECT_EQ(subnet_->getT2(), lease->t2_);
    if (lease->client_id_ && !clientid_) {
        ADD_FAILURE() << "Lease4 has a client-id, while it should have none.";
    } else if (!lease->client_id_ && clientid_) {
        ADD_FAILURE() << "Lease4 has no client-id, but it was expected to have one.";
    } else if (lease->client_id_ && clientid_) {
        EXPECT_TRUE(*lease->client_id_ == *clientid_);
    }
    EXPECT_TRUE(*lease->hwaddr_ == *hwaddr_);
}









}; 
};
};
