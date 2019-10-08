#include <kea/dhcp++/option_data_types.h>
#include <kea/dhcp++/option_custom.h>
#include <kea/util/encode/hex.h>

namespace kea {
namespace dhcp {

OptionCustom::OptionCustom(const OptionDefinition& def)
    : Option(def.getCode(), OptionBuffer()), definition_(def) {
    setEncapsulatedSpace(def.getEncapsulatedSpace());
    createBuffers();
}

OptionCustom::OptionCustom(const OptionDefinition& def,
        const OptionBuffer& data)
    : Option(def.getCode(), data.begin(), data.end()),
      definition_(def) {
    setEncapsulatedSpace(def.getEncapsulatedSpace());
    createBuffers(getData());
}

OptionCustom::OptionCustom(const OptionDefinition& def,
        OptionBufferConstIter first,
        OptionBufferConstIter last)
    : Option(def.getCode(), first, last),
      definition_(def) {
    setEncapsulatedSpace(def.getEncapsulatedSpace());
    createBuffers(getData());
}

std::unique_ptr<Option> OptionCustom::clone() const {
    return (cloneInternal<OptionCustom>());
}

void OptionCustom::addArrayDataField(const IOAddress& address) {
    checkArrayType();
    if (address.isV4() && definition_.getType() != OPT_IPV4_ADDRESS_TYPE) {
        kea_throw(BadDataTypeCast, "invalid address specified "
                << address << ". Expected a valid IPv4 address.");
    }

    OptionBuffer buf;
    OptionDataTypeUtil::writeAddress(address, buf);
    buffers_.push_back(buf);
}

void OptionCustom::addArrayDataField(const bool value) {
    checkArrayType();
    OptionBuffer buf;
    OptionDataTypeUtil::writeBool(value, buf);
    buffers_.push_back(buf);
}

void OptionCustom::checkIndex(const uint32_t index) const {
    if (index >= buffers_.size()) {
        kea_throw(kea::OutOfRange, "specified data field index " << index
                  << " is out of range.");
    }
}

void OptionCustom::createBuffers() {
    definition_.validate();
    std::vector<OptionBuffer> buffers;
    OptionDataType data_type = definition_.getType();
    if (data_type == OPT_RECORD_TYPE) {
        for (auto &field : definition_.getRecordFields()) {
            OptionBuffer buf;
            size_t data_size = OptionDataTypeUtil::getDataTypeLen(field);
            if (data_size == 0 &&
                field == OPT_FQDN_TYPE) {
                OptionDataTypeUtil::writeFqdn(".", buf);
            } else {
                buf.resize(data_size);
            }
            buffers.push_back(buf);
        }
    } else if (!definition_.isArrayType() &&
            data_type != OPT_EMPTY_TYPE) {
        OptionBuffer buf;
        size_t data_size = OptionDataTypeUtil::getDataTypeLen(data_type);
        if (data_size == 0 &&
            data_type == OPT_FQDN_TYPE) {
            OptionDataTypeUtil::writeFqdn(".", buf);
        } else {
            buf.resize(data_size);
        }
        buffers.push_back(buf);
    }
    std::swap(buffers, buffers_);
}

void OptionCustom::createBuffers(const OptionBuffer& data_buf) {
    definition_.validate();
    std::vector<OptionBuffer> buffers;
    OptionBuffer::const_iterator data = data_buf.begin();

    OptionDataType data_type = definition_.getType();
    if (data_type == OPT_RECORD_TYPE) {
        for (auto field : definition_.getRecordFields()) {
            size_t data_size = OptionDataTypeUtil::getDataTypeLen(field);
            if (data_size == 0) {
                if (field == OPT_FQDN_TYPE) {
                    std::string fqdn =
                        OptionDataTypeUtil::readFqdn(OptionBuffer(data, data_buf.end()));
                    data_size = fqdn.size() + 1;
                } else if ((field == OPT_BINARY_TYPE) || (field == OPT_STRING_TYPE)) {
                    data_size = std::distance(data, data_buf.end());
                } else {
                    kea_throw(OutOfRange, "option buffer truncated");
                }
            } else {
                if (std::distance(data, data_buf.end()) < data_size) {
                    kea_throw(OutOfRange, "option buffer truncated");
                }
            }
            buffers.push_back(OptionBuffer(data, data + data_size));
            data += data_size;
        }

        if (data != data_buf.end() && !getEncapsulatedSpace().empty()) {
            unpackOptions(OptionBuffer(data, data_buf.end()));
        }

    } else if (data_type != OPT_EMPTY_TYPE) {
        size_t data_size = OptionDataTypeUtil::getDataTypeLen(data_type);
        if (std::distance(data, data_buf.end()) < data_size) {
            kea_throw(OutOfRange, "option buffer truncated");
        }
        if (definition_.isArrayType()) {
            while (data != data_buf.end()) {
                if (data_type == OPT_FQDN_TYPE) {
                    std::string fqdn =
                        OptionDataTypeUtil::readFqdn(OptionBuffer(data, data_buf.end()));
                    data_size = fqdn.size() + 1;
                }
                assert(data_size > 0);
                if (std::distance(data, data_buf.end()) < data_size) {
                    break;
                }
                buffers.push_back(OptionBuffer(data, data + data_size));
                data += data_size;
            }
        } else {
            if (data_size == 0) {
                if (data_type == OPT_FQDN_TYPE) {
                    std::string fqdn =
                        OptionDataTypeUtil::readFqdn(OptionBuffer(data, data_buf.end()));
                    data_size = fqdn.size() + 1;
                } else {
                    data_size = std::distance(data, data_buf.end());
                }
            }
            if (data_size > 0) {
                buffers.push_back(OptionBuffer(data, data + data_size));
                data += data_size;
            } else {
                kea_throw(OutOfRange, "option buffer truncated");
            }

            if (data != data_buf.end() && !getEncapsulatedSpace().empty()) {
                unpackOptions(OptionBuffer(data, data_buf.end()));
            }
        }
    } else if (data_type == OPT_EMPTY_TYPE) {
        if (data != data_buf.end() && !getEncapsulatedSpace().empty()) {
            unpackOptions(OptionBuffer(data, data_buf.end()));
        }
    }
    std::swap(buffers_, buffers);
}

std::string OptionCustom::dataFieldToText(const OptionDataType data_type,
        const uint32_t index) const {
    std::ostringstream text;
    switch (data_type) {
    case OPT_BINARY_TYPE:
        text << util::encode::encodeHex(readBinary(index));
        break;
    case OPT_BOOLEAN_TYPE:
        text << (readBoolean(index) ? "true" : "false");
        break;
    case OPT_INT8_TYPE:
        text << static_cast<int>(readInteger<int8_t>(index));
        break;
    case OPT_INT16_TYPE:
        text << readInteger<int16_t>(index);
        break;
    case OPT_INT32_TYPE:
        text << readInteger<int32_t>(index);
        break;
    case OPT_UINT8_TYPE:
        text << static_cast<unsigned>(readInteger<uint8_t>(index));
        break;
    case OPT_UINT16_TYPE:
        text << readInteger<uint16_t>(index);
        break;
    case OPT_UINT32_TYPE:
        text << readInteger<uint32_t>(index);
        break;
    case OPT_IPV4_ADDRESS_TYPE:
        text << readAddress(index);
        break;
    case OPT_FQDN_TYPE:
        text << "\"" << readFqdn(index) << "\"";
        break;
    case OPT_STRING_TYPE:
        text << "\"" << readString(index) << "\"";
        break;
    default:
        ;
    }

    text << " (" << OptionDataTypeUtil::getDataTypeName(data_type) << ")";
    return (text.str());
}

void OptionCustom::pack(kea::util::OutputBuffer& buf) const {
    packHeader(buf);
    for (auto& buffer : buffers_) {
        if (!buffer.empty()) {
            buf.writeData(&(buffer)[0], buffer.size());
        }
    }
    packOptions(buf);
}

IOAddress OptionCustom::readAddress(const uint32_t index) const {
    checkIndex(index);
    if (buffers_[index].size() == V4ADDRESS_LEN) {
        return (OptionDataTypeUtil::readAddress(buffers_[index], AF_INET));
    } else {
        kea_throw(BadDataTypeCast, "unable to read data from the buffer as"
                << " IP address. Invalid buffer length "
                << buffers_[index].size() << ".");
    }
}

void OptionCustom::writeAddress(const IOAddress& address,
                           const uint32_t index) {
    checkIndex(index);
    if (address.isV4() && buffers_[index].size() != V4ADDRESS_LEN) {
        kea_throw(BadDataTypeCast, "invalid address specified "
                  << address << ". Expected a valid IPv4 address.");
    }

    OptionBuffer buf;
    OptionDataTypeUtil::writeAddress(address, buf);
    std::swap(buf, buffers_[index]);
}

const OptionBuffer& OptionCustom::readBinary(const uint32_t index) const {
    checkIndex(index);
    return (buffers_[index]);
}

void OptionCustom::writeBinary(const OptionBuffer& buf,
                          const uint32_t index) {
    checkIndex(index);
    buffers_[index] = buf;
}

bool OptionCustom::readBoolean(const uint32_t index) const {
    checkIndex(index);
    return (OptionDataTypeUtil::readBool(buffers_[index]));
}

void OptionCustom::writeBoolean(const bool value, const uint32_t index) {
    checkIndex(index);
    buffers_[index].clear();
    OptionDataTypeUtil::writeBool(value, buffers_[index]);
}

std::string OptionCustom::readFqdn(const uint32_t index) const {
    checkIndex(index);
    return (OptionDataTypeUtil::readFqdn(buffers_[index]));
}

void OptionCustom::writeFqdn(const std::string& fqdn, const uint32_t index) {
    checkIndex(index);
    OptionBuffer buf;
    OptionDataTypeUtil::writeFqdn(fqdn, buf);
    std::swap(buffers_[index], buf);
}

std::string OptionCustom::readString(const uint32_t index) const {
    checkIndex(index);
    return (OptionDataTypeUtil::readString(buffers_[index]));
}

void OptionCustom::writeString(const std::string& text, const uint32_t index) {
    checkIndex(index);
    buffers_[index].clear();
    if (!text.empty()) {
        OptionDataTypeUtil::writeString(text, buffers_[index]);
    }
}

void OptionCustom::unpack(OptionBufferConstIter begin,
                     OptionBufferConstIter end) {
    initialize(begin, end);
}

uint16_t OptionCustom::len() const {
    size_t length = getHeaderLen();
    // ... lengths of all buffers that hold option data ...
    for (auto& buffer : buffers_) {
        length += buffer.size();
    }

    // ... and lengths of all suboptions
    for (auto& pair : options_) {
        length += pair.second->len();
    }
    return (static_cast<uint16_t>(length));
}

void OptionCustom::initialize(const OptionBufferConstIter first,
        const OptionBufferConstIter last) {
    setData(first, last);
    createBuffers(getData());
}

std::string OptionCustom::toString() const {
    int indent = 0;
    std::stringstream output;
    OptionDataType data_type = definition_.getType();
    if (data_type == OPT_RECORD_TYPE) {
        const OptionDefinition::RecordFieldsCollection& fields =
            definition_.getRecordFields();

        // For record types we iterate over fields defined in
        // option definition and match the appropriate buffer
        // with them.
        for (auto field = fields.begin(); field != fields.end(); ++field) {
            if (field != fields.begin())
                output << ",";
            output << dataFieldToString(*field, std::distance(fields.begin(), field));
        }
    } else {
        for (unsigned int i = 0; i < getDataFieldsNum(); ++i) {
            output << dataFieldToString(definition_.getType(), i);
        }
    }

    output << suboptionsToText(indent + 2);
    return (output.str());
}

std::string OptionCustom::dataFieldToString(const OptionDataType data_type,
        const uint32_t index) const {
    std::ostringstream text;
    switch (data_type) {
    case OPT_BINARY_TYPE:
        text << util::encode::encodeHex(readBinary(index));
        break;
    case OPT_BOOLEAN_TYPE:
        text << (readBoolean(index) ? "true" : "false");
        break;
    case OPT_INT8_TYPE:
        text << static_cast<int>(readInteger<int8_t>(index));
        break;
    case OPT_INT16_TYPE:
        text << readInteger<int16_t>(index);
        break;
    case OPT_INT32_TYPE:
        text << readInteger<int32_t>(index);
        break;
    case OPT_UINT8_TYPE:
        text << static_cast<unsigned>(readInteger<uint8_t>(index));
        break;
    case OPT_UINT16_TYPE:
        text << readInteger<uint16_t>(index);
        break;
    case OPT_UINT32_TYPE:
        text << readInteger<uint32_t>(index);
        break;
    case OPT_IPV4_ADDRESS_TYPE:
        text << readAddress(index);
        break;
    case OPT_FQDN_TYPE:
        text << "\"" << readFqdn(index) << "\"";
        break;
    case OPT_STRING_TYPE:
        text << "\"" << readString(index) << "\"";
        break;
    default:
        ;
    }

    return (text.str());
}

std::string OptionCustom::toText(int indent) const {
    std::stringstream output;
    output << headerToText(indent) << ":";
    OptionDataType data_type = definition_.getType();

    if (data_type == OPT_RECORD_TYPE) {
        const OptionDefinition::RecordFieldsCollection& fields =
            definition_.getRecordFields();

        // For record types we iterate over fields defined in
        // option definition and match the appropriate buffer
        // with them.
        for (auto field = fields.begin(); field != fields.end(); ++field) {
            output << " " << dataFieldToText(*field, std::distance(fields.begin(),
                                                                   field));
        }
    } else {
        for (unsigned int i = 0; i < getDataFieldsNum(); ++i) {
            output << " " << dataFieldToText(definition_.getType(), i);
        }
    }

    output << suboptionsToText(indent + 2);
    return (output.str());
}

}; 
};
