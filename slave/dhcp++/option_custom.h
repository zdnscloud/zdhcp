#pragma once

#include <kea/dhcp++/option.h>
#include <kea/dhcp++/option_definition.h>
#include <kea/util/io_utilities.h>

namespace kea {
namespace dhcp {

class OptionCustom : public Option {
public:

    OptionCustom(const OptionDefinition& def);
    OptionCustom(const OptionDefinition& def,  
                 const OptionBuffer& data);
    OptionCustom(const OptionDefinition& def,  
                 OptionBufferConstIter first, 
                 OptionBufferConstIter last);

    virtual std::unique_ptr<Option> clone() const;

    void addArrayDataField(const IOAddress& address);
    void addArrayDataField(const bool value);

    template<typename T>
    void addArrayDataField(const T value) {
        checkArrayType();
        OptionBuffer buf;
        OptionDataTypeUtil::writeInt<T>(value, buf);
        buffers_.push_back(buf);
    }

    uint32_t getDataFieldsNum() const { return (buffers_.size()); }

    IOAddress readAddress(const uint32_t index = 0) const;
    void writeAddress(const IOAddress & address,
                      const uint32_t index = 0);

    const OptionBuffer& readBinary(const uint32_t index = 0) const;
    void writeBinary(const OptionBuffer& buf, const uint32_t index = 0);

    bool readBoolean(const uint32_t index = 0) const;
    void writeBoolean(const bool value, const uint32_t index = 0);

    std::string readFqdn(const uint32_t index = 0) const;
    void writeFqdn(const std::string& fqdn, const uint32_t index = 0);

    template<typename T>
    T readInteger(const uint32_t index = 0) const {
        checkIndex(index);
        checkDataType<T>(index);
        //assert(buffers_[index].size() == OptionDataTypeTraits<T>::len);
        // Read an integer value.
        return (OptionDataTypeUtil::readInt<T>(buffers_[index]));
    }

    template<typename T>
    void writeInteger(const T value, const uint32_t index = 0) {
        checkIndex(index);
        //checkDataType<T>(index);
        // Get some temporary buffer.
        OptionBuffer buf;
        // Try to write to the buffer.
        OptionDataTypeUtil::writeInt<T>(value, buf);
        // If successful, replace the old buffer with new one.
        std::swap(buffers_[index], buf);
    }

    std::string readString(const uint32_t index = 0) const;

    void writeString(const std::string& text,
                     const uint32_t index = 0);

    virtual void pack(kea::util::OutputBuffer& buf) const;

    virtual void unpack(OptionBufferConstIter begin,
                        OptionBufferConstIter end);

    virtual std::string toString() const;
    std::string dataFieldToString(const OptionDataType data_type,
                              const uint32_t index) const;

    virtual std::string toText(int indent = 0) const;

    virtual uint16_t len() const;

    void initialize(const OptionBufferConstIter first,
                    const OptionBufferConstIter last);

private:

    inline void checkArrayType() const {
        if (!definition_.isArrayType()) {
            kea_throw(InvalidOperation, "failed to add new array entry to an"
                      << " option. The option is not an array.");
        }
    }

    template<typename T>
    void checkDataType(const uint32_t index) const;

    void checkIndex(const uint32_t index) const;
    void createBuffers();
    void createBuffers(const OptionBuffer& data_buf);

    std::string dataFieldToText(const OptionDataType data_type,
                                const uint32_t index) const;

    using Option::setData;

    OptionDefinition definition_;
    std::vector<OptionBuffer> buffers_;
};

typedef std::shared_ptr<OptionCustom> OptionCustomPtr;

template<typename T>
void OptionCustom::checkDataType(const uint32_t index) const {
    if (OptionDataTypeTraits<T>::type == OPT_UNKNOWN_TYPE) {
        kea_throw(kea::dhcp::InvalidDataType, "specified data type"
                  " is not a supported integer type.");
    }

    OptionDataType data_type = definition_.getType();
    if (data_type == OPT_RECORD_TYPE) {
        const OptionDefinition::RecordFieldsCollection& record_fields =
            definition_.getRecordFields();
        assert(index < record_fields.size());
        data_type = record_fields[index];
    }

    if (OptionDataTypeTraits<T>::type != data_type) {
        kea_throw(kea::dhcp::InvalidDataType,
                  "specified data type " << data_type << " does not"
                  " match the data type in an option definition for field"
                  " index " << index);
    }
}
};
};
