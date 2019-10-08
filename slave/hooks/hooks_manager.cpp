#include <kea/hooks/callout_handle.h>
#include <kea/hooks/callout_manager.h>
#include <kea/hooks/library_manager_collection.h>
#include <kea/hooks/hooks_manager.h>

#include <string>
#include <vector>

using namespace std;

namespace kea {
namespace hooks {

HooksManager::HooksManager() {
    lm_collection_.reset(new LibraryManagerCollection(vector<JsonObject>()));
}

HooksManager& HooksManager::instance() {
    static HooksManager manager;
    return (manager);
}

bool HooksManager::calloutsPresent(int index) const {
    return (lm_collection_->getCalloutManager()->calloutsPresent(index));
}

void HooksManager::callCallouts(int index, CalloutHandle& handle) {
    return (lm_collection_->getCalloutManager()->callCallouts(index, handle));
}


bool HooksManager::loadLibraries(const vector<JsonObject>& libraries) {
    lm_collection_.reset(new LibraryManagerCollection(libraries));
    bool status = lm_collection_->loadLibraries();
    if (!status) {
        unloadLibraries();
    }

    return (status);
}

void HooksManager::unloadLibraries() {
    lm_collection_.reset(new LibraryManagerCollection(vector<JsonObject>()));
}

std::unique_ptr<CalloutHandle> HooksManager::createCalloutHandle() {
    return (std::unique_ptr<CalloutHandle>(new CalloutHandle(*lm_collection_->getCalloutManager())));
}

std::vector<std::string> HooksManager::getLibraryNames() const {
    return lm_collection_->getLibraryNames();
}


int HooksManager::registerHook(const std::string& name) {
    return (ServerHooks::instance().registerHook(name));
}

std::vector<std::string> HooksManager::validateLibraries(const std::vector<std::string>& libraries) {
    return (LibraryManagerCollection::validateLibraries(libraries));
}

};
};
