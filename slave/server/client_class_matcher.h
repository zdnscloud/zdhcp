#pragma once
#include <kea/dhcp++/dhcp4.h>
#include <kea/dhcp++/pkt.h>
#include <kea/dhcp++/option.h>

#include <functional>
#include <string>
#include <stack>

using namespace kea::dhcp;

namespace kea {
namespace server {

class UnknownOptionError : public Exception {
    public:
        UnknownOptionError(const char* file, size_t line, const char* what) :
            kea::Exception(file, line, what) { };
};

typedef std::function<bool(const Pkt&)> PktOptionMatcher;
enum class MatcherOperator {And, Or, LeftBracket, RightBracket};

class ClientClassMatcherBuilder {
    public:
        ClientClassMatcherBuilder() {}
        void clean() {
            std::stack<PktOptionMatcher>().swap(matcher_stack_);
            std::stack<MatcherOperator>().swap(operator_stack_);
        }

        PktOptionMatcher getMatcher(); 
        void addSubstringMatcher(const std::string &opt_name, int start_pos,
                int end_pos, bool equal_or_not, const std::string& expect_value);
        void addOptionExistsMatcher(const std::string &opt_name, bool expect_value);
        void addOptionValueMatcher(const std::string &opt_name, bool equal_or_not, 
                const std::string& expect_value);
        void onLeftBracket();
        void onRightBracket();
        void onAnd();
        void onOr();

    private:
        static uint16_t getOptionCode(const std::string& option_name);
        void addMatcher(PktOptionMatcher new_matcher);

        std::stack<PktOptionMatcher> matcher_stack_;
        std::stack<MatcherOperator> operator_stack_;
};

};
};
