#include <kea/configure/json_conf.h>
#include <kea/exceptions/exceptions.h>
#include <gtest/gtest.h>

using namespace std;
using namespace kea;
using namespace kea::configure;


const string JSON_CONF = R"({
"interfaces-config": {
    "interfaces": ["*"]
},  

"lease-database": {
    "type": "postgresql",
    "name": "zdns",
    "host": "localhost",
    "user": "zdns",
    "password": "zdns"
},  

"expired-leases-processing": {
    "reclaim-timer-wait-time": 10, 
    "flush-reclaimed-timer-wait-time": 25, 
    "hold-reclaimed-time": 3600,
    "max-reclaim-leases": 100,
    "max-reclaim-time": 250,
    "unwarned-reclaim-cycles": 5
},  


"client-classes": [
{   
    "name": "client-1",
    "test": "substring(option[55].hex,0,all) == 0x011c02030f060c"
},  
{   
    "name": "client-2",
    "test": "substring(option[60].hex,0,all) == 'asdfg'"
}],  

"subnet4": [
{   
    "subnet": "192.168.1.0/24",
    "pools": [ 
        {"pool": "192.168.1.11 - 192.168.1.12"},
        {"pool": "192.168.1.21 - 192.168.1.22"}
    ],
    "client-class": "client-1",    
    "client-class": "client-2",
    "option-data": [
        {   
            "name": "vendor-class-identifier",
            "code": 60, 
            "space": "dhcp4",
            "csv-format": true,
            "data": "MSFT 6.0"
        }   
    ],  

    "valid-lifetime": 4000
}],

"parent_struct": {
    "child_structs": [
        {"key1":false},
        {"key1":true}
    ]
}   
})";

namespace kea {

TEST(JsonConfTest, parse) {
    std::unique_ptr<JsonConf> conf = JsonConf::parseString(JSON_CONF);
    JsonObject obj = conf->root().getObject("lease-database");
    EXPECT_EQ(obj.getString("name"), "zdns");

    vector<JsonObject> objs = conf->root().getObjects("subnet4");
    EXPECT_EQ(objs[0].getInt("valid-lifetime"), 4000);
    EXPECT_EQ(objs[0].getString("subnet"), "192.168.1.0/24");

    objs = objs[0].getObjects("pools"); 
    EXPECT_EQ(objs.size(), 2);
    EXPECT_EQ(objs[0].getString("pool"), "192.168.1.11 - 192.168.1.12");

    objs = conf->root().getObjects("parent_struct.child_structs");
    EXPECT_EQ(objs.size(), 2);
    EXPECT_EQ(objs[0].getBool("key1"), false);
    EXPECT_EQ(objs[1].getBool("key1"), true);
}
};
