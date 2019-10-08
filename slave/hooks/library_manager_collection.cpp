#include <kea/hooks/callout_manager.h>
#include <kea/hooks/library_manager.h>
#include <kea/hooks/library_manager_collection.h>

namespace kea {
namespace hooks {

CalloutManager* LibraryManagerCollection::getCalloutManager() const {
    return (callout_manager_.get());
}

LibraryManagerCollection::LibraryManagerCollection(const vector<JsonObject>& libraries) {
    for (auto& lib : libraries) {
        library_names_.push_back(lib.getString("library"));
    }
    library_infos_ = libraries;
    callout_manager_.reset(new CalloutManager());
}

bool LibraryManagerCollection::loadLibraries() {
    static_cast<void>(unloadLibraries());
    callout_manager_.reset(new CalloutManager());

    for (size_t i = 0; i < library_names_.size(); ++i) {
        std::unique_ptr<LibraryManager> manager(new LibraryManager(library_names_[i], i, *callout_manager_));
        JsonObject parameters(nullptr);
        if (library_infos_[i].hasKey("parameters")) {
            parameters = library_infos_[i].getObject("parameters");
        }

        if (manager->loadLibrary(parameters)) {
            lib_managers_.push_back(std::move(manager));
        } else {
            static_cast<void>(unloadLibraries());
            return (false);
        }
    }

    return (true);
}


void LibraryManagerCollection::unloadLibraries() {
    for (int i = lib_managers_.size() - 1; i >= 0; --i) {
        lib_managers_[i].reset();
    }
    lib_managers_.clear();
    callout_manager_.reset();
}

int LibraryManagerCollection::getLoadedLibraryCount() const {
    return (lib_managers_.size());
}

std::vector<std::string> LibraryManagerCollection::validateLibraries(
        const std::vector<std::string>& libraries) {
    std::vector<std::string> failures;
    for (size_t i = 0; i < libraries.size(); ++i) {
        if (!LibraryManager::validateLibrary(libraries[i])) {
            failures.push_back(libraries[i]);
        }
    }

    return (failures);
}

};
};
