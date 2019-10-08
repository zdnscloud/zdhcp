#include <kea/hooks/server_hooks.h>

#include <utility>
#include <vector>

using namespace std;
using namespace kea;

namespace kea {
namespace hooks {

ServerHooks::ServerHooks() {
    initialize();
}

int ServerHooks::registerHook(const string& name) {
    // Determine index for the new element and insert.
    int index = hooks_.size();
    auto result = hooks_.insert(make_pair(name, index));
    if (!result.second) {
        kea_throw(DuplicateHook, "hook with name " << name <<
                  " is already registered");
    }
    inverse_hooks_[index] = name;
    return (index);
}

void ServerHooks::initialize() {
    hooks_.clear();
    inverse_hooks_.clear();

    int create = registerHook("context_create");
    int destroy = registerHook("context_destroy");
    if ((create != CONTEXT_CREATE) || (destroy != CONTEXT_DESTROY)) {
        kea_throw(Unexpected, "pre-defined hook indexes are not as expected. "
                  "context_create: expected = " << CONTEXT_CREATE <<
                  ", actual = " << create <<
                  ". context_destroy: expected = " << CONTEXT_DESTROY <<
                  ", actual = " << destroy);
    }
}

void ServerHooks::reset() {
    initialize();
}

std::string ServerHooks::getName(int index) const {
    auto i = inverse_hooks_.find(index);
    if (i == inverse_hooks_.end()) {
        kea_throw(NoSuchHook, "hook index " << index << " is not recognized");
    }
    return (i->second);
}

int ServerHooks::getIndex(const string& name) const {
    auto i = hooks_.find(name);
    if (i == hooks_.end()) {
        kea_throw(NoSuchHook, "hook name " << name << " is not recognized");
    }
    return (i->second);
}

vector<string> ServerHooks::getHookNames() const {
    vector<string> names;
    for (auto& h : hooks_) {
        names.push_back(h.first);
    }
    return (names);
}

ServerHooks& ServerHooks::instance() {
    static ServerHooks hooks;
    return (hooks);
}

}; 
};
