#pragma once 

#include <string>
#include <functional>

namespace kea{
namespace controller{

class CmdServerBase {
    public:
        virtual ~CmdServerBase() {}

        virtual std::string processCmd(const char* cmd_msg, uint32_t cmd_len, std::function<void(bool)> callback) = 0;
        virtual void stop() = 0;
};

}
}
