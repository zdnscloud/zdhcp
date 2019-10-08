#pragma once

#include <kea/exceptions/exceptions.h>
#include <map>
#include <string>
#include <vector>

namespace kea {
namespace hooks {

class DuplicateHook : public Exception {
public:
    DuplicateHook(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) {}
};

class NoSuchHook : public Exception {
public:
    NoSuchHook(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) {}
};


class ServerHooks {
public:
    ServerHooks(const ServerHooks&) = delete;
    ServerHooks& operator=(const ServerHooks&) = delete;

    static const int CONTEXT_CREATE = 0;
    static const int CONTEXT_DESTROY = 1;

    void reset();
    int registerHook(const std::string& name);
    std::string getName(int index) const;
    int getIndex(const std::string& name) const;
    int getCount() const {
        return (hooks_.size());
    }
    std::vector<std::string> getHookNames() const;
    static ServerHooks& instance();

private:
    ServerHooks();
    void initialize();

    typedef std::map<std::string, int> HookCollection;
    typedef std::map<int, std::string> InverseHookCollection;

    HookCollection  hooks_;                 
    InverseHookCollection inverse_hooks_;  
};

}; 
};
