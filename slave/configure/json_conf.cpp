#include <kea/configure/json_conf.h>
#include <kea/util/strutil.h>
#include <vector>
#include <fstream>
#include <streambuf>

namespace kea {
namespace configure {

std::unique_ptr<JsonConf> 
JsonConf::parseFile(const string& path) {
    std::fstream f(path);
    std::string json_str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return parseString(json_str);
}

std::unique_ptr<JsonConf> 
JsonConf::parseString(const string& json) {
    std::string err;
    Json json_obj = Json::parse(json, err);
    if (!err.empty()) {
        kea_throw(TypeError, "Json parse with error: " << err.c_str());
    }
    return std::unique_ptr<JsonConf>(new JsonConf(json_obj));
}

JsonConf::JsonConf(Json json) : json_(json) {
}

JsonObject::JsonObject(const Json* json_obj) : json_obj_(json_obj) {
}

bool 
JsonObject::hasKey(const string& key) const {
    return (try_recursive_get(key) != nullptr); 
}

int 
JsonObject::getInt(const string& key) const {
    auto obj = recursive_get(key);
    if (obj->is_number()) {
        return int(obj->int_value());
    } else {
        kea_throw(TypeError, "key is point to int but " << obj->type());
    }
}

unsigned int 
JsonObject::getUint(const string& key) const {
    auto obj = recursive_get(key);
    if (obj->is_number()) {
        return (unsigned int)(obj->int_value());
    } else {
        kea_throw(TypeError, "key is point to int but " << obj->type());
    }
}

vector<int> 
JsonObject::getInts(const string& key) const {
    auto obj = recursive_get(key);
    if (obj->is_array()) {
        vector<int> ret;
        for (auto& v : obj->array_items()) {
            ret.push_back(int(v.int_value()));
        }
        return std::move(ret);
    } else {
        kea_throw(TypeError, "key is point to array but " << obj->type());
    }
}

vector<unsigned int> 
JsonObject::getUints(const string& key) const {
    auto obj = recursive_get(key);
    if (obj->is_array()) {
        vector<unsigned int> ret;
        for (auto& v : obj->array_items()) {
            ret.push_back((unsigned int)(v.int_value()));
        }
        return std::move(ret);
    } else {
        kea_throw(TypeError, "key is point to array but " << obj->type());
    }
}


const Json* 
JsonObject::recursive_get(const string& key) const {
    const Json* o = try_recursive_get(key); 
    if (o == nullptr) {
        kea_throw(KeyNotExistError, key);
    }
    return o;
}

const Json* 
JsonObject::try_recursive_get(const string& key) const {
    std::vector<string> keys = kea::util::str::tokens(key, ".");
    const Json* o = json_obj_; 
    for(auto& k : keys) {
        o = &((*o)[k]);
        if (*o == kea::util::static_null()) {
            return nullptr;
        }
    }
    return o;
}

string 
JsonObject::getString(const string& key) const {
    auto obj = recursive_get(key);
    if (obj->is_string()) {
        return (obj->string_value());
    } else {
        kea_throw(TypeError, "key is point to string but " << obj->type());
    }
}

vector<string> 
JsonObject::getStrings(const string& key) const {
    auto obj = recursive_get(key);
    if (obj->is_array()) {
        vector<string> ret;
        for (auto& v : obj->array_items()) {
            ret.push_back(v.string_value());
        }
        return std::move(ret);
    } else {
        kea_throw(TypeError, "key is point to string but " << obj->type());
    }
}

bool 
JsonObject::getBool(const string& key) const {
    auto obj = recursive_get(key);
    if (obj->is_bool()) {
        return (obj->bool_value());
    } else {
        kea_throw(TypeError, "key is point to bool but " << obj->type());
    }
}

vector<bool> 
JsonObject::getBools(const string& key) const {
    auto obj = recursive_get(key);
    if (obj->is_array()) {
        vector<bool> ret;
        for (auto& v : obj->array_items()) {
            ret.push_back(v.bool_value());
        }
        return std::move(ret);
    } else {
        kea_throw(TypeError, "key is point to bool but " << obj->type());
    }
}


double 
JsonObject::getDouble(const string& key) const {
    auto obj = recursive_get(key);
    if (obj->is_number()) {
        return (obj->number_value());
    } else {
        kea_throw(TypeError, "key is point to double but " << obj->type());
    }
}

vector<double> 
JsonObject::getDoubles(const string& key) const {
    auto obj = recursive_get(key);
    if (obj->is_array()) {
        vector<double> ret;
        for (auto& v : obj->array_items()) {
            ret.push_back(v.number_value());
        }
        return std::move(ret);
    } else {
        kea_throw(TypeError, "key is point to double but " << obj->type());
    }
}

JsonObject 
JsonObject::getObject(const string& key) const {
    const Json* obj = recursive_get(key);
    if (obj->is_object()) {
        return JsonObject(obj);
    } else {
        kea_throw(TypeError, "key is point to object but " << obj->type());
    }
}

vector<JsonObject> 
JsonObject::getObjects(const string& key) const {
    const Json* obj = recursive_get(key);
    if (obj->is_array()) {
        vector<JsonObject> ret;
        for (int i = 0; i < obj->array_items().size(); i++) {
            ret.push_back(JsonObject(&((*obj)[i])));
        }
        return std::move(ret);
    } else {
        kea_throw(TypeError, "key is point to object but " << obj->type());
    }
}

std::ostream& operator<<(std::ostream& os, const JsonObject& o) {
    os << o.unwrapp()->dump();
    return (os);
}

};
};
