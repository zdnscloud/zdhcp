#pragma once

#include <kea/client/client_context.h>

namespace kea {
namespace client {


template <typename T>
struct ClientContextWrapper {
    T val_;
    mutable ClientContextPtr client_ctx_;

    ClientContextWrapper():client_ctx_(nullptr){}
    
    ClientContextWrapper(ClientContextPtr client_ctx, T val) 
        : client_ctx_(std::move(client_ctx)), 
        val_(val){
    }

    ClientContextWrapper(const ClientContextWrapper& other) 
        : client_ctx_(std::move(other.client_ctx_)),
        val_(other.val_){
    }   

    ClientContextWrapper(ClientContextWrapper&& other) noexcept
        : client_ctx_(std::move(other.client_ctx_)),
        val_(other.val_){
    }   

    ClientContextWrapper& operator=(const ClientContextWrapper& other) {
        if (this != &other) {
            client_ctx_ = std::move(other.client_ctx_);
            val_ = other.val_;
        }   
        return *this;
    }   
};

};
};

