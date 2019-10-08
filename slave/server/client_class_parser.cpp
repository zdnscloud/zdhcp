#include <functional>
#include <vector>
#include <string>
#include <boost/lexical_cast.hpp>
#include <kea/pegtl/parse.hh>
#include <kea/pegtl/rules.hh>
#include <kea/pegtl/ascii.hh>
#include <kea/pegtl/utf8.hh>
#include <kea/pegtl/utf16.hh>
#include <kea/pegtl/utf32.hh>
#include <kea/pegtl/string_parser.hh>
#include <kea/pegtl/file_parser.hh>
#include <kea/pegtl/analyze.hh>
#include <kea/server/client_class_parser.h>


namespace kea {
namespace server {
namespace internal {
using namespace pegtl;

struct logic_eql : pegtl_string_t("==") {};
struct logic_not_eql : pegtl_string_t("!=") {};
struct option_key: pegtl_string_t("option") {};
struct exist_key: pegtl_string_t("exists") {};
struct not_exist_key : pegtl_string_t("!exists") {};
struct substring_key : pegtl_string_t("substring") {};
struct and_key : pegtl_string_t("&&") {};
struct or_key: pegtl_string_t("||") {};
struct left_bracket_key: pegtl_string_t("(") {}; 
struct right_bracket_key: pegtl_string_t(")") {}; 

//pegtl pus conflict with std plus (decleared in functional)
struct option_name : list_must<pegtl::plus<alpha>, pegtl::one<'-'>> {};
struct option_code : pegtl::plus<digit> {};
struct option_with_name : seq<option_key, space, option_name> {};
struct option_with_code: seq<option_key, one<'['>, option_code, one<']'>> {};
// option host-name || option[53]
struct option_identifier : sor<option_with_name, option_with_code> {};

//exists option_identifier 
struct exists_check: seq< sor<exist_key, not_exist_key>, space, option_identifier> {};

//option dhcp-client-identifier == "xxx"
struct string_literal : if_must<one<'"'>, until<one<'"'>, pegtl::plus<not_one<'"'>>>> {};
struct option_value_check : seq <option_identifier, star<space>, sor<logic_eql, 
    logic_not_eql>, star<space>, string_literal> {};

//substring(option dhcp-client-identifier, 1, 2) == "xxx"
struct substring_pos_arg : pegtl::plus<digit> {};
struct two_number_args : list_must<substring_pos_arg, one<','>, space> {};
struct substring_func_call : seq<substring_key, one<'('>, star<space>, option_identifier, 
    star<space>, one<','>, star<space>, two_number_args, star<space>, one<')'>> {};
struct substring_check : seq<substring_func_call, star<space>, sor<logic_eql, logic_not_eql>, 
    star<space>, string_literal> {};

struct leaf_logic_check: sor<option_value_check, exists_check, substring_check> {};
struct component_logic_check : seq<left_bracket_key, star<space>, list_must<leaf_logic_check,
    sor<and_key, or_key>, space>, star<space>, right_bracket_key> {};
struct logic_check : sor<leaf_logic_check, component_logic_check> {};
struct expression: list_must<logic_check, sor<and_key, or_key>, space>{};

struct grammar: must< expression, eof > {};

template< typename Rule >
struct action : pegtl::nothing< Rule > {};

template<> struct action<and_key> {
    template< typename Input >
    static void apply(const Input& in, ClientClassMatcherBuilder& builder, std::vector<std::string>&)
    {
        builder.onAnd();
    }
};

template<> struct action<or_key> {
    template< typename Input >
    static void apply(const Input& in, ClientClassMatcherBuilder& builder, std::vector<std::string>&)
    {
        builder.onOr();
    }
};

template<> struct action<left_bracket_key> {
    template< typename Input >
    static void apply(const Input& in, ClientClassMatcherBuilder& builder, std::vector<std::string>&)
    {
        builder.onLeftBracket();
    }
};

template<> struct action<right_bracket_key> {
    template< typename Input >
    static void apply(const Input& in, ClientClassMatcherBuilder& builder, std::vector<std::string>&)
    {
        builder.onRightBracket();
    }
};

template<> struct action<logic_eql> {
    template< typename Input >
    static void apply(const Input& in, ClientClassMatcherBuilder&, std::vector<std::string>& tokens)
    {
        tokens.push_back("y");
    }
};

template<> struct action<logic_not_eql> {
    template< typename Input >
    static void apply(const Input& in, ClientClassMatcherBuilder&, std::vector<std::string>& tokens)
    {
        tokens.push_back("n");
    }
};

template<> struct action<option_name> {
    template< typename Input >
    static void apply(const Input& in, ClientClassMatcherBuilder&, std::vector<std::string>& tokens)
    {
        tokens.push_back(in.string());
    }
};

template<> struct action<option_code> {
    template< typename Input >
    static void apply(const Input& in, ClientClassMatcherBuilder&, std::vector<std::string>& tokens)
    {
        tokens.push_back(in.string());
    }
};

template<> struct action<string_literal> {
    template< typename Input >
    static void apply(const Input& in, ClientClassMatcherBuilder&, std::vector<std::string>& tokens)
    {
        std::string value = in.string();
        if (value.empty() || value.size() < 2 || value.front() != '"' || value.back() != '"') {
            kea_throw(StringLiteralFormatError, "string should be surround by quote");
        }

        tokens.push_back(value.substr(1, value.size() - 2));
    }
};

template<> struct action<exist_key> {
    template< typename Input >
    static void apply(const Input& in, ClientClassMatcherBuilder&, std::vector<std::string>& tokens)
    {
        tokens.push_back("y");
    }
};

template<> struct action<not_exist_key> {
    template< typename Input >
    static void apply(const Input& in, ClientClassMatcherBuilder&, std::vector<std::string>& tokens)
    {
        tokens.push_back("n");
    }
};

template<> struct action<substring_pos_arg> {
    template< typename Input >
    static void apply(const Input& in, ClientClassMatcherBuilder& , std::vector<std::string>& tokens)
    {
        tokens.push_back(in.string());
    }
};


template<> struct action<option_value_check> {
    template< typename Input >
    static void apply(const Input& in, ClientClassMatcherBuilder& builder, std::vector<std::string>& tokens)
    {
        builder.addOptionValueMatcher(tokens[0], tokens[1] == "y", tokens[2]);
        tokens.clear();
    }
};

template<> struct action<exists_check> {
    template< typename Input >
    static void apply(const Input& in, ClientClassMatcherBuilder& builder, std::vector<std::string>& tokens)
    {
        builder.addOptionExistsMatcher(tokens[1], tokens[0] == "y");
        tokens.clear();
    }
};

template<> struct action<substring_check> {
    template< typename Input >
    static void apply(const Input& in, ClientClassMatcherBuilder& builder, std::vector<std::string>& tokens)
    {
        builder.addSubstringMatcher(tokens[0], 
                boost::lexical_cast<int>(tokens[1]),
                boost::lexical_cast<int>(tokens[2]),
                tokens[3] == "y",
                tokens[4]);
        tokens.clear();
    }
};
};


PktOptionMatcher buildMatcher(const std::string& exp) {
    std::vector<std::string> tokens;
    ClientClassMatcherBuilder builder;
    pegtl::parse_string< internal::grammar, internal::action >(exp, __FILE__, builder, tokens);
    return builder.getMatcher();
}

};
};
