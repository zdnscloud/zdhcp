#pragma once

#include <kea/dhcp++/option.h>
#include <kea/dhcp++/option_data_types.h>

#include <map>

namespace kea {
namespace dhcp {

class InvalidOptionValue : public Exception {
public:
    InvalidOptionValue(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) { };
};

class MalformedOptionDefinition : public Exception {
public:
    MalformedOptionDefinition(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) { };
};

class DuplicateOptionDefinition : public Exception {
public:
    DuplicateOptionDefinition(const char* file, size_t line, const char* what) :
        kea::Exception(file, line, what) { };
};

template<typename T>
class OptionInt;

template<typename T>
class OptionIntArray;

class OptionDefinition {
public:

    typedef std::vector<OptionDataType> RecordFieldsCollection;
    typedef std::vector<OptionDataType>::const_iterator RecordFieldsConstIter;

    OptionDefinition(const std::string& name,
                     const uint16_t code,
                     const std::string& type,
                     const bool array_type = false);

    OptionDefinition(const std::string& name,
                     const uint16_t code,
                     const OptionDataType type,
                     const bool array_type = false);

    OptionDefinition(const std::string& name,
                     const uint16_t code,
                     const std::string& type,
                     const char* encapsulated_space);

    OptionDefinition(const std::string& name,
                     const uint16_t code,
                     const OptionDataType type,
                     const char* encapsulated_space);


    bool equals(const OptionDefinition& other) const;

    bool operator==(const OptionDefinition& other) const {
        return (equals(other));
    }

    bool operator!=(const OptionDefinition& other) const {
        return (!equals(other));
    }

    void addRecordField(const std::string& data_type_name);

    void addRecordField(const OptionDataType data_type);

    bool isArrayType() const { return (array_type_); }

    uint16_t getCode() const { return (code_); }

    std::string getEncapsulatedSpace() const { return (encapsulated_space_); } 

    std::string getName() const { return (name_); }

    const RecordFieldsCollection& getRecordFields() const { return (record_fields_); }

    OptionDataType getType() const { return (type_); };

    void validate() const;


    bool haveFqdn4Format() const;
    bool haveVendor4Format() const;
    bool haveVendorClass4Format() const;

    bool haveClientFqdnFormat() const;
    bool haveStatusCodeFormat() const;
    bool haveOpaqueDataTuplesFormat() const;


    std::unique_ptr<Option> optionFactory(uint16_t type,
                            OptionBufferConstIter begin,
                            OptionBufferConstIter end) const;

    std::unique_ptr<Option> optionFactory(uint16_t type,
                            const OptionBuffer& buf = OptionBuffer()) const;

    std::unique_ptr<Option> optionFactory(uint16_t type,
                            const std::vector<std::string>& values) const;

    static std::unique_ptr<Option> factoryAddrList4(uint16_t type,
                                      OptionBufferConstIter begin,
                                      OptionBufferConstIter end);


    static std::unique_ptr<Option> factoryEmpty(uint16_t type);

    static std::unique_ptr<Option> factoryGeneric(uint16_t type,
                                    OptionBufferConstIter begin,
                                    OptionBufferConstIter end);


    template<typename T>
    static std::unique_ptr<Option> factoryInteger(uint16_t type,
                                    const std::string& encapsulated_space,
                                    OptionBufferConstIter begin,
                                    OptionBufferConstIter end) {
        std::unique_ptr<Option> option(new OptionInt<T>(type, 0));
        option->setEncapsulatedSpace(encapsulated_space);
        option->unpack(begin, end);
        return (option);
    }

    template<typename T>
    static std::unique_ptr<Option> factoryIntegerArray(uint16_t type,
                                         OptionBufferConstIter begin,
                                         OptionBufferConstIter end) {
        std::unique_ptr<Option> option(new OptionIntArray<T>(type, begin, end));
        return (option);
    }

private:

    std::unique_ptr<Option> factorySpecialFormatOption(OptionBufferConstIter begin,
                                         OptionBufferConstIter end) const;


    inline bool haveType(const OptionDataType type) const {
        return (type == type_);
    }

    bool convertToBool(const std::string& value_str) const;

    template<typename T>
    T lexicalCastWithRangeCheck(const std::string& value_str) const;

    void writeToBuffer(const std::string& value, const OptionDataType type,
                       OptionBuffer& buf) const;

    std::string name_;
    uint16_t code_;
    OptionDataType type_;
    bool array_type_;
    std::string encapsulated_space_;
    RecordFieldsCollection record_fields_;
};


};
};
