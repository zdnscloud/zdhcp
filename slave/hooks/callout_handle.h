#pragma once

#include <map>
#include <string>
#include <vector>
#include <boost/any.hpp>

#include <kea/exceptions/exceptions.h>

namespace kea {
namespace hooks {

class ServerHooks;

class NoSuchArgument : public Exception {
public:
    NoSuchArgument(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) {}
};

class NoSuchCalloutContext : public Exception {
public:
    NoSuchCalloutContext(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) {}
};

class CalloutManager;
class LibraryHandle;
class LibraryManagerCollection;

class CalloutHandle {
public:
    enum CalloutNextStep {
        NEXT_STEP_CONTINUE = 0, ///< continue normally
        NEXT_STEP_SKIP = 1,     ///< skip the next processing step
        NEXT_STEP_DROP = 2      ///< drop the packet
    };

    typedef std::map<std::string, boost::any> ElementCollection;

    CalloutHandle(CalloutManager& manager); 
    ~CalloutHandle();

    template <typename T>
    void setArgument(const std::string& name, T value) {
        arguments_[name] = value;
    }

    template <typename T>
    void getArgument(const std::string& name, T& value) const {
        ElementCollection::const_iterator element_ptr = arguments_.find(name);
        if (element_ptr == arguments_.end()) {
            kea_throw(NoSuchArgument, "unable to find argument with name " <<
                      name);
        }

        value = boost::any_cast<T>(element_ptr->second);
    }

    std::vector<std::string> getArgumentNames() const;
    void deleteArgument(const std::string& name) {
        static_cast<void>(arguments_.erase(name));
    }

    void deleteAllArguments() {
        arguments_.clear();
    }

    void setStatus(const CalloutNextStep next) {
        next_step_ = next;
    }

    CalloutNextStep getStatus() const {
        return (next_step_);
    }

    std::string getHookName() const;

private:
    int getLibraryIndex() const;

    CalloutManager& manager_;
    ElementCollection arguments_;
    ServerHooks& server_hooks_;
    CalloutNextStep next_step_;
};
};
};
