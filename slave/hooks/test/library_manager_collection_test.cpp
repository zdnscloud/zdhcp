// Copyright (C) 2013-2016 Internet Systems Consortium, Inc. ("ISC")
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <kea/hooks/callout_handle.h>
#include <kea/hooks/callout_manager.h>
#include <kea/hooks/library_manager.h>
#include <kea/hooks/library_manager_collection.h>
#include <kea/configure/json_conf.h>


#include <gtest/gtest.h>


using namespace kea;
using namespace kea::hooks;
using namespace kea::configure;
using namespace std;

namespace kea {

TEST(LibraryManagerCollectionTest, LoadLibraries) {
    std::string lib_conf = R"({
    "hooks-libraries": [
        { "library": "./lib/libbasic_callout_library.so" },
        { "library": "./lib/libfull_callout_library.so" }]
})";

    std::unique_ptr<JsonConf> conf = JsonConf::parseString(lib_conf);
    LibraryManagerCollection lm_collection(conf->root().getObjects("hooks-libraries"));
    EXPECT_TRUE(lm_collection.loadLibraries());
    EXPECT_EQ(2, lm_collection.getLoadedLibraryCount());
    EXPECT_TRUE(lm_collection.getCalloutManager() != nullptr);
    EXPECT_NO_THROW(lm_collection.unloadLibraries());
    EXPECT_EQ(0, lm_collection.getLoadedLibraryCount());
}
}
