#include <kea/hooks/callout_handle.h>
#include <kea/hooks/callout_manager.h>


#include <algorithm>
#include <functional>
#include <iostream>

using namespace std;

namespace kea {
namespace hooks {

CalloutManager::CalloutManager()
    : server_hooks_(ServerHooks::instance()),
      hook_vector_(ServerHooks::instance().getCount())
{
}

void CalloutManager::registerCallout(int library_index, const std::string& name,
        CalloutPtr callout) {
    int hook_index = server_hooks_.getIndex(name);
    for (CalloutVector::iterator i = hook_vector_[hook_index].begin();
         i != hook_vector_[hook_index].end(); ++i) {
        if (i->first > library_index) {
            hook_vector_[hook_index].insert(i, make_pair(library_index, callout));
            return;
        }
    }

    hook_vector_[hook_index].push_back(make_pair(library_index, callout));
}

bool CalloutManager::calloutsPresent(const std::string& name) const {
    return calloutsPresent(server_hooks_.getIndex(name));
}

bool CalloutManager::calloutsPresent(int hook_index) const {
    if ((hook_index < 0) || (hook_index >= hook_vector_.size())) {
        return (false);
    }
    return (!hook_vector_[hook_index].empty());
}

void CalloutManager::callCallouts(const std::string& name, CalloutHandle& callout_handle) {
    callCallouts(server_hooks_.getIndex(name), callout_handle);
}

void CalloutManager::callCallouts(int hook_index, CalloutHandle& callout_handle) {
    callout_handle.setStatus(CalloutHandle::NEXT_STEP_CONTINUE);
    if (calloutsPresent(hook_index)) {
        CalloutVector callouts(hook_vector_[hook_index]);
        for (CalloutVector::const_iterator i = callouts.begin();
             i != callouts.end(); ++i) {
            try {
                (*i->second)(callout_handle);
            } catch (const std::exception& e) {
            }
        }
    }
}


bool CalloutManager::deregisterCallout(int library_index, const std::string& name,
        CalloutPtr callout) {
    int hook_index = server_hooks_.getIndex(name);

    CalloutEntry target(library_index, callout);
    size_t initial_size = hook_vector_[hook_index].size();
    hook_vector_[hook_index].erase(remove_if(hook_vector_[hook_index].begin(),
                                             hook_vector_[hook_index].end(),
                                             bind1st(equal_to<CalloutEntry>(),
                                                     target)),
                                   hook_vector_[hook_index].end());
    return (initial_size != hook_vector_[hook_index].size());
}

bool CalloutManager::deregisterAllCallouts(int library_index, const std::string& name) {
    int hook_index = server_hooks_.getIndex(name);
    CalloutEntry target(library_index, NULL);
    size_t initial_size = hook_vector_[hook_index].size();
    hook_vector_[hook_index].erase(remove_if(hook_vector_[hook_index].begin(),
                                             hook_vector_[hook_index].end(),
                                             bind1st(CalloutLibraryEqual(),
                                                     target)),
                                   hook_vector_[hook_index].end());

    return (initial_size != hook_vector_[hook_index].size());
}

};
};
