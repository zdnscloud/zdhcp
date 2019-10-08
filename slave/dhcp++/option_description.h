#pragma once

#include <kea/dhcp++/option_data_types.h>
#include <kea/dhcp++/option_definition.h>

namespace kea {
namespace dhcp {

struct OptionDescription { 
    const char* name;              // option name
    uint16_t code;                 // option code
    OptionDataType type;           // data type
    bool array;                    // is array
    OptionDefinition::RecordFieldsCollection records; // record fields
    const char* encapsulates;      // option space encapsulated by
};

const OptionDefinition::RecordFieldsCollection EMPTY_RECORDS {};

std::unique_ptr<OptionDefinition> 
optionDefFromDescripton(const OptionDescription &); 

};
};
