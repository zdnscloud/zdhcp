#include <kea/controller/tcp_session_manager.h>

namespace kea{
namespace controller{

TcpSessionManager::TcpSessionManager() {};

void 
TcpSessionManager::addSession(SessionSharedPtr conn) {
    connections_.insert(conn);
}

void 
TcpSessionManager::removeSession(SessionSharedPtr conn) {
    connections_.erase(conn);
    conn->stop();
}

void 
TcpSessionManager::stop() {
    for (auto conn : connections_) {
        conn->stop();
    }
    connections_.clear();
}

}
}
