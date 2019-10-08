#include <kea/dhcp++/option.h>
#include <kea/dhcp++/option_int.h>
#include <kea/util/buffer.h>

#include <gtest/gtest.h>

using namespace std;
using namespace kea;
using namespace kea::dhcp;
using namespace kea::util;

namespace kea{

const uint16_t TEST_OPT_CODE = 232;

class OptionIntTest : public ::testing::Test {
public:
    OptionIntTest(): buf_(255), out_buf_(255) {
        for (unsigned i = 0; i < 255; i++) {
            buf_[i] = 255 - i;
        }
    }

    template<typename T>
    void basicTest8() {
        std::shared_ptr<OptionInt<T> > opt;
        buf_[0] = 0xa1;
        ASSERT_NO_THROW(
            opt = std::shared_ptr<OptionInt<T> >(new OptionInt<T>(TEST_OPT_CODE,
                                                                    buf_.begin(),
                                                                    buf_.begin() + 1))
        );

        EXPECT_EQ(TEST_OPT_CODE, opt->getType());
        // Option should return the same value that we initialized the first
        // byte of the buffer with.
        EXPECT_EQ(static_cast<T>(0xa1), opt->getValue());

        // test for pack()
        opt->pack(out_buf_);

        // Data length is 1 byte.
        EXPECT_EQ(1, opt->len() - opt->getHeaderLen());
        EXPECT_EQ(TEST_OPT_CODE, opt->getType());
        // The total length is 1 byte for data and 2 bytes or 4 bytes
        // for option code and option length.
        EXPECT_EQ(3, out_buf_.getLength());

        // Check if pack worked properly:
        InputBuffer out(out_buf_.getData(), out_buf_.getLength());
        // if option type is correct
        EXPECT_EQ(TEST_OPT_CODE, out.readUint8());
        // if option length is correct
        EXPECT_EQ(1, out.readUint8());
        // if data is correct
        EXPECT_EQ(0xa1, out.readUint8() );
    }

    /// @brief Basic test for int16 and uint16 types.
    ///
    /// @note this function does not perform type check. Make
    /// sure that only int16_t or uint16_t type is used.
    ///
    /// @param u universe (V4 or V6)
    /// @tparam T int16_t or uint16_t.
    template<typename T>
    void basicTest16() {
        // Create option that conveys single 16-bit integer value.
        std::shared_ptr<OptionInt<T> > opt;
        // Initialize buffer with uint16_t value.
        buf_[0] = 0xa1;
        buf_[1] = 0xa2;
        // Constructor may throw in case provided buffer is too short.
        ASSERT_NO_THROW(
            opt = std::shared_ptr<OptionInt<T> >(new OptionInt<T>(TEST_OPT_CODE,
                                                                    buf_.begin(),
                                                                    buf_.begin() + 2))
        );

        EXPECT_EQ(TEST_OPT_CODE, opt->getType());
        // Option should return the value equal to the contents of first
        // and second byte of the buffer.
        EXPECT_EQ(static_cast<T>(0xa1a2), opt->getValue());

        // Test for pack()
        opt->pack(out_buf_);

        // Data length is 2 bytes.
        EXPECT_EQ(2, opt->len() - opt->getHeaderLen());
        EXPECT_EQ(TEST_OPT_CODE, opt->getType());
        // The total length is 2 bytes for data and 2 or 4 bytes for aheader.
        EXPECT_EQ(4, out_buf_.getLength());

        // Check if pack worked properly:
        InputBuffer out(out_buf_.getData(), out_buf_.getLength());
        // if option type is correct
        EXPECT_EQ(TEST_OPT_CODE, out.readUint8());
        // if option length is correct
        EXPECT_EQ(2, out.readUint8());
        // if data is correct
        EXPECT_EQ(0xa1a2, out.readUint16() );
    }

