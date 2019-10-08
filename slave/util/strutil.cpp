#include <kea/util/encode/hex.h>
#include <kea/util/strutil.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/constants.hpp>
#include <boost/algorithm/string/split.hpp>

#include <numeric>
#include <sstream>
#include <cstring>


using namespace std;

namespace kea {
namespace util {
namespace str {

string trim(const string& instring) {
    string retstring = "";
    if (!instring.empty()) {
        static const char* blanks = " \t\n";

        size_t first = instring.find_first_not_of(blanks);
        if (first != string::npos) {

            size_t last = instring.find_last_not_of(blanks);

            retstring = instring.substr(first, (last - first + 1));
        }
    }

    return (retstring);
}

vector<string> tokens(const std::string& text, const std::string& delim) {
    vector<string> result;

    size_t start = text.find_first_not_of(delim);
    while (start != string::npos) {

        size_t end = text.find_first_of(delim, start);
        if (end != string::npos) {
            result.push_back(text.substr(start, (end - start)));
            start = text.find_first_not_of(delim, end);
        } else {
            result.push_back(text.substr(start));
            start = string::npos;
        }
    }

    return std::move(result);
}

namespace {
size_t
lengthSum(string::size_type curlen, const string& cur_string) {
    return (curlen + cur_string.size());
}
}

std::string format(const std::string& format, const std::vector<std::string>& args) {
    static const string flag = "%s";
    string result;
    size_t length = accumulate(args.begin(), args.end(), format.size(),
        lengthSum) - (args.size() * flag.size());
    result.reserve(length);

    result = format;
    size_t tokenpos = 0; 
    std::vector<std::string>::size_type i = 0;

    while ((i < args.size()) && (tokenpos != string::npos)) {
        tokenpos = result.find(flag, tokenpos);
        if (tokenpos != string::npos) {
            result.replace(tokenpos, flag.size(), args[i++]);
        }
    }

    return (result);
}

std::string getToken(std::istringstream& iss) {
    string token;
    iss >> token;
    if (iss.bad() || iss.fail()) {
        kea_throw(StringTokenError, "could not read token from string");
    }
    return (token);
}

std::vector<uint8_t> quotedStringToBinary(const std::string& quoted_string) {
    std::vector<uint8_t> binary;
    std::string trimmed_string = trim(quoted_string);

    if ((trimmed_string.length() > 1) && ((trimmed_string[0] == '\'') &&
        (trimmed_string[trimmed_string.length()-1] == '\''))) {
        trimmed_string = trim(trimmed_string.substr(1, trimmed_string.length() - 2));
        binary.assign(trimmed_string.begin(), trimmed_string.end());
    }
    return (std::move(binary));
}

void decodeColonSeparatedHexString(const std::string& hex_string,
        std::vector<uint8_t>& binary) {
    std::vector<std::string> split_text;
    boost::split(split_text, hex_string, boost::is_any_of(":"),
                 boost::algorithm::token_compress_off);

    std::vector<uint8_t> binary_vec;
    for (size_t i = 0; i < split_text.size(); ++i) {

        if ((split_text.size() > 1) && split_text[i].empty()) {
            kea_throw(kea::BadValue, "two consecutive colons specified in"
                      " a decoded string '" << hex_string << "'");

        } else if (split_text[i].size() > 2) {
            kea_throw(kea::BadValue, "invalid format of the decoded string"
                      << " '" << hex_string << "'");

        } else if (!split_text[i].empty()) {
            std::stringstream s;
            s << "0x";

            for (unsigned int j = 0; j < split_text[i].length(); ++j) {
                if (!isxdigit(split_text[i][j])) {
                    kea_throw(kea::BadValue, "'" << split_text[i][j]
                              << "' is not a valid hexadecimal digit in"
                              << " decoded string '" << hex_string << "'");
                }
                s << split_text[i][j];
            }

            unsigned int binary_value;
            s >> std::hex >> binary_value;

            binary_vec.push_back(static_cast<uint8_t>(binary_value));
        }

    }

    binary.swap(binary_vec);
}

void decodeFormattedHexString(const std::string& hex_string,
        std::vector<uint8_t>& binary) {
    if (hex_string.find(':') != std::string::npos) {
        decodeColonSeparatedHexString(hex_string, binary);

    } else {
        std::ostringstream s;
        if (hex_string.length() % 2 != 0) {
            s << "0";
        }

        if ((hex_string.length() > 2) && (hex_string.substr(0, 2) == "0x")) {
            s << hex_string.substr(2);
        } else {
            s << hex_string;
        }

        try {
            encode::decodeHex(s.str(), binary);
        } catch (...) {
            kea_throw(kea::BadValue, "'" << hex_string << "' is not a valid"
                      " string of hexadecimal digits");
        }
    }
}

}; 
};
};
