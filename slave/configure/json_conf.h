#pragma once

#include <kea/exceptions/exceptions.h>
#include <kea/util/json11.h>
#include <vector>
#include <string>

using namespace std;
using namespace kea::util;

namespace kea {
namespace configure {

class KeyNotExistError: public Exception {
public:
    KeyNotExistError(const char* file, size_t line, const char* what) :
        Exception(file, line, what) { };
};

class TypeError: public Exception {
public:
    TypeError(const char* file, size_t line, const char* what) :
        Exception(file, line, what) { };
};

//json object is get from a parsed json file which hold the root json object
//so each json object just a pointer wrapper which point to the root json object
//once root object is out of scope, all the json object will pointer to null pointer
class JsonObject {
public:
    explicit JsonObject(const Json*);

    bool hasKey(const string& key) const;
    int getInt(const string& key) const;
    unsigned int getUint(const string& key) const;
    string getString(const string& key) const;
    bool getBool(const string& key) const;
    double getDouble(const string& key) const;
    JsonObject getObject(const string& key) const;

    vector<int> getInts(const string& key) const;
    vector<unsigned int> getUints(const string& key) const;
    vector<string> getStrings(const string& key) const;
    vector<bool> getBools(const string& key) const;
    vector<double> getDoubles(const string& key) const;
    vector<JsonObject> getObjects(const string& key) const;

    const Json* unwrapp() const {
        return json_obj_;
    }

private:
    const  Json* recursive_get(const string& key) const;
    const  Json* try_recursive_get(const string& key) const;
    const  Json* json_obj_;
};

class JsonConf {
public:
    static std::unique_ptr<JsonConf> parseFile(const string&);
    static std::unique_ptr<JsonConf> parseString(const string&);

    JsonConf(const JsonConf&) = delete;
    JsonConf& operator=(const JsonConf&) = delete;
    JsonObject root() const {
        return JsonObject(&json_);
    }

private:
    explicit JsonConf(Json json);
    Json json_;
};

std::ostream& operator<<(std::ostream& os, const JsonObject& obj); 
};
};
