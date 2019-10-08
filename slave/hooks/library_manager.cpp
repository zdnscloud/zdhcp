#include <kea/hooks/hooks.h>
#include <kea/hooks/callout_manager.h>
#include <kea/hooks/library_manager.h>
#include <kea/hooks/server_hooks.h>

#include <string>
#include <vector>
#include <dlfcn.h>

using namespace std;

namespace kea {
namespace hooks {

LibraryManager::LibraryManager(const std::string& name, int index, CalloutManager& manager)
        : dl_handle_(NULL), index_(index), manager_(manager), library_name_(name)
{}

LibraryManager::~LibraryManager() {
    unloadLibrary();
}

void* LibraryManager::openLibrary(const string& library_name) {
    return dlopen(library_name.c_str(), RTLD_NOW | RTLD_LOCAL);
}

bool LibraryManager::closeLibrary(void *handle) {
    int status = 0;
    if (handle != NULL) {
        status = dlclose(handle);
    }
    return (status == 0);
}

bool LibraryManager::checkVersion(void *handle) {
    version_function_ptr func = reinterpret_cast<version_function_ptr>(dlsym(handle, VERSION_FUNCTION_NAME));
    if (func != NULL) {
        int version = KEA_HOOKS_VERSION - 1; // This is an invalid value
        try {
            version = (*func)();
        } catch (...) {
            return (false);
        }

        if (version == KEA_HOOKS_VERSION) {
            return (true);
        } 
    }

    return (false);
}

void LibraryManager::registerStandardCallouts() {
    vector<string> hook_names = ServerHooks::instance().getHookNames();
    for (auto& name : hook_names) {
        void* dlsym_ptr = dlsym(dl_handle_, name.c_str());
        CalloutPtr call_out = reinterpret_cast<CalloutPtr>(dlsym_ptr);
        if (call_out != NULL) {
            manager_.registerCallout(index_, name, call_out);
        }
    }
}


bool LibraryManager::runLoad(const JsonObject& parameters) {
    load_function_ptr func = reinterpret_cast<load_function_ptr>(dlsym(dl_handle_, LOAD_FUNCTION_NAME));
    if (func != NULL) {
        int status = -1;
        try {
            status = (*func)(parameters);
        } catch (const kea::Exception& ex) {
            return (false);
        } catch (...) {
            return (false);
        }

        if (status != 0) {
            return (false);
        }
    } 
    return (true);
}

bool LibraryManager::runUnload() {
    unload_function_ptr func = reinterpret_cast<unload_function_ptr>(dlsym(dl_handle_, UNLOAD_FUNCTION_NAME));
    if (func != NULL) {
        int status = -1;
        try {
            status = (*func)();
        } catch (const kea::Exception& ex) {
            return (false);
        } catch (...) {
            return (false);
        }

        if (status != 0) {
            return (false);
        } 
    } 

    return (true);
}

bool LibraryManager::loadLibrary(const JsonObject& parameters) {
    void* handle = openLibrary(library_name_);
    if (handle != nullptr) {
        if (checkVersion(handle)) {
            dl_handle_ = handle;
            registerStandardCallouts();
            if (runLoad(parameters)) {
                return (true);
            } else {
                static_cast<void>(unloadLibrary());
            }
        }
        static_cast<void>(closeLibrary(handle));
    }

    return (false);
}

bool LibraryManager::unloadLibrary() {
    bool result = true;
    if (dl_handle_ != NULL) {
        result = runUnload();
        vector<string> hooks = ServerHooks::instance().getHookNames();
        for (size_t i = 0; i < hooks.size(); ++i) {
            manager_.deregisterAllCallouts(index_, hooks[i]);
        }

        result = closeLibrary(dl_handle_) && result;
    }
    return (result);
}

bool LibraryManager::validateLibrary(const std::string& name) {
    void *handle = openLibrary(name);
    bool validated = false;
    if (handle != nullptr) {
        validated = checkVersion(handle);
        closeLibrary(handle);
    }
    return validated;
}

}; 
}; 
