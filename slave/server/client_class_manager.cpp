#include <kea/server/client_class_parser.h>
#include <kea/server/client_class_manager.h>

namespace {
    static kea::server::ClientClassManager SingletonClientClassManager;
}

namespace kea {
namespace server {

ClientClassManager&
ClientClassManager::instance() {
    return SingletonClientClassManager;
}

void ClientClassManager::removeAll() {
    client_classes_.clear();
}

void ClientClassManager::addClientClass(const std::string& name, const std::string& exp) {
    auto i = client_classes_.find(name);
    if (i != client_classes_.end()) {
        kea_throw(DuplicateClientClassName, "duplicate client class with name:" << name);
    }

    PktOptionMatcher matcher = buildMatcher(exp);
    client_classes_.insert(std::make_pair(name, matcher));
}

std::vector<std::string> ClientClassManager::getMatchedClass(const Pkt& pkt) {
    std::vector<std::string> classes;
    for(auto& pair : client_classes_) {
        if (pair.second(pkt)) {
            classes.push_back(pair.first);
        }
    }
    return (std::move(classes));
}

};
};
