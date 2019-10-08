#include <kea/dhcp++/option.h>
#include <kea/dhcp++/libdhcp++.h>
#include <kea/exceptions/exceptions.h>
#include <kea/util/encode/hex.h>
#include <kea/util/io_utilities.h>

#include <iomanip>
#include <sstream>

#include <arpa/inet.h>
#include <stdint.h>
#include <string.h>

using namespace std;
using namespace kea::util;

namespace kea {
namespace dhcp {

std::unique_ptr<Option>
Option::factory(uint16_t type,
        const OptionBuffer& buf) {
    //return(LibDHCP::optionFactory(u, type, buf));
    return std::unique_ptr<Option>();
}


Option::Option(uint16_t type)
    :type_(type) {
    if (((type == 0) || (type > 254))) {
        kea_throw(BadValue, "Can't create V4 option of type "
                  << type << ", V4 options are in range 1..254");
    }
}

Option::Option(uint16_t type, const OptionBuffer& data)
    :type_(type), data_(data) {
    check();
}

Option::Option(uint16_t type, OptionBufferConstIter first,
               OptionBufferConstIter last)
    :type_(type), data_(first, last) {
    check();
}

Option::Option(const Option& option)
    : type_(option.type_),
      data_(option.data_), options_(),
      encapsulated_space_(option.encapsulated_space_) {
    option.getOptionsCopy(options_);
}

Option& Option::operator=(const Option& rhs) {
    if (&rhs != this) {
        type_ = rhs.type_;
        data_ = rhs.data_;
        rhs.getOptionsCopy(options_);
        encapsulated_space_ = rhs.encapsulated_space_;
    }
    return (*this);
}

std::unique_ptr<Option> Option::clone() const {
    return (cloneInternal<Option>());
}

void Option::check() const {
        if (type_ > 255) {
            kea_throw(OutOfRange, "DHCPv4 Option type " << type_ << " is too big. "
                      << "For DHCPv4 allowed type range is 0..255");
        } else if (data_.size() > 255) {
            kea_throw(OutOfRange, "DHCPv4 Option " << type_ << " is too big.");
        }
}

void Option::pack(kea::util::OutputBuffer& buf) const {
    packHeader(buf);
    if (!data_.empty()) {
        buf.writeData(&data_[0], data_.size());
    }
    packOptions(buf);
}

void Option::packHeader(kea::util::OutputBuffer& buf) const {
        if (len() > 255) {
            kea_throw(OutOfRange, "DHCPv4 Option " << type_ << " is too big. "
                      << "At most 255 bytes are supported.");
        }

        buf.writeUint8(type_);
        buf.writeUint8(len() - getHeaderLen());

}

void Option::packOptions(kea::util::OutputBuffer& buf) const {
        LibDHCP::packOptions4(buf, options_);
        return;
}

void Option::unpack(OptionBufferConstIter begin,
                    OptionBufferConstIter end) {
    setData(begin, end);
}

void Option::unpackOptions(const OptionBuffer& buf) {
        LibDHCP::unpackOptions4(buf, getEncapsulatedSpace(), options_);
        return;
}

uint16_t Option::len() const {
    size_t length = getHeaderLen() + data_.size();
    for (auto &sub_option_pair : options_) {
        length += sub_option_pair.second->len();
    }

    return (static_cast<uint16_t>(length));
}


const Option* Option::getOption(uint16_t opt_type) const {
    auto x = options_.find(opt_type);
    if ( x != options_.end() ) {
        return x->second.get();
    } else {
        return nullptr;
    }
}

void Option::getOptionsCopy(OptionCollection& options_copy) const {
    OptionCollection local_options;
    for (auto &pair : options_) {
        local_options.insert(std::make_pair(pair.second->getType(), pair.second->clone()));
    }

    options_copy.swap(local_options);
}

bool Option::delOption(uint16_t opt_type) {
    auto x = options_.find(opt_type);
    if (x != options_.end()) {
        options_.erase(x);
        return (true);
    } else {
        return (false); 
    }
}


std::string Option::toText(int indent) const {
    std::stringstream output;
    output << headerToText(indent) << ": ";

    for (unsigned int i = 0; i < data_.size(); i++) {
        if (i != 0) { output << ":"; }
        output << setfill('0') << setw(2) << hex
            << static_cast<unsigned short>(data_[i]);
    }

    output << suboptionsToText(indent + 2);
    return (output.str());
}

std::string Option::toString() const {
    return (toHexString(false));
}

std::vector<uint8_t> Option::toBinary(const bool include_header) const {
    OutputBuffer buf(len());
    try {
        pack(buf);
    } catch (const std::exception &ex) {
        kea_throw(OutOfRange, "unable to obtain hexadecimal representation"
                  " of option " << getType() << ": " << ex.what());
    }
    const uint8_t* option_data = static_cast<const uint8_t*>(buf.getData());

    std::vector<uint8_t> option_vec(option_data + (include_header ? 0 : getHeaderLen()),
                                    option_data + buf.getLength());
    return (option_vec);
}

std::string Option::toHexString(const bool include_header) const {
    std::vector<uint8_t> option_vec = toBinary(include_header);

    std::ostringstream s;
    if (!option_vec.empty()) {
        s << "0x" << encode::encodeHex(option_vec);
    }
    return (s.str());
}

std::string Option::headerToText(const int indent, const std::string& type_name) const {
    std::stringstream output;
    for (int i = 0; i < indent; i++)
        output << " ";

    int field_len = 3;
    output << "type=" << std::setw(field_len) << std::setfill('0')
           << type_;

    if (!type_name.empty()) {
        output << "(" << type_name << ")";
    }

    output << ", len=" << std::setw(field_len) << std::setfill('0')
           << len()-getHeaderLen();
    return (output.str());
}

std::string Option::suboptionsToText(const int indent) const {
    std::stringstream output;

    if (!options_.empty()) {
        output << "," << std::endl << "options:";
        for(auto& pair : options_) {
            output << std::endl << pair.second->toText(indent);
        }
    }

    return (output.str());
}

uint16_t Option::getHeaderLen() const {
    return OPTION4_HDR_LEN; 
}

void Option::addOption(std::unique_ptr<Option> opt) {
    if (getOption(opt->getType())) {
	kea_throw(BadValue, "Option " << opt->getType()
			<< " already present in this message.");
    }
    options_.insert(make_pair(opt->getType(), std::move(opt)));
}

uint8_t Option::getUint8() const {
    if (data_.size() < sizeof(uint8_t) ) {
        kea_throw(OutOfRange, "Attempt to read uint8 from option " << type_
                  << " that has size " << data_.size());
    }
    return (data_[0]);
}

uint16_t Option::getUint16() const {
    return (readUint16(&data_[0], data_.size()));
}

uint32_t Option::getUint32() const {
    return (readUint32(&data_[0], data_.size()));
}

void Option::setUint8(uint8_t value) {
    data_.resize(sizeof(value));
    data_[0] = value;
}

void Option::setUint16(uint16_t value) {
    data_.resize(sizeof(value));
    writeUint16(value, &data_[0], data_.size());
}

void Option::setUint32(uint32_t value) {
    data_.resize(sizeof(value));
    writeUint32(value, &data_[0], data_.size());
}

bool Option::equals(const Option& other) const {
    return ( (getType() == other.getType()) &&
             (getData() == other.getData()) );
}

Option::~Option() {}

}; 
};
