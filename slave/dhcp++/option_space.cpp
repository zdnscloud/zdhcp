#include <kea/dhcp++/option_space.h>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace kea {
namespace dhcp {

OptionSpace::OptionSpace(const std::string& name, const bool vendor_space)
    : name_(name), vendor_space_(vendor_space) {
    if (!validateName(name_)) {
        kea_throw(InvalidOptionSpace, "Invalid option space name " << name_);
    }
}

bool OptionSpace::validateName(const std::string& name) {
    using namespace boost::algorithm;

    if (all(name, boost::is_from_range('a', 'z') ||
            boost::is_from_range('A', 'Z') ||
            boost::is_digit() ||
            boost::is_any_of(std::string("-_"))) &&
        !name.empty() &&
        // Hyphens and underscores are not allowed at the beginning
        // and at the end of the option space name.
        !all(find_head(name, 1), boost::is_any_of(std::string("-_"))) &&
        !all(find_tail(name, 1), boost::is_any_of(std::string("-_")))) {
        return (true);

    }
    return (false);
}

}; 
};
