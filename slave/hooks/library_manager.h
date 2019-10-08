#pragma once

#include <kea/exceptions/exceptions.h>
#include <kea/configure/json_conf.h>

#include <string>

namespace kea {
namespace hooks {

using JsonObject = kea::configure::JsonObject;

class NoCalloutManager : public Exception {
public:
    NoCalloutManager(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) {}
};

class CalloutManager;
class LibraryHandle;
class LibraryManager;

class LibraryManager {
public:
    LibraryManager(const std::string& name, int index, CalloutManager& manager);
    ~LibraryManager();

    static bool validateLibrary(const std::string& name);
    bool loadLibrary(const JsonObject& parameters);
    bool unloadLibrary();
    std::string getName() const {
        return (library_name_);
    }

    static void* openLibrary(const string& path);
    static bool closeLibrary(void* handle);
    static bool checkVersion(void* handle);

protected:
    void registerStandardCallouts();
    bool runLoad(const JsonObject& parameters);
    bool runUnload();

private:
    void*       dl_handle_;   
    int         index_;        
    CalloutManager& manager_;
    std::string library_name_;  
};
};
};
