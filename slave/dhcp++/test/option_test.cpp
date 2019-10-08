#include <kea/dhcp++/dhcp4.h>
#include <kea/dhcp++/libdhcp++.h>
#include <kea/dhcp++/option.h>
#include <kea/exceptions/exceptions.h>
#include <kea/util/buffer.h>

#include <gtest/gtest.h>

#include <iostream>
#include <sstream>

#include <arpa/inet.h>

using namespace std;
using namespace kea;
using namespace kea::dhcp;
using namespace kea::util;

namespace kea {

/// @brief A class which derives from option and exposes protected members.
class NakedOption : public Option {
public:
    NakedOption() : Option(258) {
    }

    using Option::unpackOptions;
};

class OptionTest : public ::testing::Test {
public:
    OptionTest(): buf_(255), outBuf_(255) {
        for (unsigned i = 0; i < 255; i++) {
            buf_[i] = 255 - i;
        }
    }
    OptionBuffer buf_;
    OutputBuffer outBuf_;
};

// Basic tests for V4 functionality
TEST_F(OptionTest, v4_basic) {

    unique_ptr<Option> opt;
    EXPECT_NO_THROW(opt.reset(new Option(17)));

    EXPECT_EQ(17, opt->getType());
    EXPECT_EQ(0, opt->getData().size());
    EXPECT_EQ(2, opt->len()); // just v4 header

    EXPECT_NO_THROW(opt.reset());

    // V4 options have type 0...255
    EXPECT_THROW(opt.reset(new Option(256)), BadValue);

    // 0 is a special PAD option
    EXPECT_THROW(opt.reset(new Option(0)), BadValue);

    // 255 is a special END option
    EXPECT_THROW(opt.reset(new Option(255)), BadValue);
}

const uint8_t dummyPayload[] =
{ 1, 2, 3, 4};

TEST_F(OptionTest, v4_data1) {

    vector<uint8_t> data(dummyPayload, dummyPayload + sizeof(dummyPayload));

    unique_ptr<Option> opt;

    // Create DHCPv4 option of type 123 that contains 4 bytes of data.
    ASSERT_NO_THROW(opt.reset(new Option(123, data)));

    // Check that content is reported properly
    EXPECT_EQ(123, opt->getType());
    vector<uint8_t> optData = opt->getData();
    ASSERT_EQ(optData.size(), data.size());
    EXPECT_TRUE(optData == data);
    EXPECT_EQ(2, opt->getHeaderLen());
    EXPECT_EQ(6, opt->len());

    // Now store that option into a buffer
    OutputBuffer buf(100);
    EXPECT_NO_THROW(opt->pack(buf));

    // Check content of that buffer:
    // 2 byte header + 4 bytes data
    ASSERT_EQ(6, buf.getLength());

    // That's how this option is supposed to look like
    uint8_t exp[] = { 123, 4, 1, 2, 3, 4 };

    /// TODO: use vector<uint8_t> getData() when it will be implemented
    EXPECT_EQ(0, memcmp(exp, buf.getData(), 6));

    // Check that we can destroy that option
    EXPECT_NO_THROW(opt.reset());
}

// This is almost the same test as v4_data1, but it uses a different
// constructor
TEST_F(OptionTest, v4_data2) {

    vector<uint8_t> data(dummyPayload, dummyPayload + sizeof(dummyPayload));

    vector<uint8_t> expData = data;

    // Add fake data in front and end. Main purpose of this test is to check
    // that only subset of the whole vector can be used for creating option.
    data.insert(data.begin(), 56);
    data.push_back(67);

    // Data contains extra garbage at beginning and at the end. It should be
    // ignored, as we pass interators to proper data. Only subset (limited by
    // iterators) of the vector should be used.
    // expData contains expected content (just valid data, without garbage).
    unique_ptr<Option> opt;

    // Create DHCPv4 option of type 123 that contains
    // 4 bytes (sizeof(dummyPayload).
    ASSERT_NO_THROW(
        opt.reset(new Option(123, data.begin() + 1,
                             data.end() - 1));
    );

    // Check that content is reported properly
    EXPECT_EQ(123, opt->getType());
    vector<uint8_t> optData = opt->getData();
    ASSERT_EQ(optData.size(), expData.size());
    EXPECT_TRUE(optData == expData);
    EXPECT_EQ(2, opt->getHeaderLen());
    EXPECT_EQ(6, opt->len());

    // Now store that option into a buffer
    OutputBuffer buf(100);
    EXPECT_NO_THROW(opt->pack(buf));

    // Check content of that buffer

    // 2 byte header + 4 bytes data
    ASSERT_EQ(6, buf.getLength());

    // That's how this option is supposed to look like
    uint8_t exp[] = { 123, 4, 1, 2, 3, 4 };

    /// TODO: use vector<uint8_t> getData() when it will be implemented
    EXPECT_EQ(0, memcmp(exp, buf.getData(), 6));

    // Check that we can destroy that option
    EXPECT_NO_THROW(opt.reset());
}

TEST_F(OptionTest, v4_toText) {

    vector<uint8_t> buf(3);
    buf[0] = 0;
    buf[1] = 0xf;
    buf[2] = 0xff;

    Option opt(253, buf);

    EXPECT_EQ("type=253, len=003: 00:0f:ff", opt.toText());
}

// Test converting option to the hexadecimal representation.
TEST_F(OptionTest, v4_toHexString) {
    std::vector<uint8_t> payload;
    for (unsigned int i = 0; i < 16; ++i) {
        payload.push_back(static_cast<uint8_t>(i));
    }
    Option opt(122, payload);
    EXPECT_EQ("0x000102030405060708090A0B0C0D0E0F", opt.toHexString());
    EXPECT_EQ("0x7A10000102030405060708090A0B0C0D0E0F",
              opt.toHexString(true));

    // Test empty option.
    Option opt_empty(65, std::vector<uint8_t>());
    EXPECT_TRUE(opt_empty.toHexString().empty());
    EXPECT_EQ("0x4100", opt_empty.toHexString(true));

    // Test too long option. We can't simply create such option by
    // providing a long payload, because class constructor would not
    // accept it. Instead we'll add two long sub options after we
    // create an option instance.
    Option opt_too_long(33);
    // Both suboptions have payloads of 150 bytes.
    std::vector<uint8_t> long_payload(150, 1);
    unique_ptr<Option> sub1(new Option(100, long_payload));
    unique_ptr<Option> sub2(new Option(101, long_payload));
    opt_too_long.addOption(std::move(sub1));
    opt_too_long.addOption(std::move(sub2));

    // The toHexString() should throw exception.
    EXPECT_THROW(opt_too_long.toHexString(), kea::OutOfRange);
}




// Check that an option can contain 2 suboptions:
// opt1
//  +----opt2
//  |
//  +----opt3
//
TEST_F(OptionTest, v4_suboptions1) {
    for (unsigned i = 0; i < 128; i++) {
        buf_[i] = 100 + i;
    }

    unique_ptr<Option> opt1(new Option(232, // Type
                                       buf_.begin(), // 3 bytes of data
                                       buf_.begin() + 3));
    unique_ptr<Option> opt2(new Option(13));
    unique_ptr<Option> opt3(new Option(7,
                              buf_.begin() + 3,
                              buf_.begin() + 8)); // 5 bytes of data

    // opt2 len = 4 (just header)
    // opt3 len = 9 4(header)+5(data)
    EXPECT_EQ(2, opt2->len());
    EXPECT_EQ(7, opt3->len());

    opt1->addOption(std::move(opt2));
    opt1->addOption(std::move(opt3));
    // opt1 len = 7 + suboptions() = 7 + 4 + 9 = 20
    EXPECT_EQ(14, opt1->len());

    uint8_t expected[] = {
        0xe8, 12, 100, 101, 102,
        7, 5, 103, 104, 105, 106, 107,
        13, 0  
    };

    opt1->pack(outBuf_);
    EXPECT_EQ(14, outBuf_.getLength());
    // Payload
    EXPECT_EQ(0, memcmp(outBuf_.getData(), expected, 14) );

    EXPECT_NO_THROW(opt1.reset());
}

// Check that an option can contain nested suboptions:
// opt1
//  +----opt2
//        |
//        +----opt3
//
TEST_F(OptionTest, v4_suboptions2) {
    for (unsigned i = 0; i < 128; i++) {
        buf_[i] = 100 + i;
    }

    unique_ptr<Option> opt1(new Option(232, // Type
                                       buf_.begin(), buf_.begin() + 3));
    unique_ptr<Option> opt2(new Option(13));
    unique_ptr<Option> opt3(new Option(7,
                              buf_.begin() + 3,
                              buf_.begin() + 8));
    opt2->addOption(std::move(opt3));
    opt1->addOption(std::move(opt2));
    // opt3 len = 9 4(header)+5(data)
    // opt2 len = 4 (just header) + len(opt3)
    // opt1 len = 7 + len(opt2)

    uint8_t expected[] = {
        0xe8, 12, 100, 101, 102,
        13, 7, 
        7, 5, 103, 104, 105, 106, 107 
    };
    opt1->pack(outBuf_);
    EXPECT_EQ(14, outBuf_.getLength());

    // Payload
    EXPECT_EQ(0, memcmp(outBuf_.getData(), expected, 14) );

    EXPECT_NO_THROW(opt1.reset());
}



TEST_F(OptionTest, getUintX) {

    buf_[0] = 0x5;
    buf_[1] = 0x4;
    buf_[2] = 0x3;
    buf_[3] = 0x2;
    buf_[4] = 0x1;

    // Five options with varying lengths
    unique_ptr<Option> opt1(new Option(232, buf_.begin(), buf_.begin() + 1));
    unique_ptr<Option> opt2(new Option(232, buf_.begin(), buf_.begin() + 2));
    unique_ptr<Option> opt3(new Option(232, buf_.begin(), buf_.begin() + 3));
    unique_ptr<Option> opt4(new Option(232, buf_.begin(), buf_.begin() + 4));
    unique_ptr<Option> opt5(new Option(232, buf_.begin(), buf_.begin() + 5));

    EXPECT_EQ(5, opt1->getUint8());
    EXPECT_THROW(opt1->getUint16(), OutOfRange);
    EXPECT_THROW(opt1->getUint32(), OutOfRange);

    EXPECT_EQ(5, opt2->getUint8());
    EXPECT_EQ(0x0504, opt2->getUint16());
    EXPECT_THROW(opt2->getUint32(), OutOfRange);

    EXPECT_EQ(5, opt3->getUint8());
    EXPECT_EQ(0x0504, opt3->getUint16());
    EXPECT_THROW(opt3->getUint32(), OutOfRange);

    EXPECT_EQ(5, opt4->getUint8());
    EXPECT_EQ(0x0504, opt4->getUint16());
    EXPECT_EQ(0x05040302, opt4->getUint32());

    // The same as for 4-byte long, just get first 1,2 or 4 bytes
    EXPECT_EQ(5, opt5->getUint8());
    EXPECT_EQ(0x0504, opt5->getUint16());
    EXPECT_EQ(0x05040302, opt5->getUint32());

}

TEST_F(OptionTest, setUintX) {
    unique_ptr<Option> opt1(new Option(125));
    unique_ptr<Option> opt2(new Option(125));
    unique_ptr<Option> opt4(new Option(125));

    // Verify setUint8
    opt1->setUint8(255);
    EXPECT_EQ(255, opt1->getUint8());
    opt1->pack(outBuf_);
    EXPECT_EQ(3, opt1->len());
    EXPECT_EQ(3, outBuf_.getLength());
    uint8_t exp1[] = {125, 1, 255};
    EXPECT_TRUE(0 == memcmp(exp1, outBuf_.getData(), 3));

    // Verify getUint16
    outBuf_.clear();
    opt2->setUint16(12345);
    opt2->pack(outBuf_);
    EXPECT_EQ(12345, opt2->getUint16());
    EXPECT_EQ(4, opt2->len());
    EXPECT_EQ(4, outBuf_.getLength());
    uint8_t exp2[] = {125, 2, 12345/256, 12345%256};
    EXPECT_TRUE(0 == memcmp(exp2, outBuf_.getData(), 4));

    // Verify getUint32
    outBuf_.clear();
    opt4->setUint32(0x12345678);
    opt4->pack(outBuf_);
    EXPECT_EQ(0x12345678, opt4->getUint32());
    EXPECT_EQ(6, opt4->len());
    EXPECT_EQ(6, outBuf_.getLength());
    uint8_t exp4[] = {125, 4, 0x12, 0x34, 0x56, 0x78};
    EXPECT_TRUE(0 == memcmp(exp4, outBuf_.getData(), 6));
}

TEST_F(OptionTest, setData) {
    // Verify data override with new buffer larger than initial option buffer
    // size.
    unique_ptr<Option> opt1(new Option(125,
                              buf_.begin(), buf_.begin() + 10));
    buf_.resize(20, 1);
    opt1->setData(buf_.begin(), buf_.end());
    opt1->pack(outBuf_);
    ASSERT_EQ(outBuf_.getLength() - opt1->getHeaderLen(), buf_.size());
    const uint8_t* test_data = static_cast<const uint8_t*>(outBuf_.getData());
    EXPECT_TRUE(0 == memcmp(&buf_[0], test_data + opt1->getHeaderLen(),
                            buf_.size()));

    // Verify data override with new buffer shorter than initial option buffer
    // size.
    unique_ptr<Option> opt2(new Option(125,
                              buf_.begin(), buf_.begin() + 10));
    outBuf_.clear();
    buf_.resize(5, 1);
    opt2->setData(buf_.begin(), buf_.end());
    opt2->pack(outBuf_);
    ASSERT_EQ(outBuf_.getLength() - opt1->getHeaderLen(), buf_.size());
    test_data = static_cast<const uint8_t*>(outBuf_.getData());
    EXPECT_TRUE(0 == memcmp(&buf_[0], test_data + opt1->getHeaderLen(),
                            buf_.size()));
}

// This test verifies that options can be compared using equals(unique_ptr<Option>)
// method.
TEST_F(OptionTest, equalsWithPointers) {

    // Five options with varying lengths
    unique_ptr<Option> opt1(new Option(232, buf_.begin(), buf_.begin() + 1));
    unique_ptr<Option> opt2(new Option(232, buf_.begin(), buf_.begin() + 2));
    unique_ptr<Option> opt3(new Option(232, buf_.begin(), buf_.begin() + 3));

    // The same content as opt2, but different type
    unique_ptr<Option> opt4(new Option(1, buf_.begin(), buf_.begin() + 2));

    // Another instance with the same type and content as opt2
    unique_ptr<Option> opt5(new Option(232, buf_.begin(), buf_.begin() + 2));

    EXPECT_TRUE(opt1->equals(*opt1));

    EXPECT_FALSE(opt1->equals(*opt2));
    EXPECT_FALSE(opt1->equals(*opt3));
    EXPECT_FALSE(opt1->equals(*opt4));

    EXPECT_TRUE(opt2->equals(*opt5));
}

// This test verifies that options can be compared using equals(Option) method.
TEST_F(OptionTest, equals) {

    // Five options with varying lengths
    Option opt1(232, buf_.begin(), buf_.begin() + 1);
    Option opt2(232, buf_.begin(), buf_.begin() + 2);
    Option opt3(232, buf_.begin(), buf_.begin() + 3);

    // The same content as opt2, but different type
    Option opt4(1, buf_.begin(), buf_.begin() + 2);

    // Another instance with the same type and content as opt2
    Option opt5(232, buf_.begin(), buf_.begin() + 2);

    EXPECT_TRUE(opt1.equals(opt1));

    EXPECT_FALSE(opt1.equals(opt2));
    EXPECT_FALSE(opt1.equals(opt3));
    EXPECT_FALSE(opt1.equals(opt4));

    EXPECT_TRUE(opt2.equals(opt5));
}

// This test verifies that the name of the option space being encapsulated by
// the particular option can be set.
TEST_F(OptionTest, setEncapsulatedSpace) {
    Option optv4(125);
    EXPECT_TRUE(optv4.getEncapsulatedSpace().empty());

    optv4.setEncapsulatedSpace("dhcp4");
    EXPECT_EQ("dhcp4", optv4.getEncapsulatedSpace());

}
};
