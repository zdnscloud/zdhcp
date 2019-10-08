#include <kea/dhcp++/classify.h>
#include <kea/server/subnet_mgr.h>
#include <kea/server/subnet.h>
#include <kea/server/subnet_id.h>
#include <gtest/gtest.h>

using namespace kea;
using namespace kea::server;
using namespace kea::dhcp;

namespace kea {

using Subnet4Ptr = kea::server::SubnetMgr::Subnet4Ptr;

TEST(SubnetMgrTest, selectSubnetByCiaddr) {
    SubnetMgr mgr;
    Subnet4* subnet1 = new Subnet4(IPAddress("192.0.2.0"), 26, 1, 2, 3);
    Subnet4* subnet2 = new Subnet4(IPAddress("192.0.2.64"), 26, 1, 2, 3);
    Subnet4* subnet3 = new Subnet4(IPAddress("192.0.2.128"), 26, 1, 2, 3);

    SubnetSelector selector;
    selector.ciaddr_ = IPAddress("192.0.2.0");
    selector.local_address_ = IPAddress("10.0.0.100");
    ASSERT_FALSE(mgr.selectSubnet(selector));

    mgr.add(Subnet4Ptr(subnet1));
    selector.ciaddr_ = IPAddress("192.0.2.63");
    EXPECT_EQ(subnet1, mgr.selectSubnet(selector));

    // Add all other subnets.
    mgr.add(Subnet4Ptr(subnet2));
    mgr.add(Subnet4Ptr(subnet3));

    // Make sure they are returned for the appropriate addresses.
    selector.ciaddr_ = IPAddress("192.0.2.15");
    EXPECT_EQ(subnet1, mgr.selectSubnet(selector));
    selector.ciaddr_ = IPAddress("192.0.2.85");
    EXPECT_EQ(subnet2, mgr.selectSubnet(selector));
    selector.ciaddr_ = IPAddress("192.0.2.191");
    EXPECT_EQ(subnet3, mgr.selectSubnet(selector));

    selector.ciaddr_ = IPAddress("192.0.2.192");
    EXPECT_FALSE(mgr.selectSubnet(selector));
}


TEST(SubnetMgrTest, selectSubnetByClasses) {
    SubnetMgr mgr;
    Subnet4* subnet1 = new Subnet4(IPAddress("192.0.2.0"), 26, 1, 2, 3);
    Subnet4* subnet2 = new Subnet4(IPAddress("192.0.2.64"), 26, 1, 2, 3);
    Subnet4* subnet3 = new Subnet4(IPAddress("192.0.2.128"), 26, 1, 2, 3);

    mgr.add(Subnet4Ptr(subnet1));
    mgr.add(Subnet4Ptr(subnet2));     
    mgr.add(Subnet4Ptr(subnet3));   

    SubnetSelector selector;

    selector.local_address_ = IPAddress("10.0.0.10");

    selector.ciaddr_ = IPAddress("192.0.2.5");
    EXPECT_EQ(subnet1, mgr.selectSubnet(selector));
    selector.ciaddr_ = IPAddress("192.0.2.70");
    EXPECT_EQ(subnet2, mgr.selectSubnet(selector));
    selector.ciaddr_ = IPAddress("192.0.2.130");
    EXPECT_EQ(subnet3, mgr.selectSubnet(selector));

    ClientClasses client_classes;
    client_classes.insert("bar");
    selector.client_classes_ = client_classes;

    selector.ciaddr_ = IPAddress("192.0.2.5");
    EXPECT_EQ(subnet1, mgr.selectSubnet(selector));
    selector.ciaddr_ = IPAddress("192.0.2.70");
    EXPECT_EQ(subnet2, mgr.selectSubnet(selector));
    selector.ciaddr_ = IPAddress("192.0.2.130");
    EXPECT_EQ(subnet3, mgr.selectSubnet(selector));

    subnet1->allowClientClass("foo"); // Serve here only clients from foo class
    subnet2->allowClientClass("bar"); // Serve here only clients from bar class
    subnet3->allowClientClass("baz"); // Serve here only clients from baz class

    selector.ciaddr_ = IPAddress("192.0.2.5");
    EXPECT_FALSE(mgr.selectSubnet(selector));
    selector.ciaddr_ = IPAddress("192.0.2.70");
    EXPECT_EQ(subnet2, mgr.selectSubnet(selector));
    selector.ciaddr_ = IPAddress("192.0.2.130");
    EXPECT_FALSE(mgr.selectSubnet(selector));

    client_classes.clear();
    client_classes.insert("some_other_class");
    selector.client_classes_ = client_classes;
    selector.ciaddr_ = IPAddress("192.0.2.5");
    EXPECT_FALSE(mgr.selectSubnet(selector));
    selector.ciaddr_ = IPAddress("192.0.2.70");
    EXPECT_FALSE(mgr.selectSubnet(selector));
    selector.ciaddr_ = IPAddress("192.0.2.130");
    EXPECT_FALSE(mgr.selectSubnet(selector));

    client_classes.clear();
    selector.ciaddr_ = IPAddress("192.0.2.5");
    EXPECT_FALSE(mgr.selectSubnet(selector));
    selector.ciaddr_ = IPAddress("192.0.2.70");
    EXPECT_FALSE(mgr.selectSubnet(selector));
    selector.ciaddr_ = IPAddress("192.0.2.130");
    EXPECT_FALSE(mgr.selectSubnet(selector));
}

TEST(SubnetMgrTest, selectSubnetByOptionSelect) {
    SubnetMgr mgr;

    Subnet4* subnet1 = new Subnet4(IPAddress("192.0.2.0"), 26, 1, 2, 3);
    Subnet4* subnet2 = new Subnet4(IPAddress("192.0.2.64"), 26, 1, 2, 3);
    Subnet4* subnet3 = new Subnet4(IPAddress("192.0.2.128"), 26, 1, 2, 3);

    mgr.add(Subnet4Ptr(subnet1));
    mgr.add(Subnet4Ptr(subnet2));     
    mgr.add(Subnet4Ptr(subnet3));   

    SubnetSelector selector;

    // Check that without option selection something else is used
    selector.ciaddr_ = IPAddress("192.0.2.5");
    EXPECT_EQ(subnet1, mgr.selectSubnet(selector));

    // The option selection has precedence
    selector.option_select_ = IPAddress("192.0.2.130");
    EXPECT_EQ(subnet3, mgr.selectSubnet(selector));

    // Over relay-info too
    selector.giaddr_ = IPAddress("10.0.0.1");
    subnet2->setRelayInfo(IPAddress("10.0.0.1"));
    EXPECT_EQ(subnet3, mgr.selectSubnet(selector));
    selector.option_select_ = IPAddress("0.0.0.0");
    EXPECT_EQ(subnet2, mgr.selectSubnet(selector));

    // Check that a not matching option selection it shall fail
    selector.option_select_ = IPAddress("10.0.0.1");
    EXPECT_FALSE(mgr.selectSubnet(selector));
}

// This test verifies that the relay information can be used to retrieve the
// subnet.
TEST(SubnetMgrTest, selectSubnetByRelayAddress) {
    SubnetMgr mgr;
    Subnet4* subnet1 = new Subnet4(IPAddress("192.0.2.0"), 26, 1, 2, 3);
    Subnet4* subnet2 = new Subnet4(IPAddress("192.0.2.64"), 26, 1, 2, 3);
    Subnet4* subnet3 = new Subnet4(IPAddress("192.0.2.128"), 26, 1, 2, 3);
    mgr.add(Subnet4Ptr(subnet1));
    mgr.add(Subnet4Ptr(subnet2));     
    mgr.add(Subnet4Ptr(subnet3));   

    SubnetSelector selector;

    // Check that without relay-info specified, subnets are not selected
    selector.giaddr_ = IPAddress("10.0.0.1");
    EXPECT_FALSE(mgr.selectSubnet(selector));
    selector.giaddr_ = IPAddress("10.0.0.2");
    EXPECT_FALSE(mgr.selectSubnet(selector));
    selector.giaddr_ = IPAddress("10.0.0.3");
    EXPECT_FALSE(mgr.selectSubnet(selector));

    // Now specify relay info
    subnet1->setRelayInfo(IPAddress("10.0.0.1"));
    subnet2->setRelayInfo(IPAddress("10.0.0.2"));
    subnet3->setRelayInfo(IPAddress("10.0.0.3"));

    // And try again. This time relay-info is there and should match.
    selector.giaddr_ = IPAddress("10.0.0.1");
    EXPECT_EQ(subnet1, mgr.selectSubnet(selector));
    selector.giaddr_ = IPAddress("10.0.0.2");
    EXPECT_EQ(subnet2, mgr.selectSubnet(selector));
    selector.giaddr_ = IPAddress("10.0.0.3");
    EXPECT_EQ(subnet3, mgr.selectSubnet(selector));
}

TEST(SubnetMgrTest, selectSubnetNoCiaddr) {
    SubnetMgr mgr;

    // Create 3 subnets.
    Subnet4* subnet1 = new Subnet4(IPAddress("192.0.2.0"), 26, 1, 2, 3);
    Subnet4* subnet2 = new Subnet4(IPAddress("192.0.2.64"), 26, 1, 2, 3);
    Subnet4* subnet3 = new Subnet4(IPAddress("192.0.2.128"), 26, 1, 2, 3);

    SubnetSelector selector;
    selector.remote_address_ = IPAddress("192.0.2.0");
    selector.local_address_ = IPAddress("10.0.0.100");
    ASSERT_FALSE(mgr.selectSubnet(selector));

    mgr.add(Subnet4Ptr(subnet1));     
    selector.remote_address_ = IPAddress("192.0.2.63");
    EXPECT_EQ(subnet1, mgr.selectSubnet(selector));

    mgr.add(Subnet4Ptr(subnet2));   
    mgr.add(Subnet4Ptr(subnet3));   

    selector.remote_address_ = IPAddress("192.0.2.15");
    EXPECT_EQ(subnet1, mgr.selectSubnet(selector));
    selector.remote_address_ = IPAddress("192.0.2.85");
    EXPECT_EQ(subnet2, mgr.selectSubnet(selector));
    selector.remote_address_ = IPAddress("192.0.2.191");
    EXPECT_EQ(subnet3, mgr.selectSubnet(selector));

    selector.remote_address_ = IPAddress("192.0.2.192");
    EXPECT_FALSE(mgr.selectSubnet(selector));
}

TEST(SubnetMgrTest, selectSubnetInterface) {
    /*
    IfaceMgrTestConfig config(true);

    SubnetMgr mgr;
    SubnetSelector selector;

    selector.iface_name_ = "eth0";
    EXPECT_FALSE(mgr.selectSubnet(selector));
    selector.iface_name_ = "eth1";
    EXPECT_FALSE(mgr.selectSubnet(selector));

    Subnet4Ptr subnet1(new Subnet4(IPAddress("10.0.0.1"), 24, 1, 2, 3));
    mgr.add(subnet1);

    selector.iface_name_ = "eth0";
    Subnet4Ptr subnet1_ret = mgr.selectSubnet(selector);
    ASSERT_TRUE(subnet1_ret);
    EXPECT_EQ(subnet1->get().first, subnet1_ret->get().first);
    selector.iface_name_ = "eth1";
    EXPECT_FALSE(mgr.selectSubnet(selector));

    Subnet4Ptr subnet2(new Subnet4(IPAddress("192.0.2.1"), 24, 1, 2, 3));
    mgr.add(subnet2);

    selector.iface_name_ = "eth0";
    subnet1_ret = mgr.selectSubnet(selector);
    ASSERT_TRUE(subnet1_ret);
    EXPECT_EQ(subnet1->get().first, subnet1_ret->get().first);
    selector.iface_name_ = "eth1";
    Subnet4Ptr subnet2_ret = mgr.selectSubnet(selector);
    ASSERT_TRUE(subnet2_ret);
    EXPECT_EQ(subnet2->get().first, subnet2_ret->get().first);

    selector.iface_name_ = "bogus-interface";
    EXPECT_THROW(mgr.selectSubnet(selector), kea::BadValue);
    */
}

TEST(SubnetMgrTest, duplication) {
    SubnetMgr mgr;

    Subnet4Ptr subnet1(new Subnet4(IPAddress("192.0.2.0"), 26, 1, 2, 3, 123));
    Subnet4Ptr subnet2(new Subnet4(IPAddress("192.0.2.64"), 26, 1, 2, 3, 124));
    Subnet4Ptr subnet3(new Subnet4(IPAddress("192.0.2.128"), 26, 1, 2, 3, 123));

    ASSERT_NO_THROW(mgr.add(std::move(subnet1)));
    EXPECT_NO_THROW(mgr.add(std::move(subnet2)));
    EXPECT_THROW(mgr.add(std::move(subnet3)), kea::server::DuplicateSubnetID);
}
};
