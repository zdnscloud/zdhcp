#pragma once

#include <kea/server/lease_mgr.h>
#include <kea/server/alloc_engine.h>
#include <kea/server/subnet_mgr.h>
#include <kea/server/base_host_data_source.h>
#include <folly/IPAddress.h>
#include <gtest/gtest.h>
#include <vector>

using namespace folly;
using namespace kea::dhcp;
using namespace kea::server;

namespace kea {
namespace server {
namespace testutil {

using Lease4Ptr = kea::server::LeaseMgr::Lease4Ptr;

class AllocEngine4Test : public ::testing::Test {
public:

    enum ExpectedResult {
        SHOULD_PASS,
        SHOULD_FAIL
    };

    AllocEngine4Test();

    ClientContext4& getContext(bool fake_allocation,
                               const IPAddress& addr, 
                               const std::string& host_name = "");

    std::unique_ptr<AllocEngine> createAllocateEngine(int retry_count);

    void checkLease4(const Lease4Ptr& lease, bool ignore_valid_lft_check = false);

    void testReuseLease4(AllocEngine& alloc_engine,
                         Lease4Ptr& existing_lease,
                         const std::string& addr,
                         const bool fake_allocation,
                         ExpectedResult exp_result,
                         Lease4Ptr& result);

    Lease4Ptr generateDeclinedLease(const std::string& addr,
                                    time_t probation_period,
                                    int32_t expired);

    void initSubnet(const IPAddress& pool_start, const IPAddress& pool_end);
    void restoreClientId();
    void restoreHWAddr();

    virtual ~AllocEngine4Test() {}

    std::unique_ptr<ClientId> clientid_;      
    std::unique_ptr<HWAddr>   hwaddr_;        
    std::unique_ptr<Subnet4>  subnet_;
    std::unique_ptr<LeaseMgr> lease_mgr_;
    std::unique_ptr<SubnetMgr> subnet_mgr_;
    std::unique_ptr<BaseHostDataSource> host_mgr_;
    Pool4* pool_;             
    std::unique_ptr<ClientContext4> ctx_; 
    std::unique_ptr<Pkt4> pkt_; 
};

}; 
};
};
