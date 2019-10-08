#pragma once

#include <kea/exceptions/exceptions.h>
#include <kea/util/io_utilities.h>
#include <kea/util/io_address.h>

#include <cstdint>
#include <type_traits>
#include <unordered_map>

using namespace std;
using namespace kea::util;

namespace kea {
namespace dhcp {

class InvalidDataType : public Exception {
public:
    InvalidDataType(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) { };
};

class BadDataTypeCast : public Exception {
public:
    BadDataTypeCast(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) { };
};


enum OptionDataType {
    OPT_EMPTY_TYPE,
    OPT_BINARY_TYPE,
    OPT_BOOLEAN_TYPE,
    OPT_INT8_TYPE,
    OPT_INT16_TYPE,
    OPT_INT32_TYPE,
    OPT_UINT8_TYPE,
    OPT_UINT16_TYPE,
    OPT_UINT32_TYPE,
    OPT_ANY_ADDRESS_TYPE,
    OPT_IPV4_ADDRESS_TYPE,
    OPT_STRING_TYPE,
    OPT_FQDN_TYPE,
    OPT_RECORD_TYPE,
    OPT_UNKNOWN_TYPE
};

template<typename T>
struct OptionDataTypeTraits {
    static const OptionDataType type = OPT_UNKNOWN_TYPE;
};

template<>
struct OptionDataTypeTraits<int8_t> {
    static const OptionDataType type = OPT_INT8_TYPE;
};

template<>
struct OptionDataTypeTraits<int16_t> {
    static const OptionDataType type = OPT_INT16_TYPE;
};

template<>
struct OptionDataTypeTraits<int32_t> {
    static const OptionDataType type = OPT_INT32_TYPE;
};

template<>
struct OptionDataTypeTraits<uint8_t> {
    static const OptionDataType type = OPT_UINT8_TYPE;
};

template<>
struct OptionDataTypeTraits<uint16_t> {
    static const OptionDataType type = OPT_UINT16_TYPE; 
}; 

template<>
struct OptionDataTypeTraits<uint32_t> {
    static const OptionDataType type = OPT_UINT32_TYPE; 
}; 

class OptionDataTypeUtil {
    public:

    static OptionDataType getDataType(const std::string& data_type);
    static const std::string& getDataTypeName(const OptionDataType data_type);

    static int getDataTypeLen(const OptionDataType data_type);
    static IOAddress readAddress(const std::vector<uint8_t>& buf,
                                           const short family);

    static void writeAddress(const IOAddress& address,
                             std::vector<uint8_t>& buf);
    static void writeBinary(const std::string& hex_str,
                            std::vector<uint8_t>& buf);

    static bool readBool(const std::vector<uint8_t>& buf);
    static void writeBool(const bool value, std::vector<uint8_t>& buf);

    template<typename T>
    static T readInt(const std::vector<uint8_t>& buf) {
        if (std::is_integral<T>::value != 1) {
            kea_throw(kea::dhcp::InvalidDataType, "specified data type to be returned"
                      " by readInteger is unsupported integer type");
        }

        size_t data_len = sizeof(T);
        if (buf.size() < data_len) {
            kea_throw(kea::dhcp::BadDataTypeCast,
                      "failed to read an integer value from a buffer"
                      << " - buffer is truncated.");
        }

        T value;
        switch (data_len) {
        case 1:
            value = *(buf.begin());
            break;
        case 2:
            value = kea::util::readUint16(&(*buf.begin()), buf.size());
            break;
        case 4:
            value = kea::util::readUint32(&(*buf.begin()), buf.size());
            break;
        default:
            kea_throw(kea::dhcp::InvalidDataType,
                      "invalid size of the data type to be read as integer.");
        }
        return (value);
    }

    template<typename T>
    static void writeInt(const T value,
                         std::vector<uint8_t>& buf) {
        if (std::is_integral<T>::value != 1) {
            kea_throw(InvalidDataType, "provided data type is not the supported.");
        }

        size_t data_len = sizeof(T);
        switch (data_len) {
        case 1:
            buf.push_back(static_cast<uint8_t>(value));
            break;
        case 2:
            buf.resize(buf.size() + 2);
            kea::util::writeUint16(static_cast<uint16_t>(value), &buf[buf.size() - 2], 2);
            break;
        case 4:
            buf.resize(buf.size() + 4);
            kea::util::writeUint32(static_cast<uint32_t>(value), &buf[buf.size() - 4], 4);
            break;
        default:
            kea_throw(InvalidDataType, "provided data type is not the supported.");
        }
    }

    static std::string readFqdn(const std::vector<uint8_t>& buf);
    static void writeFqdn(const std::string& fqdn,
                          std::vector<uint8_t>& buf,
                          const bool downcase = false);
    static unsigned int getLabelCount(const std::string& text_name);

    static std::string readString(const std::vector<uint8_t>& buf);
    static void writeString(const std::string& value,
                            std::vector<uint8_t>& buf);
};


};
}; 

namespace std {
    template<> struct hash<kea::dhcp::OptionDataType> {
        typedef kea::dhcp::OptionDataType argument_type;
        typedef std::size_t result_type;
        result_type operator()(argument_type const& t) const {
            return (size_t(t));
        }
    };
};
