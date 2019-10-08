#pragma once

#include <cstdlib>
#include <queue>
#include <iostream>
#include <thread>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <string>
#include <vector>
#include <asio.hpp>
#include <kea/rpc/context.pb.h>
#include <kea/util/io_address.h>
#include <kea/client/client_context_wrapper.h>
#include <folly/MPMCQueue.h>

using namespace kea::util;

namespace kea {
namespace rpc {

using RPCRecord = client::ClientContextWrapper<client::ClientContextHandler>;
using kea::client::ClientContextPtr;
using kea::client::ClientContext;
using kea::client::ClientContextHandler;

typedef folly::MPMCQueue<RPCRecord> RPCRequestQueue;

class RpcConn {
public:
    RpcConn(std::string server_addr, uint32_t port, RPCRequestQueue& in_queue);
    ~RpcConn();
    void stop();

private:
    void connectServer();
    void messageWrite();
    void writeRequestHeader(std::string request);
    void writeRequestBody(std::string request);
    void readResultHeader();
    void readResultBody();
    int marshRequset(ClientContext& ctx);
    void unMarshResult(char* result_body);

    asio::io_service io_service_;
    asio::ip::tcp::socket socket_;
    char result_len_[2];
    static const int MAX_RESULT_BODY_LEN = 1024;
    char result_body_[MAX_RESULT_BODY_LEN];
    std::thread io_loop_;
    asio::ip::tcp::resolver::iterator endpoint_iterator_;
    std::atomic<bool> stop_;
    RPCRecord rpc_request_;
    RPCRequestQueue& in_queue_;
};

class RpcAllocateEngine{
public:
    RpcAllocateEngine(std::string server_addr, uint32_t port);

    void allocateAddr(ClientContextPtr client_ctx, ClientContextHandler callback);
    void stop();

    static RpcAllocateEngine& instance();
    static void init(std::string server_addr, uint32_t port);

private:
    std::atomic<bool> stop_;
    std::unique_ptr<RPCRequestQueue> request_queue_;
    std::vector<std::unique_ptr<RpcConn>> connections_;
};

};
};
