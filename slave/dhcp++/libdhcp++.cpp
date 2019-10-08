#include <kea/dhcp++/dhcp4.h>
#include <kea/dhcp++/libdhcp++.h>
#include <kea/dhcp++/option.h>
#include <kea/dhcp++/option_vendor.h>
#include <kea/dhcp++/option_definition.h>
#include <kea/dhcp++/option_int_array.h>
#include <kea/dhcp++/option_space.h>
#include <kea/dhcp++/option_meta.h>
#include <kea/dhcp++/std_option_defs.h>
#include <kea/dhcp++/vendor_option_defs.h>
#include <kea/dhcp++/option_definition.h>

#include <kea/exceptions/exceptions.h>
#include <kea/util/buffer.h>

#include <boost/lexical_cast.hpp>

#include <limits>
#include <list>

namespace kea {
namespace dhcp {

std::unordered_map<unsigned short, Option::Factory*> v4factories_;
std::unordered_map<unsigned short, Option::Factory*> v6factories_;
std::unordered_map<std::string, std::unique_ptr<OptionMeta>> runtime_options_; 

const char* DOCSIS3_CLASS_MODEM = "docsis3.0";
const char* DOCSIS3_CLASS_EROUTER = "eRouter1.0";

void LibDHCP::initOptions() {
    initStdOptions();
    initVendorOptions();
}


const OptionDefinition* LibDHCP::getStdOptionDef(uint16_t code) {
    const OptionMeta& options =  getStdV4Options();
    return options.getOptionDef(code);
}

const OptionDefinition* LibDHCP::getStdOptionDef(const std::string& name) {
    const OptionMeta& options =  getStdV4Options();
    return options.getOptionDef(name);
}


const OptionDefinition* LibDHCP::getVendorOptionDef(uint32_t vendor_id,
        const std::string& name) {
    const OptionMeta* options =  getVendorV4Options(vendor_id);
    if (options) {
        return options->getOptionDef(name);
    } else {
        return (nullptr);
    }
}

const OptionDefinition* LibDHCP::getVendorOptionDef(uint32_t vendor_id,
        uint16_t code) {
    const OptionMeta* options =  getVendorV4Options(vendor_id);
    if (options) {
        return (options->getOptionDef(code));
    } else {
        return (nullptr);
    }
}

const OptionDefinition* LibDHCP::getRuntimeOptionDef(const std::string& space, uint16_t code) {
    auto i = runtime_options_.find(space);
    if (i == runtime_options_.end()) {
        return (nullptr);
    } else {
        return (i->second->getOptionDef(code));
    }
}

const OptionDefinition* LibDHCP::getRuntimeOptionDef(const std::string& space, const std::string& name) {
    auto i = runtime_options_.find(space);
    if (i == runtime_options_.end()) {
        return (nullptr);
    } else {
        return (i->second->getOptionDef(name));
    }
}

void LibDHCP::addRuntimeOptionDef(const std::string &space, std::unique_ptr<OptionDefinition> def) {
    auto i = runtime_options_.find(space);
    OptionMeta *meta = nullptr;
    if (i == runtime_options_.end()) {
        meta = new OptionMeta();
        runtime_options_.insert(std::make_pair(space, std::unique_ptr<OptionMeta>(meta)));
    } else {
        meta = i->second.get();
    }
    meta->addOptionDef(std::move(def));
}

void LibDHCP::clearRuntimeOptionDefs() {
    runtime_options_.clear();
}

bool LibDHCP::isStandardOption(uint16_t code) {
    if (!(code == 84 || code == 96 || (code > 101 && code < 112) ||
                code == 115 || code == 126 || code == 127 ||
                (code > 146 && code < 150) || (code > 177 && code < 208) ||
                (code > 213 && code <  220) || (code > 221 && code < 255))) {
        return (true);
    }

    return (false);
}

std::unique_ptr<Option> LibDHCP::optionFactory(uint16_t type,
        const OptionBuffer& buf) {
    FactoryMap::iterator it;
    it = v4factories_.find(type);
    if (it == v4factories_.end()) {
	kea_throw(BadValue, "factory function not registered "
        "for DHCP v4 option type " << type);
    }
    return (it->second(type, buf));
}



size_t LibDHCP::unpackOptions4(const OptionBuffer& buf,
        const std::string& option_space,
        kea::dhcp::OptionCollection& options) {
    size_t offset = 0;
    size_t last_offset = 0;

    // a one-byte type code and a one-byte length field.
    while (offset < buf.size()) {
        last_offset = offset;

        uint8_t opt_type = buf[offset++];
        if (opt_type == DHO_END) {
            return (last_offset);
        }

        if (opt_type == DHO_PAD)
            continue;

        if (offset + 1 > buf.size()) {
            return (last_offset);
        }

        uint8_t opt_len =  buf[offset++];
        if (offset + opt_len > buf.size()) {
            return (last_offset);
        }

        const OptionDefinition *def = nullptr;
        if (option_space == DHCP4_OPTION_SPACE) {
            def = LibDHCP::getStdOptionDef(uint16_t(opt_type));
        } else {
            def = LibDHCP::getRuntimeOptionDef(option_space, uint16_t(opt_type));
        }

        try {
            if (def != nullptr) {
                std::unique_ptr<Option> opt_ptr = def->optionFactory(opt_type,
                        buf.begin() + offset,
                        buf.begin() + offset + opt_len);
                options.insert(std::make_pair(opt_type, std::move(opt_ptr)));
            } else {
                Option* opt = new Option(opt_type,
                        buf.begin() + offset,
                        buf.begin() + offset + opt_len);
                opt->setEncapsulatedSpace(DHCP4_OPTION_SPACE);
                options.insert(std::make_pair(opt_type, std::unique_ptr<Option>(opt)));
            }
        } catch (const kea::Exception& e) {
            logError("Dhcp++    ", "!!! unpack option $0 exception:$1", opt_type, e.what());
        }

        offset += opt_len;
    }
    last_offset = offset;
    return (last_offset);
}


size_t LibDHCP::unpackVendorOptions4(uint32_t vendor_id, const OptionBuffer& buf,
        kea::dhcp::OptionCollection& options) {
    size_t offset = 0;
    while (offset < buf.size()) {
        uint8_t data_len = buf[offset++];

        if (offset + data_len > buf.size()) {
            kea_throw(OutOfRange, "Attempt to parse truncated vendor option");
        }

        uint8_t offset_end = offset + data_len;
        while (offset < offset_end) {
            uint8_t opt_type = buf[offset++];
            if (offset + 1 > offset_end) {
                kea_throw(OutOfRange,
                          "Attempt to parse truncated vendor option "
                          << static_cast<int>(opt_type));
            }

            uint8_t opt_len =  buf[offset++];
            if (offset + opt_len > offset_end) {
                kea_throw(OutOfRange, "Option parse failed. Tried to parse "
                          << offset + opt_len << " bytes from " << buf.size()
                          << "-byte long buffer.");
            }

            const OptionDefinition *def = LibDHCP::getVendorOptionDef(vendor_id, uint16_t(opt_type));
            if (def != nullptr) {
                std::unique_ptr<Option> opt_ptr = def->optionFactory(opt_type,
                                             buf.begin() + offset,
                                             buf.begin() + offset + opt_len);
                options.insert(std::make_pair(opt_type, std::move(opt_ptr)));
            } else {
                Option *opt = new Option(opt_type,
                                           buf.begin() + offset,
                                           buf.begin() + offset + opt_len);
                options.insert(std::make_pair(opt_type, std::unique_ptr<Option>(opt)));
            }

            offset += opt_len;
        } 

        break; 
    }
    return (offset);
}

void LibDHCP::packOptions4(kea::util::OutputBuffer& buf,
        const OptionCollection& options) {
    const Option *agent = nullptr;
    const Option *end = nullptr;

    for (auto &i : options) {
        // RAI and END options must be last.
        switch (i.first) {
            case DHO_DHCP_AGENT_OPTIONS:
                agent = i.second.get();
                break;
            case DHO_END:
                end = i.second.get();
                break;
            default:
                i.second->pack(buf);
                break;
        }
    }

    // Add the RAI option if it exists.
    if (agent) {
       agent->pack(buf);
    }

    // And at the end the END option.
    if (end)  {
       end->pack(buf);
    }
}

void LibDHCP::OptionFactoryRegister(uint16_t opt_type,
        Option::Factory* factory) {
        // Option 0 is special (a one octet-long, equal 0) PAD option. It is never
        // instantiated as an Option object, but rather consumed during packet parsing.
        if (opt_type == 0) {
            kea_throw(BadValue, "Cannot redefine PAD option (code=0)");
        }
        // Option 255 is never instantiated as an option object. It is special
        // (a one-octet equal 255) option that is added at the end of all options
        // during packet assembly. It is also silently consumed during packet parsing.
        if (opt_type > 254) {
            kea_throw(BadValue, "Too big option type for DHCPv4, only 0-254 allowed.");
        }
        if (v4factories_.find(opt_type)!=v4factories_.end()) {
            kea_throw(BadValue, "There is already DHCPv4 factory registered "
                     << "for option type "  << opt_type);
        }
        v4factories_[opt_type]=factory;

    return;
}

uint32_t LibDHCP::optionSpaceToVendorId(const std::string& option_space) {
    // 8 is a minimal length of "vendor-X" format
    if ((option_space.size() < 8) || (option_space.substr(0,7) != "vendor-")) {
        return (0);
    }

    int64_t check;
    try {
        // text after "vendor-", supposedly numbers only
        std::string x = option_space.substr(7);
        check = boost::lexical_cast<int64_t>(x);
    } catch (const boost::bad_lexical_cast &) {
        return (0);
    }

    if ((check < 0) || (check > std::numeric_limits<uint32_t>::max())) {
        return (0);
    }

    // value is small enough to fit
    return (static_cast<uint32_t>(check));
}

};
};
