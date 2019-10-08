#include <kea/dhcp++/hwaddr.h>
#include <kea/dhcp++/dhcp4.h>
#include <kea/exceptions/exceptions.h>
#include <gtest/gtest.h>
#include <folly/IPAddress.h>

#include <iostream>
#include <sstream>

using namespace std;
using namespace kea;
using namespace kea::dhcp;
using namespace folly;


namespace kea {

TEST(HWAddrTest, constructor) {
    const uint8_t data1[] = {0, 1, 2, 3, 4, 5, 6};
    const uint8_t htype = HTYPE_ETHER;
    vector<uint8_t> data2(data1, data1 + sizeof(data1));

    vector<uint8_t> big_data_vector(HWAddr::MAX_HWADDR_LEN + 1, 0); 

    unique_ptr<HWAddr> hwaddr1(new HWAddr(data1, sizeof(data1), htype));
    unique_ptr<HWAddr> hwaddr2(new HWAddr(data2, htype));
    unique_ptr<HWAddr> hwaddr3(new HWAddr());

    EXPECT_TRUE(data2 == hwaddr1->hwaddr_);
    EXPECT_EQ(htype, hwaddr1->htype_);

    EXPECT_TRUE(data2 == hwaddr2->hwaddr_);
    EXPECT_EQ(htype, hwaddr2->htype_);

    EXPECT_EQ(0, hwaddr3->hwaddr_.size());
    EXPECT_EQ(htype, hwaddr3->htype_);

    EXPECT_THROW(HWAddr(&big_data_vector[0], big_data_vector.size(), HTYPE_ETHER), 
        BadValue);

    EXPECT_THROW(HWAddr(big_data_vector, HTYPE_ETHER), BadValue);
}

TEST(HWAddrTest, operators) {
    uint8_t data1[] = {0, 1, 2, 3, 4, 5, 6};
    uint8_t data2[] = {0, 1, 2, 3, 4};
    uint8_t data3[] = {0, 1, 2, 3, 4, 5, 7}; // last digit different
    uint8_t data4[] = {0, 1, 2, 3, 4, 5, 6}; // the same as 1

    uint8_t htype1 = HTYPE_ETHER;
    uint8_t htype2 = HTYPE_FDDI;

    unique_ptr<HWAddr> hw1(new HWAddr(data1, sizeof(data1), htype1));
    unique_ptr<HWAddr> hw2(new HWAddr(data2, sizeof(data2), htype1));
    unique_ptr<HWAddr> hw3(new HWAddr(data3, sizeof(data3), htype1));
    unique_ptr<HWAddr> hw4(new HWAddr(data4, sizeof(data4), htype1));

    unique_ptr<HWAddr> hw5(new HWAddr(data4, sizeof(data4), htype2));

    EXPECT_TRUE(*hw1 == *hw4);
    EXPECT_FALSE(*hw1 == *hw2);
    EXPECT_FALSE(*hw1 == *hw3);

    EXPECT_FALSE(*hw1 != *hw4);
    EXPECT_TRUE(*hw1 != *hw2);
    EXPECT_TRUE(*hw1 != *hw3);

    EXPECT_FALSE(*hw1 == *hw5);
    EXPECT_FALSE(*hw4 == *hw5);

    EXPECT_TRUE(*hw1 != *hw5);
    EXPECT_TRUE(*hw4 != *hw5);
}

TEST(HWAddrTest, toText) {
    uint8_t data[] = {0, 1, 2, 3, 4, 5};
    uint8_t htype = 15;

    std::unique_ptr<HWAddr> hw(new HWAddr(data, sizeof(data), htype));
    EXPECT_EQ("hwtype=15 00:01:02:03:04:05", hw->toText());
    EXPECT_EQ("00:01:02:03:04:05", hw->toText(false));
}

TEST(HWAddrTest, stringConversion) {
    HWAddr hwaddr;
    std::string result = hwaddr.toText();
    EXPECT_EQ(std::string("hwtype=1 "), result);

    hwaddr.hwaddr_.push_back(0xc3);
    result = hwaddr.toText();
    EXPECT_EQ(std::string("hwtype=1 c3"), result);

    hwaddr.hwaddr_.push_back(0x7);
    hwaddr.hwaddr_.push_back(0xa2);
    hwaddr.hwaddr_.push_back(0xe8);
    hwaddr.hwaddr_.push_back(0x42);
    result = hwaddr.toText();
    EXPECT_EQ(std::string("hwtype=1 c3:07:a2:e8:42"), result);
}

TEST(HWAddrTest, fromText) {
    unique_ptr<HWAddr> hwaddr;
    ASSERT_NO_THROW(
        hwaddr = HWAddr::fromText("00:01:A:bc:d:67");
    );
    EXPECT_EQ("00:01:0a:bc:0d:67", hwaddr->toText(false));

    ASSERT_NO_THROW(
        hwaddr = (HWAddr::fromText(""));
    );
    EXPECT_TRUE(hwaddr->toText(false).empty());

    EXPECT_THROW(
       (hwaddr = HWAddr::fromText("00::01:00:bc:0d:67")),
       kea::BadValue
    );

    EXPECT_THROW(
       (hwaddr = HWAddr::fromText("00:01:00A:bc:0d:67")),
       kea::BadValue
    );

}

TEST(HWAddrTest, 16bits) {

    uint8_t data[] = {0, 1, 2, 3, 4, 5};
    uint16_t htype = 257;
    std::unique_ptr<HWAddr> hw(new HWAddr(data, sizeof(data), htype));

    EXPECT_EQ("hwtype=257 00:01:02:03:04:05", hw->toText());
}

}
