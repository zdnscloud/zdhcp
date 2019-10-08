#include <kea/exceptions/exceptions.h>
#include <kea/dhcp++/option_description.h>
#include <memory>

namespace kea {
namespace dhcp {

std::unique_ptr<OptionDefinition> optionDefFromDescripton(
        const OptionDescription &description) {
    std::string encapsulates(description.encapsulates);
    if (!encapsulates.empty() && description.array) {
        kea_throw(BadValue, "invalid standard option definition: "
                << "option with code '" << description.code
                << "' may not encapsulate option space '"
                << encapsulates << "' because the definition"
                << " indicates that this option comprises an array"
                << " of values");
    }  

    OptionDefinition* def_ptr;
    if (encapsulates.empty()) {
        def_ptr = new OptionDefinition(description.name,
                description.code, description.type, description.array);
        
    } else {
        def_ptr = new OptionDefinition(description.name,
                description.code, description.type, description.encapsulates);
    }

    if (description.records.empty() == false) {
        for(auto& data_type : description.records) {
            def_ptr->addRecordField(data_type);
        }
    }

    def_ptr->validate();
    return std::unique_ptr<OptionDefinition>(def_ptr);
}

};
};