    /// @brief Basic test for int32 and uint32 types.
    ///
    /// @note this function does not perform type check. Make
    /// sure that only int32_t or uint32_t type is used.
    ///
    /// @param u universe (V4 or V6).
    /// @tparam T int32_t or uint32_t.
    template<typename T>
    void basicTest32() {
        // Create option that conveys single 32-bit integer value.
        std::shared_ptr<OptionInt<T> > opt;
        // Initialize buffer with 32-bit integer value.
        buf_[0] = 0xa1;
        buf_[1] = 0xa2;
        buf_[2] = 0xa3;
        buf_[3] = 0xa4;
        // Constructor may throw in case provided buffer is too short.
        ASSERT_NO_THROW(
            opt = std::shared_ptr<OptionInt<T> >(new OptionInt<T>(TEST_OPT_CODE,
                                                                    buf_.begin(),
                                                                    buf_.begin() + 4))
        );

        EXPECT_EQ(TEST_OPT_CODE, opt->getType());
        // Option should return the value equal to the value made of
        // first 4 bytes of the buffer.
        EXPECT_EQ(static_cast<T>(0xa1a2a3a4), opt->getValue());

        // Test for pack()
        opt->pack(out_buf_);

        // Data length is 4 bytes.
        EXPECT_EQ(4, opt->len() - opt->getHeaderLen());
        EXPECT_EQ(TEST_OPT_CODE, opt->getType());
        // The total length is 4 bytes for data and 2 or 4 bytes for a header.
        EXPECT_EQ(6, out_buf_.getLength());

        // Check if pack worked properly:
        InputBuffer out(out_buf_.getData(), out_buf_.getLength());
        // if option type is correct
        EXPECT_EQ(TEST_OPT_CODE, out.readUint8());
        // if option length is correct
        EXPECT_EQ(4, out.readUint8());
        // if data is correct
        EXPECT_EQ(0xa1a2a3a4, out.readUint32());
    }

