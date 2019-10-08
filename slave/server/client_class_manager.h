#pragma once
#include <kea/server/client_class_matcher.h>
#include <map>

using namespace kea::dhcp;

namespace kea {
namespace server {

class DuplicateClientClassName : public Exception {
    public:
        DuplicateClientClassName(const char* file, size_t line, const char* what) :
            kea::Exception(file, line, what) { };
};

class ClientClassManager {
    public:
        static ClientClassManager& instance();
        ClientClassManager() {}

        void addClientClass(const std::string& name, const std::string& exp);
        std::vector<std::string> getMatchedClass(const Pkt& pkt);
        void removeAll();

    private:
        std::map<std::string, PktOptionMatcher> client_classes_;
};

};
};
