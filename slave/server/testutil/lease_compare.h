#pragma once

#include <kea/server/lease_mgr.h>
#include <vector>

namespace kea {
namespace server {
namespace testutil {

void
detailCompareLease(const LeaseMgr::Lease4Ptr& first, const LeaseMgr::Lease4Ptr& second);

};
};
};
