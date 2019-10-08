#pragma once
#include <kea/exceptions/exceptions.h>
#include <kea/configure/json_conf.h>
#include <kea/hooks/library_manager.h>
#include <kea/hooks/callout_manager.h>

#include <vector>

namespace kea {
namespace hooks {

using JsonObject = kea::configure::JsonObject;

class LoadLibrariesNotCalled : public Exception {
public:
    LoadLibrariesNotCalled(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) {}
};


class CalloutManager;
class LibraryManager;

class LibraryManagerCollection {
public:
    LibraryManagerCollection(const vector<JsonObject>& libraries);
    ~LibraryManagerCollection() {
        static_cast<void>(unloadLibraries());
    }

    bool loadLibraries();

    CalloutManager* getCalloutManager() const;

    std::vector<std::string> getLibraryNames() const {
        return (library_names_);
    }

    int getLoadedLibraryCount() const;

    static std::vector<std::string>
    validateLibraries(const std::vector<std::string>& libraries);
    void unloadLibraries();

private:
    std::vector<std::string>                        library_names_;
    std::vector<std::unique_ptr<LibraryManager> >   lib_managers_;
    std::unique_ptr<CalloutManager>               callout_manager_;
    std::vector<JsonObject>                       library_infos_;
};

}; 
};
