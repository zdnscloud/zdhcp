#pragma once

#include <kea/server/lease_mgr.h>
#include <folly/IPAddress.h>
#include <gtest/gtest.h>
#include <vector>

using namespace folly;

namespace kea {
namespace server {
namespace testutil {

class GenericLeaseMgrTest : public ::testing::Test {
public:

    enum Universe {
        V4,
    };

    GenericLeaseMgrTest();
    virtual ~GenericLeaseMgrTest();

    virtual void reopen(Universe u = V4) = 0;

    LeaseMgr::Lease4Ptr initializeLease4(std::string address);

    template <typename T>
    void checkLeasesDifferent(const std::vector<T>& leases) const;

    void createLeases4(std::vector<LeaseMgr::Lease4Ptr>&);

    void testBasicLease4();
    void testMaxDate4();
    void testGetLease4ClientId();
    void testGetLease4NullClientId();
    void testGetLease4HWAddr1();
    void testGetLease4HWAddr2();
    void testGetLease4ClientIdHWAddrSubnetId();
    void testGetLease4HWAddrSize();
    void testGetLease4HWAddrSubnetId();
    void testGetLease4HWAddrSubnetIdSize();
    void testGetLease4ClientId2();
    void testGetLease4ClientIdSize();
    void testGetLease4ClientIdSubnetId();
    void testLease4NullClientId();
    void testRecreateLease4();
    void testLease4InvalidHostname();
    void testUpdateLease4();
    void testVersion(int major, int minor);
    void testGetExpiredLeases4();
    void testGetDeclinedLeases4();
    void testDeleteExpiredReclaimedLeases4();

    std::vector<std::string>  straddress4_;
    std::vector<IPAddress> ioaddress4_;
    LeaseMgr* lmptr_;
};

}; 
};
};
