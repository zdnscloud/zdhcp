#include <kea/dhcp++/classify.h>
#include <kea/util/strutil.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/constants.hpp>
#include <boost/algorithm/string/split.hpp>
#include <sstream>
#include <vector>

namespace kea {
namespace dhcp {

ClientClasses::ClientClasses(const std::string& class_names)
    : std::set<ClientClass>() {
    std::vector<std::string> split_text;
    boost::split(split_text, class_names, boost::is_any_of(","),
                 boost::algorithm::token_compress_off);
    for (size_t i = 0; i < split_text.size(); ++i) {
        std::string trimmed = util::str::trim(split_text[i]);
        if (!trimmed.empty()) {
            insert(ClientClass(trimmed));
        }
    }
}

std::string ClientClasses::toText(const std::string& separator) const {
    std::stringstream s;
    for (const_iterator class_it = begin(); class_it != end(); ++class_it) {
        if (class_it != begin()) {
            s << separator;
        }
        s << *class_it;
    }
    return (s.str());
}
    
}; 
};

