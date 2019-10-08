#pragma once

#include <set>
#include <string>


namespace kea {
namespace dhcp {

    typedef std::string ClientClass;

    class ClientClasses : public std::set<ClientClass> {
    public:

        ClientClasses() : std::set<ClientClass>() {
        }

        ClientClasses(const std::string& class_names);

        bool
        contains(const ClientClass& x) const {
            return (find(x) != end());
        }

        std::string toText(const std::string& separator = ", ") const;
    };

};

};
