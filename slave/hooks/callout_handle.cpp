#include <kea/hooks/callout_handle.h>
#include <kea/hooks/callout_manager.h>
#include <kea/hooks/server_hooks.h>

#include <string>
#include <utility>
#include <vector>

using namespace std;

namespace kea {
namespace hooks {

CalloutHandle::CalloutHandle(CalloutManager& manager)
    : manager_(manager), arguments_(), server_hooks_(ServerHooks::instance()),
      next_step_(NEXT_STEP_CONTINUE) {
    manager_.callCallouts(ServerHooks::CONTEXT_CREATE, *this);
}

CalloutHandle::~CalloutHandle() {
    manager_.callCallouts(ServerHooks::CONTEXT_DESTROY, *this);
    arguments_.clear();
}

vector<string> CalloutHandle::getArgumentNames() const {
    vector<string> names;
    for(auto& pair : arguments_) {
        names.push_back(pair.first);
    }
    return (names);
}

}; 
};
