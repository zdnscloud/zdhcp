#pragma once

#include <algorithm>
#include <cctype>
#include <stdint.h>
#include <string>
#include <sstream>
#include <vector>
#include <kea/exceptions/exceptions.h>
#include <boost/lexical_cast.hpp>

namespace kea {
namespace util {
namespace str {

class StringTokenError : public Exception {
public:
    StringTokenError(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) {}
};


/// \brief Trim Leading and Trailing Spaces
std::string trim(const std::string& instring);


/// \brief Split String into Tokens
std::vector<std::string> tokens(const std::string& text,
        const std::string& delim = std::string(" \t\n"));


/// \brief Uppercase Character
inline char toUpper(char chr) {
    return (static_cast<char>(std::toupper(static_cast<int>(chr))));
}

inline void uppercase(std::string& text) {
    std::transform(text.begin(), text.end(), text.begin(),
        kea::util::str::toUpper);
}

/// \brief Lowercase Character
inline char toLower(char chr) {
    return (static_cast<char>(std::tolower(static_cast<int>(chr))));
}

/// \brief Lowercase String
inline void lowercase(std::string& text) {
    std::transform(text.begin(), text.end(), text.begin(),
        kea::util::str::toLower);
}


/// \brief Apply Formatting
std::string format(const std::string& format,
    const std::vector<std::string>& args);


/// \brief Returns one token from the given stringstream
std::string getToken(std::istringstream& iss);

/// \brief Converts a string token to an *unsigned* integer.
template <typename NumType, int BitSize>
NumType
tokenToNum(const std::string& num_token) {
    NumType num;
    try {
        num = boost::lexical_cast<NumType>(num_token);
    } catch (const boost::bad_lexical_cast&) {
        kea_throw(StringTokenError, "Invalid SRV numeric parameter: " <<
                  num_token);
    }
    if (num < 0 || num >= (static_cast<NumType>(1) << BitSize)) {
        kea_throw(StringTokenError, "Numeric SRV parameter out of range: " <<
                  num);
    }
    return (num);
}

/// \brief Converts a string in quotes into vector.
std::vector<uint8_t>
quotedStringToBinary(const std::string& quoted_string);

/// \brief Converts a string of hexadecimal digits with colons into
void
decodeColonSeparatedHexString(const std::string& hex_string,
                              std::vector<uint8_t>& binary);

/// \brief Converts a formatted string of hexadecimal digits into
void
decodeFormattedHexString(const std::string& hex_string,
                         std::vector<uint8_t>& binary);

};
};
};