    OptionBuffer buf_;     ///< Option buffer
    OutputBuffer out_buf_; ///< Output buffer
};

/// @todo: below, there is a bunch of tests for options that
/// convey unsigned value. We should maybe extend these tests for
/// signed types too.


TEST_F(OptionIntTest, basicUint8V4) {
    basicTest8<uint8_t>();
}


TEST_F(OptionIntTest, basicUint16V4) {
    basicTest16<uint16_t>();
}


TEST_F(OptionIntTest, basicUint32V4) {
    basicTest32<uint32_t>();
}


TEST_F(OptionIntTest, basicInt8V4) {
    basicTest8<int8_t>();
}


TEST_F(OptionIntTest, basicInt16V4) {
    basicTest16<int16_t>();
}


TEST_F(OptionIntTest, basicInt32V4) {
    basicTest32<int32_t>();
}


TEST_F(OptionIntTest, setValueUint8) {
    std::shared_ptr<OptionInt<uint8_t> > opt(new OptionInt<uint8_t>(OPT_UINT8_TYPE, 123));
    // Check if constructor initialized the option value correctly.
    EXPECT_EQ(123, opt->getValue());
    // Override the value.
    opt->setValue(111);

    EXPECT_EQ(OPT_UINT8_TYPE, opt->getType());
    // Check if the value has been overriden.
    EXPECT_EQ(111, opt->getValue());
}

TEST_F(OptionIntTest, setValueInt8) {
    std::shared_ptr<OptionInt<int8_t> > opt(new OptionInt<int8_t>(OPT_INT8_TYPE, -123));
    // Check if constructor initialized the option value correctly.
    EXPECT_EQ(-123, opt->getValue());
    // Override the value.
    opt->setValue(-111);

    EXPECT_EQ(OPT_INT8_TYPE, opt->getType());
    // Check if the value has been overriden.
    EXPECT_EQ(-111, opt->getValue());
}


TEST_F(OptionIntTest, setValueUint16) {
    std::shared_ptr<OptionInt<uint16_t> > opt(new OptionInt<uint16_t>(OPT_UINT16_TYPE, 123));
    // Check if constructor initialized the option value correctly.
    EXPECT_EQ(123, opt->getValue());
    // Override the value.
    opt->setValue(0x0102);

    EXPECT_EQ(OPT_UINT16_TYPE, opt->getType());
    // Check if the value has been overriden.
    EXPECT_EQ(0x0102, opt->getValue());
}

TEST_F(OptionIntTest, setValueInt16) {
    std::shared_ptr<OptionInt<int16_t> > opt(new OptionInt<int16_t>(OPT_INT16_TYPE, -16500));
    // Check if constructor initialized the option value correctly.
    EXPECT_EQ(-16500, opt->getValue());
    // Override the value.
    opt->setValue(-20100);

    EXPECT_EQ(OPT_INT16_TYPE, opt->getType());
    // Check if the value has been overriden.
    EXPECT_EQ(-20100, opt->getValue());
}

TEST_F(OptionIntTest, setValueUint32) {
    std::shared_ptr<OptionInt<uint32_t> > opt(new OptionInt<uint32_t>(OPT_UINT32_TYPE, 123));
    // Check if constructor initialized the option value correctly.
    EXPECT_EQ(123, opt->getValue());
    // Override the value.
    opt->setValue(0x01020304);

    EXPECT_EQ(OPT_UINT32_TYPE, opt->getType());
    // Check if the value has been overriden.
    EXPECT_EQ(0x01020304, opt->getValue());
}

TEST_F(OptionIntTest, setValueInt32) {
    std::shared_ptr<OptionInt<int32_t> > opt(new OptionInt<int32_t>(OPT_INT32_TYPE, -120100));
    // Check if constructor initialized the option value correctly.
    EXPECT_EQ(-120100, opt->getValue());
    // Override the value.
    opt->setValue(-125000);

    EXPECT_EQ(OPT_INT32_TYPE, opt->getType());
    // Check if the value has been overriden.
    EXPECT_EQ(-125000, opt->getValue());
}

TEST_F(OptionIntTest, packSuboptions4) {
    std::shared_ptr<OptionInt<uint16_t> > opt(new OptionInt<uint16_t>(TEST_OPT_CODE,
                                                                        0x0102));
    // Add sub option with some 4 bytes of data (each byte set to 1)
    std::unique_ptr<Option> sub1(new Option(TEST_OPT_CODE + 1, OptionBuffer(4, 1)));
    // Add sub option with some 5 bytes of data (each byte set to 2)
    std::unique_ptr<Option> sub2(new Option(TEST_OPT_CODE + 2, OptionBuffer(5, 2)));

    // Add suboptions.
    opt->addOption(std::move(sub1));
    opt->addOption(std::move(sub2));

    // Prepare reference data: option + suoptions in wire format.
    uint8_t expected[] = {
        TEST_OPT_CODE, 15, // option header
        0x01, 0x02,        // data, uint16_t value = 0x0102
        TEST_OPT_CODE + 1, 0x04, 0x01, 0x01, 0x01, 0x01, // sub1
        TEST_OPT_CODE + 2, 0x05, 0x02, 0x02, 0x02, 0x02, 0x02 // sub2
    };

    // Create on-wire format of option and suboptions.
    opt->pack(out_buf_);
    // Compare the on-wire data with the reference buffer.
    ASSERT_EQ(sizeof(expected), out_buf_.getLength());
    EXPECT_EQ(0, memcmp(out_buf_.getData(), expected, sizeof(expected)));
}


TEST_F(OptionIntTest, unpackSuboptions4) {
    // Prepare reference data.
    const uint8_t expected[] = {
        TEST_OPT_CODE, 0x0A, // option code and length
        0x01, 0x02, 0x03, 0x04, // data, uint32_t value = 0x01020304
        TEST_OPT_CODE + 1, 0x4, 0x01, 0x01, 0x01, 0x01 // suboption
    };
    // Make sure that the buffer size is sufficient to copy the
    // elements from the array.
    ASSERT_GE(buf_.size(), sizeof(expected));
    // Copy the data to a vector so as we can pass it to the
    // OptionInt's constructor.
    memcpy(&buf_[0], expected, sizeof(expected));

    // Create an option.
    std::shared_ptr<OptionInt<uint32_t> > opt;
    EXPECT_NO_THROW(
        opt = std::shared_ptr<
            OptionInt<uint32_t> >(new OptionInt<uint32_t>(TEST_OPT_CODE,
                                                          buf_.begin() + 2,
                                                          buf_.begin() + sizeof(expected)));
    );

    // Verify that it has expected type and data.
    EXPECT_EQ(TEST_OPT_CODE, opt->getType());
    EXPECT_EQ(0x01020304, opt->getValue());

    // Expect that there is the sub option with the particular
    // option code added.
    const Option* subopt = opt->getOption(TEST_OPT_CODE + 1);
    ASSERT_TRUE(subopt != nullptr);
    // Check that this option has correct universe and code.
    EXPECT_EQ(TEST_OPT_CODE + 1, subopt->getType());
    // Check the sub option's data.
    OptionBuffer subopt_buf = subopt->getData();
    ASSERT_EQ(4, subopt_buf.size());
    // The data in the input buffer starts at offset 8.
    EXPECT_TRUE(std::equal(subopt_buf.begin(), subopt_buf.end(), buf_.begin() + 8));
}


// This test checks that the toText function returns the option in the
// textual format correctly.
TEST_F(OptionIntTest, toText) {
    OptionUint32 option(128, 345678);
    EXPECT_EQ("type=128, len=004: 345678 (uint32)", option.toText());

    option.addOption(std::unique_ptr<Option>(new OptionUint16(1, 234)));
    option.addOption(std::unique_ptr<Option>(new OptionUint8(3, 22)));
    EXPECT_EQ("type=128, len=011: 345678 (uint32),\n"
              "options:\n"
              "  type=001, len=002: 234 (uint16)\n"
              "  type=003, len=001: 22 (uint8)",
              option.toText());
}

} // anonymous namespace
