// Copyright (C) 2013-2016 Internet Systems Consortium, Inc. ("ISC")
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once
#include <kea/hooks/server_hooks.h>
#include <kea/hooks/library_manager_collection.h>
#include <kea/hooks/library_manager.h>
#include <kea/configure/json_conf.h>

#include <string>
#include <vector>

namespace kea {
namespace hooks {

using JsonObject = kea::configure::JsonObject;

class CalloutHandle;
class LibraryManagerCollection;

class HooksManager {
public:
    static HooksManager& instance();

    bool loadLibraries(const vector<JsonObject>&);
    void unloadLibraries();
    std::vector<std::string> getLibraryNames() const;

    bool calloutsPresent(int) const;
    void callCallouts(int, CalloutHandle&);

    std::unique_ptr<CalloutHandle> createCalloutHandle();

    int registerHook(const std::string&);

    std::vector<std::string> validateLibraries(const std::vector<std::string>& libraries);

    static const int CONTEXT_CREATE = ServerHooks::CONTEXT_CREATE;
    static const int CONTEXT_DESTROY = ServerHooks::CONTEXT_DESTROY;

private:
    HooksManager();
    std::unique_ptr<LibraryManagerCollection> lm_collection_;
};

}; 
};
