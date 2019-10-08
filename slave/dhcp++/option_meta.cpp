#include <kea/dhcp++/option_meta.h>


namespace kea {
namespace dhcp {

OptionMeta::OptionMeta(){}

void OptionMeta::clear() {
    code_to_def_.clear();
    name_to_def_.clear();
}

void OptionMeta::addOptionDef(std::unique_ptr<OptionDefinition> def) {
    std::shared_ptr<OptionDefinition> shared_def = std::move(def);
    code_to_def_.insert(std::make_pair(shared_def->getCode(), shared_def));
    name_to_def_.insert(std::make_pair(shared_def->getName(), shared_def));
}

const OptionDefinition* OptionMeta::getOptionDef(const std::string& name) const {
    auto it = name_to_def_.find(name);
    if (it != name_to_def_.end()) {
        return it->second.get();
    } else {
        return nullptr;
    }
}

const OptionDefinition* OptionMeta::getOptionDef(std::uint16_t code) const {
    auto it = code_to_def_.find(code);
    if (it != code_to_def_.end()) {
        return it->second.get();
    } else {
        return nullptr;
    }
}

};
};
