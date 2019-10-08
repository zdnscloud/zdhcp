#include <kea/dhcp++/dhcp4.h>
#include <kea/dhcp++/option4_addrlst.h>
#include <kea/dhcp++/option4_client_fqdn.h>
#include <kea/dhcp++/option_custom.h>
#include <kea/dhcp++/option_definition.h>
#include <kea/dhcp++/option_int.h>
#include <kea/dhcp++/option_int_array.h>
#include <kea/dhcp++/option_opaque_data_tuples.h>
#include <kea/dhcp++/option_space.h>
#include <kea/dhcp++/option_string.h>
#include <kea/dhcp++/option_vendor.h>
#include <kea/dhcp++/option_vendor_class.h>
#include <kea/util/encode/hex.h>
#include <kea/util/strutil.h>
#include <kea/logging/logging.h>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <type_traits>

using namespace std;
using namespace kea::util;
using namespace kea::logging;

namespace kea {
namespace dhcp {

OptionDefinition::OptionDefinition(const std::string& name,
        const uint16_t code, const std::string& type, const bool array_type)
    : name_(name),
      code_(code),
      type_(OPT_UNKNOWN_TYPE),
      array_type_(array_type),
      encapsulated_space_("") {
    type_ = OptionDataTypeUtil::getDataType(type);
}

OptionDefinition::OptionDefinition(const std::string& name,
        const uint16_t code, const OptionDataType type, const bool array_type)
    : name_(name),
      code_(code),
      type_(type),
      array_type_(array_type),
      encapsulated_space_("") {
}

OptionDefinition::OptionDefinition(const std::string& name,
        const uint16_t code, const std::string& type, const char* encapsulated_space)
    : name_(name),
      code_(code),
      type_(OptionDataTypeUtil::getDataType(type)),
      array_type_(false),
      encapsulated_space_(encapsulated_space) {
}

OptionDefinition::OptionDefinition(const std::string& name,
        const uint16_t code, const OptionDataType type, const char* encapsulated_space)
    : name_(name),
      code_(code),
      type_(type),
      array_type_(false),
      encapsulated_space_(encapsulated_space) {
}

bool OptionDefinition::equals(const OptionDefinition& other) const {
    return (name_ == other.name_ &&
            code_ == other.code_ &&
            type_ == other.type_ &&
            array_type_ == other.array_type_ &&
            encapsulated_space_ == other.encapsulated_space_ &&
            record_fields_ == other.record_fields_);
}

void OptionDefinition::addRecordField(const std::string& data_type_name) {
    OptionDataType data_type = OptionDataTypeUtil::getDataType(data_type_name);
    addRecordField(data_type);
}

void OptionDefinition::addRecordField(const OptionDataType data_type) {
    if (type_ != OPT_RECORD_TYPE) {
        kea_throw(kea::InvalidOperation, "'record' option type must be used"
                " to add data fields to the record");
    }

    if (data_type >= OPT_RECORD_TYPE ||
        data_type == OPT_ANY_ADDRESS_TYPE ||
        data_type == OPT_EMPTY_TYPE) {
        kea_throw(kea::BadValue,
                "attempted to add invalid data type to the record.");
    }
    record_fields_.push_back(data_type);
}

std::unique_ptr<Option> OptionDefinition::optionFactory(uint16_t type,
        OptionBufferConstIter begin, OptionBufferConstIter end) const {
    std::unique_ptr<Option> option = factorySpecialFormatOption(begin, end);
    if (option) {
        return (option);
    }

    switch(type_) {
        case OPT_EMPTY_TYPE:
            if (getEncapsulatedSpace().empty()) {
                return (factoryEmpty(type));
            } else {
                return (std::unique_ptr<Option>(new OptionCustom(*this, begin, end)));
            }

        case OPT_BINARY_TYPE:
            return (factoryGeneric(type, begin, end));

        case OPT_UINT8_TYPE:
            return (array_type_ ?
                    factoryIntegerArray<uint8_t>(type, begin, end) :
                    factoryInteger<uint8_t>(type, getEncapsulatedSpace(),
                        begin, end));

        case OPT_INT8_TYPE:
            return (array_type_ ?
                    factoryIntegerArray<int8_t>(type, begin, end) :
                    factoryInteger<int8_t>(type, getEncapsulatedSpace(),
                        begin, end));

        case OPT_UINT16_TYPE:
            return (array_type_ ?
                    factoryIntegerArray<uint16_t>(type, begin, end) :
                    factoryInteger<uint16_t>(type, getEncapsulatedSpace(),
                        begin, end));

        case OPT_INT16_TYPE:
            return (array_type_ ?
                    factoryIntegerArray<uint16_t>(type, begin, end) :
                    factoryInteger<int16_t>(type, getEncapsulatedSpace(),
                        begin, end));

        case OPT_UINT32_TYPE:
            return (array_type_ ?
                    factoryIntegerArray<uint32_t>(type, begin, end) :
                    factoryInteger<uint32_t>(type, getEncapsulatedSpace(),
                        begin, end));

        case OPT_INT32_TYPE:
            return (array_type_ ?
                    factoryIntegerArray<uint32_t>(type, begin, end) :
                    factoryInteger<int32_t>(type, getEncapsulatedSpace(),
                        begin, end));

        case OPT_IPV4_ADDRESS_TYPE:
            if (array_type_) {
                return (factoryAddrList4(type, begin, end));
            }
            break;

        case OPT_STRING_TYPE:
            return (std::unique_ptr<Option>(new OptionString(type, begin, end)));

        default:
            ;
    }
    return (std::unique_ptr<Option>(new OptionCustom(*this, begin, end)));
}

std::unique_ptr<Option> OptionDefinition::optionFactory(uint16_t type,
        const OptionBuffer& buf) const {
    return (optionFactory(type, buf.begin(), buf.end()));
}

std::unique_ptr<Option> OptionDefinition::optionFactory(uint16_t type,
        const std::vector<std::string>& values) const {
    OptionBuffer buf;
    if (!array_type_ && type_ != OPT_RECORD_TYPE) {
        if (values.empty()) {
            if (type_ != OPT_EMPTY_TYPE) {
                kea_throw(InvalidOptionValue, "no option value specified");
            }
        } else {
            writeToBuffer(util::str::trim(values[0]), type_, buf);
        }
    } else if (array_type_ && type_ != OPT_RECORD_TYPE) {
        for (size_t i = 0; i < values.size(); ++i) {
            writeToBuffer(util::str::trim(values[i]), type_, buf);
        }
    } else if (type_ == OPT_RECORD_TYPE) {
        const RecordFieldsCollection& records = getRecordFields();
        if (records.size() > values.size()) {
            kea_throw(InvalidOptionValue, "number of data fields for the option"
                      << " type '" <<  getCode() << "' is greater than number"
                      << " of values provided.");
        }
        for (size_t i = 0; i < records.size(); ++i) {
            writeToBuffer(util::str::trim(values[i]),
                          records[i], buf);
        }
    }
    return (optionFactory(type, buf.begin(), buf.end()));
}

void OptionDefinition::validate() const {
    using namespace boost::algorithm;
    std::ostringstream err_str;
    if (!all(name_, boost::is_from_range('a', 'z') ||
             boost::is_from_range('A', 'Z') ||
             boost::is_digit() ||
             boost::is_any_of(std::string("-_"))) ||
        name_.empty() ||
        // Hyphens and underscores are not allowed at the beginning
        // and at the end of the option name.
        all(find_head(name_, 1), boost::is_any_of(std::string("-_"))) ||
        all(find_tail(name_, 1), boost::is_any_of(std::string("-_")))) {
        err_str << "invalid option name '" << name_ << "'";
    } else if (!encapsulated_space_.empty() &&
               !OptionSpace::validateName(encapsulated_space_)) {
        err_str << "invalid encapsulated option space name: '"
                << encapsulated_space_ << "'";
    } else if (type_ >= OPT_UNKNOWN_TYPE) {
        err_str << "option type " << type_ << " not supported.";
    } else if (array_type_) {
        if (type_ == OPT_STRING_TYPE) {
            err_str << "array of strings is not a valid option definition.";
        } else if (type_ == OPT_BINARY_TYPE) {
            err_str << "array of binary values is not"
                    << " a valid option definition.";
        } else if (type_ == OPT_EMPTY_TYPE) {
            err_str << "array of empty value is not"
                    << " a valid option definition.";
        }
    } else if (type_ == OPT_RECORD_TYPE) {
        if (getRecordFields().size() < 2) {
            err_str << "invalid number of data fields: "
                    << getRecordFields().size()
                    << " specified for the option of type 'record'. Expected at"
                    << " least 2 fields.";
        } else {
            // If the number of fields is valid we have to check if their order
            // is valid too. We check that string or binary data fields are not
            // laid before other fields. But we allow that they are laid at the
            // end of an option.
            const RecordFieldsCollection& fields = getRecordFields();
            for (RecordFieldsConstIter it = fields.begin();
                 it != fields.end(); ++it) {
                if (*it == OPT_STRING_TYPE &&
                    it < fields.end() - 1) {
                    err_str << "string data field can't be laid before data"
                            << " fields of other types.";
                    break;
                }
                if (*it == OPT_BINARY_TYPE &&
                    it < fields.end() - 1) {
                    err_str << "binary data field can't be laid before data"
                            << " fields of other types.";
                }
                /// Empty type is not allowed within a record.
                if (*it == OPT_EMPTY_TYPE) {
                    err_str << "empty data type can't be stored as a field in"
                            << " an option record.";
                    break;
                }
            }
        }

    }

    if (!err_str.str().empty()) {
        kea_throw(MalformedOptionDefinition, err_str.str());
    }
}

bool OptionDefinition::haveFqdn4Format() const {
    return (haveType(OPT_RECORD_TYPE) &&
            record_fields_.size() == 4 &&
            record_fields_[0] == OPT_UINT8_TYPE &&
            record_fields_[1] == OPT_UINT8_TYPE &&
            record_fields_[2] == OPT_UINT8_TYPE &&
            record_fields_[3] == OPT_FQDN_TYPE);
}

bool OptionDefinition::haveClientFqdnFormat() const {
    return (haveType(OPT_RECORD_TYPE) &&
            (record_fields_.size() == 2) &&
            (record_fields_[0] == OPT_UINT8_TYPE) &&
            (record_fields_[1] == OPT_FQDN_TYPE));
}

bool OptionDefinition::haveVendor4Format() const {
    return (true);
}

bool OptionDefinition::haveVendorClass4Format() const {
    return (haveType(OPT_RECORD_TYPE) &&
            (record_fields_.size() == 2) &&
            (record_fields_[0] == OPT_UINT32_TYPE) &&
            (record_fields_[1] == OPT_BINARY_TYPE));
}

bool OptionDefinition::haveStatusCodeFormat() const {
    return (haveType(OPT_RECORD_TYPE) &&
            (record_fields_.size() == 2) &&
            (record_fields_[0] == OPT_UINT16_TYPE) &&
            (record_fields_[1] == OPT_STRING_TYPE));
}

bool OptionDefinition::haveOpaqueDataTuplesFormat() const {
    return (getType() == OPT_BINARY_TYPE);
}

bool OptionDefinition::convertToBool(const std::string& value_str) const {
    if (boost::iequals(value_str, "true")) {
        return (true);
    } else if (boost::iequals(value_str, "false")) {
        return (false);
    }

    int result;
    try {
       result = boost::lexical_cast<int>(value_str);
    } catch (const boost::bad_lexical_cast&) {
        kea_throw(BadDataTypeCast, "unable to covert the value '"
                  << value_str << "' to boolean data type");
    }

    if (result != 1 && result != 0) {
        kea_throw(BadDataTypeCast, "unable to convert '" << value_str
                  << "' to boolean data type");
    }
    return (static_cast<bool>(result));
}

template<typename T>
T OptionDefinition::lexicalCastWithRangeCheck(const std::string& value_str)
    const {
    if (std::is_integral<T>::value == 0) {
        kea_throw(BadDataTypeCast,
                  "must not convert '" << value_str
                  << "' to non-integer data type");
    }

    int64_t result = 0;
    try {
        result = boost::lexical_cast<int64_t>(value_str);

    } catch (const boost::bad_lexical_cast&) {
        kea_throw(BadDataTypeCast, "unable to convert the value '"
                  << value_str << "' to integer data type");
    }

    if (std::is_integral<T>::value == 1) {
        if (result > numeric_limits<T>::max() ||
            result < numeric_limits<T>::min()) {
            kea_throw(BadDataTypeCast, "unable to convert '"
                      << value_str << "' to numeric type. This value is "
                      " expected to be in the range of "
                      << numeric_limits<T>::min()
                      << ".." << numeric_limits<T>::max());
        }
    }
    return (static_cast<T>(result));
}

void OptionDefinition::writeToBuffer(const std::string& value,
        const OptionDataType type, OptionBuffer& buf) const {
    switch (type) {
    case OPT_BINARY_TYPE:
        OptionDataTypeUtil::writeBinary(value, buf);
        return;
    case OPT_BOOLEAN_TYPE:
        OptionDataTypeUtil::writeBool(convertToBool(value), buf);
        return;
    case OPT_INT8_TYPE:
        OptionDataTypeUtil::writeInt<uint8_t>
            (lexicalCastWithRangeCheck<int8_t>(value), buf);
        return;
    case OPT_INT16_TYPE:
        OptionDataTypeUtil::writeInt<uint16_t>
            (lexicalCastWithRangeCheck<int16_t>(value), buf);
        return;
    case OPT_INT32_TYPE:
        OptionDataTypeUtil::writeInt<uint32_t>
            (lexicalCastWithRangeCheck<int32_t>(value), buf);
        return;
    case OPT_UINT8_TYPE:
        OptionDataTypeUtil::writeInt<uint8_t>
            (lexicalCastWithRangeCheck<uint8_t>(value), buf);
        return;
    case OPT_UINT16_TYPE:
        OptionDataTypeUtil::writeInt<uint16_t>
            (lexicalCastWithRangeCheck<uint16_t>(value), buf);
        return;
    case OPT_UINT32_TYPE:
        OptionDataTypeUtil::writeInt<uint32_t>
            (lexicalCastWithRangeCheck<uint32_t>(value), buf);
        return;
    case OPT_IPV4_ADDRESS_TYPE:
        {
            IOAddress address(value);
            if (!address.isV4()) {
                kea_throw(BadDataTypeCast, "provided address "
                          << address << " is not a valid IPv4 address.");
            }
            OptionDataTypeUtil::writeAddress(address, buf);
            return;
        }
    case OPT_STRING_TYPE:
        OptionDataTypeUtil::writeString(value, buf);
        return;
    case OPT_FQDN_TYPE:
        OptionDataTypeUtil::writeFqdn(value, buf);
        return;
    default:
        ;
    }
    kea_throw(kea::BadValue, "attempt to write invalid option data field type"
              " into the option buffer: " << type);

}

std::unique_ptr<Option> OptionDefinition::factoryAddrList4(uint16_t type,
        OptionBufferConstIter begin, OptionBufferConstIter end) {
    return std::unique_ptr<Option>(new Option4AddrLst(type, begin, end));
}

std::unique_ptr<Option> OptionDefinition::factoryEmpty(uint16_t type) {
    return std::unique_ptr<Option> (new Option(type));
}

std::unique_ptr<Option> OptionDefinition::factoryGeneric(uint16_t type,
        OptionBufferConstIter begin, OptionBufferConstIter end) {
    return std::unique_ptr<Option> (new Option(type, begin, end));
}

std::unique_ptr<Option> OptionDefinition::factorySpecialFormatOption(
        OptionBufferConstIter begin, OptionBufferConstIter end) const {
    if ((getCode() == DHO_FQDN) && haveFqdn4Format()) {
        return (std::unique_ptr<Option>(new Option4ClientFqdn(begin, end)));
    } else if ((getCode() == DHO_VIVCO_SUBOPTIONS) &&
            haveVendorClass4Format()) {
        return (std::unique_ptr<Option>(new OptionVendorClass(begin, end)));
    } else if (getCode() == DHO_VIVSO_SUBOPTIONS && haveVendor4Format()) {
        return (std::unique_ptr<Option>(new OptionVendor(begin, end)));
    } else {
        return (std::unique_ptr<Option>());
    }
}

}; 
};
