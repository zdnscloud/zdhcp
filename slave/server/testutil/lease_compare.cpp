#include <kea/server/testutil/lease_compare.h>
#include <gtest/gtest.h>
#include <sstream>

using namespace std;

namespace kea {
namespace server {
namespace testutil {

void
detailCompareLease(const LeaseMgr::Lease4Ptr& first, const LeaseMgr::Lease4Ptr& second) {
    EXPECT_EQ(first->addr_, second->addr_);

    EXPECT_EQ(*first->hwaddr_, *second->hwaddr_);

    if (first->client_id_ && second->client_id_) {
        EXPECT_TRUE(*first->client_id_ == *second->client_id_);
    } else {
        if (first->client_id_ && !second->client_id_) {

            ADD_FAILURE() << "Client-id present in first lease ("
                          << first->client_id_->getClientId().size()
                          << " bytes), but missing in second.";
        }
        if (!first->client_id_ && second->client_id_) {
            ADD_FAILURE() << "Client-id missing in first lease, but present in second ("
                          << second->client_id_->getClientId().size()
                          << " bytes).";
        }
        // else here would mean that both leases do not have client_id_
        // which makes them equal in that regard. It is ok.
    }
    EXPECT_EQ(first->valid_lft_, second->valid_lft_);
    EXPECT_EQ(first->cltt_, second->cltt_);
    EXPECT_EQ(first->subnet_id_, second->subnet_id_);
    EXPECT_EQ(first->fqdn_fwd_, second->fqdn_fwd_);
    EXPECT_EQ(first->fqdn_rev_, second->fqdn_rev_);
    EXPECT_EQ(first->hostname_, second->hostname_);
}
}; 
}; 
};
