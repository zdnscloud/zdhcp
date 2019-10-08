#pragma once

#include <kea/dhcp++/option_meta.h>

namespace kea {
namespace dhcp {

#define VENDOR_ID_CABLE_LABS 4491
#define DOCSIS3_V4_ORO 1
#define DOCSIS3_V4_TFTP_SERVERS 2

extern const char* DOCSIS3_CLASS_EROUTER;
extern const char* DOCSIS3_CLASS_MODEM;

void initVendorOptions();
const OptionMeta* getVendorV4Options(uint32_t); 

};
};
