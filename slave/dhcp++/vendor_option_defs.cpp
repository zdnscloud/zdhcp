#include <kea/dhcp++/dhcp4.h>
#include <kea/dhcp++/vendor_option_defs.h>
#include <kea/dhcp++/option_description.h>
#include <kea/dhcp++/option_definition.h>
#include <memory>
#include <cstdint>
#include <unordered_map>

namespace kea {
namespace dhcp {

static std::unordered_map<uint32_t, OptionMeta> vendor_v4_options_;

const std::vector<OptionDescription> DOCSIS3_V4_DEFS {
    { "oro", DOCSIS3_V4_ORO, OPT_UINT8_TYPE, true, EMPTY_RECORDS, "" },
    { "tftp-servers", DOCSIS3_V4_TFTP_SERVERS, OPT_IPV4_ADDRESS_TYPE, true, EMPTY_RECORDS, "" }
};

void initVendorOptions() {
    vendor_v4_options_.clear();
    for (auto& description : DOCSIS3_V4_DEFS) {
        vendor_v4_options_[VENDOR_ID_CABLE_LABS].addOptionDef(optionDefFromDescripton(description));
    }
}

const OptionMeta* getVendorV4Options(uint32_t vendor_id) {
    auto i = vendor_v4_options_.find(vendor_id);
    if (i == vendor_v4_options_.end()) {
        return nullptr;
    } else {
        return &(i->second);
    }
}

};
};

