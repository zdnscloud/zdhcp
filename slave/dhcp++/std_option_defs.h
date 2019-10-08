#pragma once

#include <kea/dhcp++/option_meta.h>

namespace kea {
namespace dhcp {

void initStdOptions();
OptionMeta& getStdV4Options(); 

};
};
