#pragma once

#include <kea/controller/tcp_session.h>
#include <set>

namespace kea{
namespace controller{

class TcpSessionManager {
    public:
        TcpSessionManager();
        TcpSessionManager(const TcpSessionManager&) = delete;
        TcpSessionManager& operator=(const TcpSessionManager&) = delete;

        void addSession(SessionSharedPtr conn);
        void removeSession(SessionSharedPtr conn);
        void stop();

    private:
        std::set<SessionSharedPtr> connections_;
};

}
}
