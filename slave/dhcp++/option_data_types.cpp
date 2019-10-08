#include <kea/dhcp++/option_data_types.h>
#include <kea/dns/labelsequence.h>
#include <kea/dns/name.h>
#include <kea/util/encode/hex.h>

namespace kea {
namespace dhcp {

static std::unordered_map<std::string, OptionDataType> StringDataTypeMap = {
    {"empty",OPT_EMPTY_TYPE},
    {"binary",OPT_BINARY_TYPE},
    {"boolean",OPT_BOOLEAN_TYPE},
    {"int8",OPT_INT8_TYPE},
    {"int16",OPT_INT16_TYPE},
    {"int32",OPT_INT32_TYPE},
    {"uint8",OPT_UINT8_TYPE},
    {"uint16",OPT_UINT16_TYPE},
    {"uint32",OPT_UINT32_TYPE},
    {"ipv4-address",OPT_IPV4_ADDRESS_TYPE},
    {"string",OPT_STRING_TYPE},
    {"fqdn",OPT_FQDN_TYPE},
    {"record",OPT_RECORD_TYPE},
};

static std::unordered_map<OptionDataType, std::string> DataTypeStringMap = {
    {OPT_BOOLEAN_TYPE, "boolean"},
    {OPT_INT8_TYPE, "int8"},
    {OPT_INT16_TYPE, "int16"},
    {OPT_INT32_TYPE, "int32"},
    {OPT_UINT8_TYPE, "uint8"},
    {OPT_UINT16_TYPE, "uint16"},
    {OPT_UINT32_TYPE, "uint32"},
    {OPT_IPV4_ADDRESS_TYPE, "ipv4-address"},
    {OPT_STRING_TYPE, "string"},
    {OPT_FQDN_TYPE, "fqdn"},
    {OPT_RECORD_TYPE, "record"},
    {OPT_UNKNOWN_TYPE, "unknown"},
};

OptionDataType OptionDataTypeUtil::getDataType(const std::string& name) {
    auto it = StringDataTypeMap.find(name);
    if (it != StringDataTypeMap.end()) {
        return (it->second);
    } else {
        return (OPT_UNKNOWN_TYPE);
    }
}

int OptionDataTypeUtil::getDataTypeLen(const OptionDataType data_type) {
    switch (data_type) {
    case OPT_BOOLEAN_TYPE:
    case OPT_INT8_TYPE:
    case OPT_UINT8_TYPE:
        return (1);

    case OPT_INT16_TYPE:
    case OPT_UINT16_TYPE:
        return (2);

    case OPT_INT32_TYPE:
    case OPT_UINT32_TYPE:
        return (4);

    case OPT_IPV4_ADDRESS_TYPE:
        return (V4ADDRESS_LEN);

    default:
        return (0);
    }
}

const std::string& OptionDataTypeUtil::getDataTypeName(const OptionDataType data_type) {
    auto it = DataTypeStringMap.find(data_type);
    if (it != DataTypeStringMap.end()) {
        return (it->second);
    } else {
        return (DataTypeStringMap.at(OPT_UNKNOWN_TYPE));
    }
}

IOAddress OptionDataTypeUtil::readAddress(const std::vector<uint8_t>& buf,
                                const short family) {
    if (family == AF_INET) {
        if (buf.size() < V4ADDRESS_LEN) {
            kea_throw(BadDataTypeCast, "unable to read data from the buffer as"
                      << " IPv4 address. Invalid buffer size: " << buf.size());
        }
        return IOAddress::fromBytes(family, buf.data());
    } else {
        kea_throw(BadDataTypeCast, "unable to read data from the buffer as"
                  "IP address. Invalid family: " << family);
    }
}

void OptionDataTypeUtil::writeAddress(const IOAddress& address,
        std::vector<uint8_t>& buf) {
    if (address.isV4()) {
        auto binary = address.toBytes();
        buf.insert(buf.end(), binary.begin(), binary.end());
    }
}

void OptionDataTypeUtil::writeBinary(const std::string& hex_str,
        std::vector<uint8_t>& buf) {
    vector<uint8_t> binary;
    try {
        util::encode::decodeHex(hex_str, binary);
    } catch (const Exception& ex) {
        kea_throw(BadDataTypeCast, "unable to cast " << hex_str
                  << " to binary data type: " << ex.what());
    }
    buf.insert(buf.end(), binary.begin(), binary.end());
}

bool OptionDataTypeUtil::readBool(const std::vector<uint8_t>& buf) {
    if (buf.empty()) {
        kea_throw(BadDataTypeCast, "unable to read the buffer as boolean"
                << " value. Invalid buffer size " << buf.size());
    }
    if (buf[0] == 1) {
        return (true);
    } else if (buf[0] == 0) {
        return (false);
    }
    kea_throw(BadDataTypeCast, "unable to read the buffer as boolean"
              << " value. Invalid value " << static_cast<int>(buf[0]));
}

void OptionDataTypeUtil::writeBool(const bool value,
        std::vector<uint8_t>& buf) {
    buf.push_back(static_cast<uint8_t>(value ? 1 : 0));
}

std::string OptionDataTypeUtil::readFqdn(const std::vector<uint8_t>& buf) {
    if (buf.empty()) {
        kea_throw(BadDataTypeCast, "unable to read FQDN from a buffer."
                << " The buffer is empty.");
    }

    kea::util::InputBuffer in_buf(static_cast<const void*>(&buf[0]), buf.size());
    try {
        kea::dns::Name name(in_buf);
        return (name.toText());
    } catch (const kea::Exception& ex) {
        kea_throw(BadDataTypeCast, ex.what());
    }
}

void OptionDataTypeUtil::writeFqdn(const std::string& fqdn,
        std::vector<uint8_t>& buf, bool downcase) {
    try {
        kea::dns::Name name(fqdn, downcase);
        kea::dns::LabelSequence labels(name);
        if (labels.getDataLength() > 0) {
            size_t read_len = 0;
            const uint8_t* data = labels.getData(&read_len);
            buf.insert(buf.end(), data, data + read_len);
        }
    } catch (const kea::Exception& ex) {
        kea_throw(BadDataTypeCast, ex.what());
    }
}

unsigned int OptionDataTypeUtil::getLabelCount(const std::string& text_name) {
    if (text_name.empty()) {
        return (0);
    }

    try {
        kea::dns::Name name(text_name);
        return (name.getLabelCount());
    } catch (const kea::Exception& ex) {
        kea_throw(BadDataTypeCast, ex.what());
    }
}

std::string OptionDataTypeUtil::readString(const std::vector<uint8_t>& buf) {
    std::string value;
    if (!buf.empty()) {
        value.insert(value.end(), buf.begin(), buf.end());
    }
    return (value);
}

void OptionDataTypeUtil::writeString(const std::string& value,
        std::vector<uint8_t>& buf) {
    if (value.size() > 0) {
        buf.insert(buf.end(), value.begin(), value.end());
    }
}

};
};
