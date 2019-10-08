#include <kea/dhcp++/option_data_types.h>
#include <kea/util/io_utilities.h>
#include <gtest/gtest.h>
#include <cstdint>

using namespace std;
using namespace kea;
using namespace kea::dhcp;

namespace kea {

TEST(OptionDataTypesTest, getLabelCount) {
    EXPECT_EQ(0, OptionDataTypeUtil::getLabelCount(""));
    EXPECT_EQ(1, OptionDataTypeUtil::getLabelCount("."));
    EXPECT_EQ(2, OptionDataTypeUtil::getLabelCount("example"));
    EXPECT_EQ(3, OptionDataTypeUtil::getLabelCount("example.com"));
    EXPECT_EQ(3, OptionDataTypeUtil::getLabelCount("example.com."));
    EXPECT_EQ(4, OptionDataTypeUtil::getLabelCount("myhost.example.com"));
    EXPECT_THROW(OptionDataTypeUtil::getLabelCount(".abc."), kea::dhcp::BadDataTypeCast);
}

TEST(OptionDataTypesTest, readAddress) {
    folly::IPAddress address("192.168.0.1");
    std::vector<uint8_t> buf;
    OptionDataTypeUtil::writeAddress(address, buf);
    folly::IPAddress address_out;
    EXPECT_NO_THROW(address_out = OptionDataTypeUtil::readAddress(buf, AF_INET));
    EXPECT_EQ(address, address_out);

    EXPECT_THROW(
        OptionDataTypeUtil::readAddress(buf, AF_INET6),
        kea::dhcp::BadDataTypeCast
    );
}

TEST(OptionDataTypesTest, writeAddress) {
    const uint8_t data[] = {
        0x20, 0x01, 0x0d, 0xb8, 0x0, 0x1, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1
    };
    std::vector<uint8_t> buf_in(data, data + sizeof(data));
    std::vector<uint8_t> buf_out;

    folly::IPAddress address("192.168.0.1");
    ASSERT_NO_THROW(OptionDataTypeUtil::writeAddress(address, buf_out));
    ASSERT_EQ(4, buf_out.size());
    EXPECT_EQ(192, buf_out[0]);
    EXPECT_EQ(168, buf_out[1]);
    EXPECT_EQ(0, buf_out[2]);
    EXPECT_EQ(1, buf_out[3]);
}

TEST(OptionDataTypesTest, writeBinary) {
    const char data[] = {
        0x0, 0x1, 0x2, 0x3, 0x4, 0x5,
        0x6, 0x7, 0x8, 0x9, 0xA, 0xB
    };
    std::vector<uint8_t> buf_ref(data, data + sizeof(data));
    std::vector<uint8_t> buf;
    ASSERT_NO_THROW(
        OptionDataTypeUtil::writeBinary("000102030405060708090A0B", buf)
    );
    ASSERT_EQ(buf_ref.size(), buf.size());
    EXPECT_TRUE(std::equal(buf_ref.begin(), buf_ref.end(), buf.begin()));
}

TEST(OptionDataTypesTest, readBool) {
    std::vector<uint8_t> buf;
    buf.push_back(1);

    bool value = false;
    ASSERT_NO_THROW(
        value = OptionDataTypeUtil::readBool(buf);
    );
    EXPECT_TRUE(value);
    buf[0] = 0;
    ASSERT_NO_THROW(
        value = OptionDataTypeUtil::readBool(buf);
    );
    EXPECT_FALSE(value);

    buf[0] = 5;
    ASSERT_THROW(
        OptionDataTypeUtil::readBool(buf),
        kea::dhcp::BadDataTypeCast
    );
}

TEST(OptionDataTypesTest, writeBool) {
    std::vector<uint8_t> buf;
    ASSERT_NO_THROW(OptionDataTypeUtil::writeBool(true, buf));
    ASSERT_EQ(1, buf.size());
    EXPECT_EQ(buf[0], 1);
    ASSERT_NO_THROW(OptionDataTypeUtil::writeBool(false, buf));
    ASSERT_EQ(2, buf.size());
    EXPECT_EQ(buf[0], 1);
    EXPECT_EQ(buf[1], 0);
}

TEST(OptionDataTypesTest, readInt) {
    std::vector<uint8_t> buf;

    OptionDataTypeUtil::writeInt<uint8_t>(129, buf);
    uint8_t valueUint8 = 0;
    ASSERT_NO_THROW(
        valueUint8 = OptionDataTypeUtil::readInt<uint8_t>(buf);
    );
    EXPECT_EQ(129, valueUint8);

    EXPECT_THROW(
        OptionDataTypeUtil::readInt<uint16_t>(buf),
        kea::dhcp::BadDataTypeCast
    );

    buf.clear();
    OptionDataTypeUtil::writeInt<uint16_t>(1234, buf);
    uint16_t valueUint16 = 0;
    ASSERT_NO_THROW(
        valueUint16 = OptionDataTypeUtil::readInt<uint16_t>(buf);
    );
    EXPECT_EQ(1234, valueUint16);

    EXPECT_THROW(
        OptionDataTypeUtil::readInt<uint32_t>(buf),
        kea::dhcp::BadDataTypeCast
    );

    buf.clear();
    OptionDataTypeUtil::writeInt<uint32_t>(56789, buf);
    uint32_t valueUint32 = 0;
    ASSERT_NO_THROW(
        valueUint32 = OptionDataTypeUtil::readInt<uint32_t>(buf);
    );
    EXPECT_EQ(56789, valueUint32);
    buf.clear();

    OptionDataTypeUtil::writeInt<int8_t>(-65, buf);
    int8_t valueInt8 = 0;
    ASSERT_NO_THROW(
        valueInt8 = OptionDataTypeUtil::readInt<int8_t>(buf);
    );
    EXPECT_EQ(-65, valueInt8);
    buf.clear();

    EXPECT_THROW(
        OptionDataTypeUtil::readInt<int16_t>(buf),
        kea::dhcp::BadDataTypeCast
    );

    OptionDataTypeUtil::writeInt<int16_t>(2345, buf);
    int32_t valueInt16 = 0;
    ASSERT_NO_THROW(
        valueInt16 = OptionDataTypeUtil::readInt<int16_t>(buf);
    );
    EXPECT_EQ(2345, valueInt16);
    buf.clear();

    EXPECT_THROW(
        OptionDataTypeUtil::readInt<int32_t>(buf),
        kea::dhcp::BadDataTypeCast
    );

    OptionDataTypeUtil::writeInt<int32_t>(-16543, buf);
    int32_t valueInt32 = 0;
    ASSERT_NO_THROW(
        valueInt32 = OptionDataTypeUtil::readInt<int32_t>(buf);
    );
    EXPECT_EQ(-16543, valueInt32);

    buf.clear();
}

TEST(OptionDataTypesTest, writeInt) {
    const uint8_t data[] = {
        0x7F, // 127
        0x03, 0xFF, // 1023
        0x00, 0x00, 0x10, 0x00, // 4096
        0xFF, 0xFF, 0xFC, 0x00, // -1024
        0x02, 0x00, // 512
        0x81 // -127
    };
    std::vector<uint8_t> buf_ref(data, data + sizeof(data));

    std::vector<uint8_t> buf;
    ASSERT_NO_THROW(OptionDataTypeUtil::writeInt<uint8_t>(127, buf));
    ASSERT_NO_THROW(OptionDataTypeUtil::writeInt<uint16_t>(1023, buf));
    ASSERT_NO_THROW(OptionDataTypeUtil::writeInt<uint32_t>(4096, buf));
    ASSERT_NO_THROW(OptionDataTypeUtil::writeInt<int32_t>(-1024, buf));
    ASSERT_NO_THROW(OptionDataTypeUtil::writeInt<int16_t>(512, buf));
    ASSERT_NO_THROW(OptionDataTypeUtil::writeInt<int8_t>(-127, buf));

    ASSERT_EQ(buf_ref.size(), buf.size());
    EXPECT_TRUE(std::equal(buf_ref.begin(), buf_ref.end(), buf.begin()));
}

TEST(OptionDataTypesTest, readFqdn) {
    // The binary representation of the "mydomain.example.com".
    // Values: 8, 7, 3 and 0 specify the lengths of subsequent
    // labels within the FQDN.
    const char data[] = {
        8, 109, 121, 100, 111, 109, 97, 105, 110, // "mydomain"
        7, 101, 120, 97, 109, 112, 108, 101,      // "example"
        3, 99, 111, 109,                          // "com"
        0
    };

    std::vector<uint8_t> buf(data, data + sizeof(data));

    std::string fqdn;
    EXPECT_NO_THROW(fqdn = OptionDataTypeUtil::readFqdn(buf));
    EXPECT_EQ("mydomain.example.com.", fqdn);

    buf.resize(5);
    EXPECT_THROW(
        OptionDataTypeUtil::readFqdn(buf),
        kea::dhcp::BadDataTypeCast
    );

    buf.clear();
    EXPECT_THROW(
        OptionDataTypeUtil::readFqdn(buf),
        kea::dhcp::BadDataTypeCast
    );
}

TEST(OptionDataTypesTest, writeFqdn) {
    std::vector<uint8_t> buf;
    EXPECT_NO_THROW(
        OptionDataTypeUtil::writeFqdn("mydomain.example.com", buf)
    );
    ASSERT_EQ(22, buf.size());

    EXPECT_EQ(8, buf[0]);  // length of "mydomain"
    EXPECT_EQ(7, buf[9]);  // length of "example"
    EXPECT_EQ(3, buf[17]); // length of "com"
    EXPECT_EQ(0, buf[21]); // zero byte at the end.

    std::string label0(buf.begin() + 1, buf.begin() + 9);
    EXPECT_EQ("mydomain", label0);

    std::string label1(buf.begin() + 10, buf.begin() + 17);
    EXPECT_EQ("example", label1);

    std::string label2(buf.begin() + 18, buf.begin() + 21);
    EXPECT_EQ("com", label2);

    OptionDataTypeUtil::writeFqdn("hello.net", buf);

    ASSERT_EQ(33, buf.size());

    EXPECT_EQ(5, buf[22]);
    EXPECT_EQ(3, buf[28]);

    std::string label3(buf.begin() + 23, buf.begin() + 28);
    EXPECT_EQ("hello", label3);

    std::string label4(buf.begin() + 29, buf.begin() + 32);
    EXPECT_EQ("net", label4);

    buf.clear();
    EXPECT_THROW(
        OptionDataTypeUtil::writeFqdn("", buf),
        kea::dhcp::BadDataTypeCast
    );

    buf.clear();
    EXPECT_THROW(
        OptionDataTypeUtil::writeFqdn("example..com", buf),
        kea::dhcp::BadDataTypeCast
    );
}

TEST(OptionDataTypesTest, readString) {
    std::vector<uint8_t> buf;
    OptionDataTypeUtil::writeString("hello world", buf);

    std::string value;
    ASSERT_NO_THROW(
        value = OptionDataTypeUtil::readString(buf);
    );
    EXPECT_EQ("hello world", value);
}

TEST(OptionDataTypesTest, writeString) {
    std::vector<uint8_t> buf_ref;
    OptionDataTypeUtil::writeString("hello world!", buf_ref);
    std::vector<uint8_t> buf;
    ASSERT_NO_THROW(OptionDataTypeUtil::writeString("hello world!", buf));
    ASSERT_EQ(buf_ref.size(), buf.size());
    EXPECT_TRUE(std::equal(buf_ref.begin(), buf_ref.end(), buf.begin()));
}

}; 
