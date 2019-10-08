#include <kea/rpc/rpc_allocate_engine.h>
#include <kea/rpc/lease.pb.h>
#include <kea/dhcp++/dhcp4.h>
#include <thread>
#include <kea/logging/logging.h>
#include <chrono>
#include <iostream>

using namespace kea::logging;

const uint32_t QUEUE_MAX_SIZE = 1024;
const uint32_t DEFAULT_CONN_COUNT = 4; 

namespace kea {
namespace rpc {

static RpcAllocateEngine* SingletonRpcAllocateEngine = nullptr;

RpcAllocateEngine::RpcAllocateEngine(std::string server_addr, uint32_t port) {
    request_queue_.reset(new RPCRequestQueue(QUEUE_MAX_SIZE * DEFAULT_CONN_COUNT));
    for (int i = 0; i < DEFAULT_CONN_COUNT; i++) {
        connections_.push_back(std::unique_ptr<RpcConn>(new RpcConn(server_addr, port, *request_queue_)));
    }
    stop_.store(false);
}

void 
RpcAllocateEngine::allocateAddr(ClientContextPtr client_ctx, ClientContextHandler callback) {
    request_queue_->blockingWrite(RPCRecord(std::move(client_ctx), callback));
}

void 
RpcAllocateEngine::stop() {
    if(stop_.load()) { return; }
    stop_.store(true);

    RPCRecord tmp;
    while (request_queue_->isEmpty() == false) {
        request_queue_->blockingRead(tmp);    
    }

    for(auto& conn : connections_) {
        request_queue_->blockingWrite(RPCRecord());
    }

    for(auto& conn : connections_) {
        conn->stop();
    }
}

void
RpcAllocateEngine::init(std::string server_addr, uint32_t port) {
    if(SingletonRpcAllocateEngine != nullptr) {
        SingletonRpcAllocateEngine->stop();
        delete SingletonRpcAllocateEngine;
    }
        
    SingletonRpcAllocateEngine = new RpcAllocateEngine(server_addr, port);
    std::atexit([](){ 
            SingletonRpcAllocateEngine->stop();
            delete SingletonRpcAllocateEngine; 
    });
}

RpcAllocateEngine&
RpcAllocateEngine::instance() {
    return *SingletonRpcAllocateEngine;
}


RpcConn::RpcConn(std::string server_addr, uint32_t port, RPCRequestQueue& in_queue) 
    : socket_(io_service_), 
    in_queue_(in_queue) {
    asio::ip::tcp::resolver resolver(io_service_);
    endpoint_iterator_ = resolver.resolve({server_addr, std::to_string(port)});
    connectServer();
    std::thread io_loop([this](){ this->io_service_.run();});
    io_loop_ = std::move(io_loop);
    stop_.store(false);
}

RpcConn::~RpcConn() {
    stop();
}

void 
RpcConn::stop() {
    if(stop_.load()) { return; }
    stop_.store(true);
    io_service_.post([this]() { socket_.close(); });
    io_loop_.join();
}

void
RpcConn::connectServer() {
    asio::async_connect(socket_, endpoint_iterator_, [&](std::error_code ec, asio::ip::tcp::resolver::iterator) {
        if (!ec) {
            messageWrite();
        } else {
            logError("RpcConn   ", "!!!connect failed: $0, and reconnect", ec.message().c_str());
            if (this->stop_.load() == false) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
                connectServer();
            }
        }
    });
}

void
RpcConn::messageWrite() {
    in_queue_.blockingRead(rpc_request_);    
    if (rpc_request_.client_ctx_ != nullptr) {
        int request_len = marshRequset(*(rpc_request_.client_ctx_));
        result_body_[0] = (request_len & 0xff00) >> 8;
        result_body_[1] = (request_len & 0xff);
        asio::async_write(socket_, asio::buffer(result_body_, request_len + 2), [this, request_len](std::error_code ec, std::size_t){
            if (!ec) {
                memset(result_body_, 0, request_len + 2);
                readResultHeader();
            } else {
                logError("RpcConn   ", "Send message to master failed: $0, and reconnect", ec.message().c_str());
                if (this->stop_.load() == false) {
                    connectServer();
                }
            }
        });
    }
}

void 
RpcConn::readResultHeader() {
    asio::async_read(socket_, asio::buffer(result_len_, 2), [this](std::error_code ec, std::size_t ) {
        if (!ec) {
            readResultBody();
        } else {
            logError("RpcConn   ", "Read result message header failed: $0, and reconnect", ec.message().c_str());
            if (this->stop_.load() == false) {
                connectServer();
            }
        }
    });
}

void 
RpcConn::readResultBody() {
    int result_len = (int(static_cast<uint8_t>(result_len_[0])) << 8) + static_cast<uint8_t>(result_len_[1]); 
    if (result_len > 0 && result_len < MAX_RESULT_BODY_LEN) {
        asio::async_read(socket_, asio::buffer(result_body_, result_len), [=](std::error_code ec, std::size_t) {
                if (!ec) {
                    unMarshResult(result_body_);
                    messageWrite();
                } else {
                    logError("RpcConn   ", "Read result message body failed: $0, and reconnect",ec.message().c_str());
                    if (this->stop_.load() == false) {
                        connectServer();
                    }
                }
        });
    } else if (this->stop_.load() == false) {
        logWarning("RpcConn   ", "Read result message body failed with len $0", result_len);
        messageWrite();
    }
}

void 
RpcConn::unMarshResult(char* result_body) {
    kea::rpc::LeaseResult result;
    std::istringstream istream(result_body);
    result.ParseFromIstream(&istream);
    IOAddress allocate_addr(0);
    uint32_t subnet_id = 0;

    if (result.succeed()) {
        allocate_addr = IOAddress::fromLong(result.addr());
        subnet_id = result.subnetid();
    }

    logDebug("RpcConn   ", "Receive result $0 with ip $1 and subnet_id $2 and msg type $3", 
            result.succeed(), allocate_addr.toText(), subnet_id, rpc_request_.client_ctx_->getQueryType());
    rpc_request_.client_ctx_->setYourAddr(allocate_addr);
    rpc_request_.client_ctx_->setSharedSubnetID(subnet_id);

    if (rpc_request_.val_ != nullptr) {
        rpc_request_.val_(std::move(rpc_request_.client_ctx_));
    }
}

int
RpcConn::marshRequset(ClientContext& request) {
    ContextMsg cmsg;
    switch (request.getQueryType()){
        case dhcp::DHCPDISCOVER:
            cmsg.set_requesttype(ContextMsg::Discover);
            break;
        case dhcp::DHCPREQUEST:
            cmsg.set_requesttype(ContextMsg::Request);
            break;
        case dhcp::DHCPRELEASE:
            cmsg.set_requesttype(ContextMsg::Release);
            break;
        case dhcp::DHCPDECLINE:
            cmsg.set_requesttype(ContextMsg::Decline);
            break;
        case dhcp::DHCPCONFLICTIP:
            cmsg.set_requesttype(ContextMsg::ConflictIP);
    }
    cmsg.set_subnetid(request.getSubnetID());
    std::vector<uint8_t> clientID = request.getClientID();
    cmsg.set_clientid(clientID.data(), clientID.size());
    std::vector<uint8_t> hwaddr = request.getHWAddr();
    cmsg.set_mac(hwaddr.data(), hwaddr.size());
    cmsg.set_hostname(request.getHostName());
    cmsg.set_requestaddr(IOAddress::toLong(request.getRequestAddr()));

    int msg_size = cmsg.ByteSize();
    assert(msg_size + 2 < MAX_RESULT_BODY_LEN);
    memset(result_body_, 0, msg_size + 2);
    cmsg.SerializeToArray(result_body_ + 2, msg_size);
    return msg_size;
}

};
};
