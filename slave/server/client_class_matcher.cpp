#include <kea/server/client_class_matcher.h>
#include <kea/dhcp++/std_option_defs.h>
#include <boost/lexical_cast.hpp>

#include <cassert>

namespace kea {
namespace server {

static PktOptionMatcher substringMatcherGenerator(uint16_t code, int start_pos, 
        int end_pos, bool equal_or_not, const std::string& expect_value) {
    return [=](const Pkt& pkt){
        const Option* option = pkt.getOption(code);
        if (option == nullptr) {
            return (false);
        }
        return ((option->toString().substr(start_pos, end_pos) == expect_value) == equal_or_not);
    };
}

static PktOptionMatcher optionExistsMatcherGenerator(uint16_t code, 
        bool expect_value) {
    return [=](const Pkt& pkt){
        const Option* option = pkt.getOption(code);
        return (expect_value ? option != nullptr : option == nullptr); 
    };
}
 
static PktOptionMatcher optionValueMatcherGenerator(uint16_t code, bool equal_or_not, 
        const std::string& expect_value) {
    return [=](const Pkt& pkt){
        const Option* option = pkt.getOption(code);
        if (option == nullptr) {
            return (false);
        }

        return ((option->toString() == expect_value) == equal_or_not);
    };
}

static PktOptionMatcher andMatcher(PktOptionMatcher f1, PktOptionMatcher f2) {
    return [=](const Pkt& pkt){
        return (f1(pkt) && f2(pkt));
    };
}

static PktOptionMatcher orMatcher(PktOptionMatcher f1, PktOptionMatcher f2) {
    return [=](const Pkt& pkt){
        return (f1(pkt) || f2(pkt));
    };
}

void ClientClassMatcherBuilder::addSubstringMatcher(const std::string &opt_name, 
        int start_pos, int end_pos, bool equal_or_not, const std::string& expect_value) {
    addMatcher(substringMatcherGenerator(getOptionCode(opt_name), start_pos, end_pos, equal_or_not, expect_value));
};

void ClientClassMatcherBuilder::addMatcher(PktOptionMatcher new_matcher) {
    if (matcher_stack_.empty()) {
        matcher_stack_.push(new_matcher);
    } else {
        switch (operator_stack_.top()) {
            case MatcherOperator::And:
                new_matcher = andMatcher(matcher_stack_.top(), new_matcher);
                matcher_stack_.pop();
                matcher_stack_.push(new_matcher);
                operator_stack_.pop();
                break;
            case MatcherOperator::Or:
                new_matcher = orMatcher(matcher_stack_.top(), new_matcher);
                matcher_stack_.pop();
                matcher_stack_.push(new_matcher);
                operator_stack_.pop();
                break;
            case MatcherOperator::LeftBracket:
                matcher_stack_.push(new_matcher);
                break;
            default:
                assert(0); //syntax parse should avoid this
        }
    }
}

uint16_t ClientClassMatcherBuilder::getOptionCode(const std::string& opt_name) {
    const OptionDefinition *def = nullptr;
    try {
        uint16_t code = boost::lexical_cast<uint16_t>(opt_name);
        def = kea::dhcp::getStdV4Options().getOptionDef(code);
    } catch(boost::bad_lexical_cast &) {
        def = kea::dhcp::getStdV4Options().getOptionDef(opt_name);
    }

    if (def == nullptr) {
        kea_throw(UnknownOptionError, opt_name);
    }
    return def->getCode();
}

void ClientClassMatcherBuilder::addOptionExistsMatcher(const std::string &opt_name, 
        bool expect_value) {
    addMatcher(optionExistsMatcherGenerator(getOptionCode(opt_name), expect_value));
}

void ClientClassMatcherBuilder::addOptionValueMatcher(const std::string &opt_name, 
        bool equal_or_not, const std::string& expect_value) {
    addMatcher(optionValueMatcherGenerator(getOptionCode(opt_name), equal_or_not, expect_value));
}

void ClientClassMatcherBuilder::onLeftBracket() {
    operator_stack_.push(MatcherOperator::LeftBracket);
}

void ClientClassMatcherBuilder::onRightBracket() {
    assert(operator_stack_.top() == MatcherOperator::LeftBracket);
    operator_stack_.pop();

    PktOptionMatcher component_matcher = matcher_stack_.top();
    matcher_stack_.pop();
    addMatcher(component_matcher);
}

void ClientClassMatcherBuilder::onAnd() {
    operator_stack_.push(MatcherOperator::And);
}

void ClientClassMatcherBuilder::onOr() {
    operator_stack_.push(MatcherOperator::Or);
}

PktOptionMatcher ClientClassMatcherBuilder::getMatcher() {
    assert(matcher_stack_.size() == 1);
    assert(operator_stack_.empty());
    return matcher_stack_.top();
}

};
};
