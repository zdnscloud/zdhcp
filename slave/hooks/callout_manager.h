#pragma once

#include <kea/exceptions/exceptions.h>
#include <kea/hooks/server_hooks.h>

#include <climits>
#include <map>
#include <string>

namespace kea {
namespace hooks {

class NoSuchLibrary : public Exception {
public:
    NoSuchLibrary(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) {}
};

class CalloutHandle;
typedef int (*CalloutPtr)(CalloutHandle&);

class CalloutManager {
private:
    typedef std::pair<int, CalloutPtr> CalloutEntry; //library index + callout function
    typedef std::vector<CalloutEntry> CalloutVector; //hook index -> entries

public:
    CalloutManager();

    void registerCallout(int library_index, const std::string& name, CalloutPtr callout);
    bool deregisterCallout(int library_index, const std::string& name, CalloutPtr callout);
    bool deregisterAllCallouts(int library_index, const std::string& name);

    bool calloutsPresent(const std::string& name) const;
    bool calloutsPresent(int hook_index) const;

    void callCallouts(const std::string& name, CalloutHandle& callout_handle);
    void callCallouts(int hook_index, CalloutHandle& callout_handle);

private:
    class CalloutLibraryEqual :
        public std::binary_function<CalloutEntry, CalloutEntry, bool> {
    public:
        bool operator()(const CalloutEntry& ent1,
                        const CalloutEntry& ent2) const {
            return (ent1.first == ent2.first);
        }
    };

    ServerHooks& server_hooks_;
    std::vector<CalloutVector> hook_vector_;
};

};
};
