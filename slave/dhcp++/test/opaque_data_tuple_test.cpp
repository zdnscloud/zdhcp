#include <kea/dhcp++/opaque_data_tuple.h>
#include <kea/util/buffer.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <sstream>
#include <vector>

using namespace kea;
using namespace kea::dhcp;
using namespace kea::util;

namespace kea {

// This test checks that when the default constructor is called, the data buffer
// is empty.
TEST(OpaqueDataTuple, constructor) {
    OpaqueDataTuple tuple;
    // There should be no data in the tuple.
    EXPECT_EQ(0, tuple.getLength());
    EXPECT_TRUE(tuple.getData().empty());
    EXPECT_TRUE(tuple.getText().empty());
}

// Test that the constructor which takes the buffer as argument parses the
// wire data.
TEST(OpaqueDataTuple, constructorParse1Byte) {
    const char wire_data[] = {
        0x0B,                               // Length is 11
        0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, // Hello<space>
        0x77, 0x6F, 0x72, 0x6C, 0x64        // world
    };

    OpaqueDataTuple tuple(wire_data, wire_data + sizeof(wire_data));

    EXPECT_EQ(11, tuple.getLength());
    EXPECT_EQ("Hello world", tuple.getText());

}


// This test checks that it is possible to set the tuple data using raw buffer.
TEST(OpaqueDataTuple, assignData) {
    OpaqueDataTuple tuple;
    // Initially the tuple buffer should be empty.
    OpaqueDataTuple::Buffer buf = tuple.getData();
    ASSERT_TRUE(buf.empty());
    // Prepare some input data and assign to the tuple.
    const uint8_t data1[] = {
        0xCA, 0xFE, 0xBE, 0xEF
    };
    tuple.assign(data1, sizeof(data1));
    // Tuple should now hold the data we assigned.
    ASSERT_EQ(sizeof(data1), tuple.getLength());
    buf = tuple.getData();
    EXPECT_TRUE(std::equal(buf.begin(), buf.end(), data1));

    // Prepare the other set of data and assign to the tuple.
    const uint8_t data2[] = {
        1, 2, 3, 4, 5, 6
    };
    tuple.assign(data2, sizeof(data2));
    // The new data should have replaced the old data.
    ASSERT_EQ(sizeof(data2), tuple.getLength());
    buf = tuple.getData();
    EXPECT_TRUE(std::equal(buf.begin(), buf.end(), data2));
}

// This test checks that it is possible to append the data to the tuple using
// raw buffer.
TEST(OpaqueDataTuple, appendData) {
    OpaqueDataTuple tuple;
    // Initially the tuple buffer should be empty.
    OpaqueDataTuple::Buffer buf = tuple.getData();
    ASSERT_TRUE(buf.empty());
    // Prepare some input data and append to the empty tuple.
    const uint8_t data1[] = {
        0xCA, 0xFE, 0xBE, 0xEF
    };
    tuple.append(data1, sizeof(data1));
    // The tuple should now hold only the data we appended.
    ASSERT_EQ(sizeof(data1), tuple.getLength());
    buf = tuple.getData();
    EXPECT_TRUE(std::equal(buf.begin(), buf.end(), data1));
    // Prepare the new set of data and append.
    const uint8_t data2[] = {
        1, 2, 3, 4, 5, 6
    };
    tuple.append(data2, sizeof(data2));
    // We expect that the tuple now has both sets of data we appended. In order
    // to verify that, we have to concatenate the input data1 and data2.
    std::vector<uint8_t> data12(data1, data1 + sizeof(data1));
    data12.insert(data12.end(), data2, data2 + sizeof(data2));
    // The size of the tuple should be a sum of data1 and data2 lengths.
    ASSERT_EQ(sizeof(data1) + sizeof(data2), tuple.getLength());
    buf = tuple.getData();
    EXPECT_TRUE(std::equal(buf.begin(), buf.end(), data12.begin()));
}

// This test checks that it is possible to assign the string to the tuple.
TEST(OpaqueDataTuple, assignString) {
    OpaqueDataTuple tuple;
    // Initially, the tuple should be empty.
    ASSERT_EQ(0, tuple.getLength());
    // Assign some string data.
    tuple.assign("Some string");
    // Verify that the data has been assigned.
    EXPECT_EQ(11, tuple.getLength());
    EXPECT_EQ("Some string", tuple.getText());
    // Assign some other string.
    tuple.assign("Different string");
    // The new string should have replaced the old string.
    EXPECT_EQ(16, tuple.getLength());
    EXPECT_EQ("Different string", tuple.getText());
}

// This test checks that it is possible to append the string to the tuple.
TEST(OpaqueDataTuple, appendString) {
    OpaqueDataTuple tuple;
    // Initially the tuple should be empty.
    ASSERT_EQ(0, tuple.getLength());
    // Append the string to it.
    tuple.append("First part");
    ASSERT_EQ(10, tuple.getLength());
    ASSERT_EQ("First part", tuple.getText());
    // Now append the other string.
    tuple.append(" and second part");
    EXPECT_EQ(26, tuple.getLength());
    // The resulting data in the tuple should be a concatenation of both
    // strings.
    EXPECT_EQ("First part and second part", tuple.getText());
}

// This test verifies that equals function correctly checks that the tuple
// holds a given string but it doesn't hold the other. This test also
// checks the assignment operator for the tuple.
TEST(OpaqueDataTuple, equals) {
    OpaqueDataTuple tuple;
    // Tuple is supposed to be empty so it is not equal xyz.
    EXPECT_FALSE(tuple.equals("xyz"));
    // Assign xyz.
    EXPECT_NO_THROW(tuple = "xyz");
    // The tuple should be equal xyz, but not abc.
    EXPECT_FALSE(tuple.equals("abc"));
    EXPECT_TRUE(tuple.equals("xyz"));
    // Assign abc to the tuple.
    EXPECT_NO_THROW(tuple = "abc");
    // It should be now opposite.
    EXPECT_TRUE(tuple.equals("abc"));
    EXPECT_FALSE(tuple.equals("xyz"));
}

// This test checks that the conversion of the data in the tuple to the string
// is performed correctly.
TEST(OpaqueDataTuple, getText) {
    OpaqueDataTuple tuple;
    // Initially the tuple should be empty.
    ASSERT_TRUE(tuple.getText().empty());
    // ASCII representation of 'Hello world'.
    const char as_ascii[] = {
        0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, // Hello<space>
        0x77, 0x6F, 0x72, 0x6C, 0x64        // world
    };
    // Assign it to the tuple.
    tuple.assign(as_ascii, sizeof(as_ascii));
    // Conversion to string should give as the original text.
    EXPECT_EQ("Hello world", tuple.getText());
}

// This test verifies the behavior of (in)equality and assignment operators.
TEST(OpaqueDataTuple, operators) {
    OpaqueDataTuple tuple;
    // Tuple should be empty initially.
    ASSERT_EQ(0, tuple.getLength());
    // Check assignment.
    EXPECT_NO_THROW(tuple = "Hello World");
    EXPECT_EQ(11, tuple.getLength());
    EXPECT_TRUE(tuple == "Hello World");
    EXPECT_TRUE(tuple != "Something else");
    // Assign something else to make sure it affects the tuple.
    EXPECT_NO_THROW(tuple = "Something else");
    EXPECT_EQ(14, tuple.getLength());
    EXPECT_TRUE(tuple == "Something else");
    EXPECT_TRUE(tuple != "Hello World");
}

// This test verifies that the tuple is inserted in the textual format to the
// output stream.
TEST(OpaqueDataTuple, operatorOutputStream) {
    OpaqueDataTuple tuple;
    // The tuple should be empty initially.
    ASSERT_EQ(0, tuple.getLength());
    // The tuple is empty, so assigning its content to the output stream should
    // be no-op and result in the same text in the stream.
    std::ostringstream s;
    s << "Some text";
    EXPECT_NO_THROW(s << tuple);
    EXPECT_EQ("Some text", s.str());
    // Now, let's assign some text to the tuple and call operator again.
    // The new text should be added to the stream.
    EXPECT_NO_THROW(tuple = " and some other text");
    EXPECT_NO_THROW(s << tuple);
    EXPECT_EQ(s.str(), "Some text and some other text");

}

// This test verifies that the value of the tuple can be initialized from the
// input stream.
TEST(OpaqueDataTuple, operatorInputStream) {
    OpaqueDataTuple tuple;
    // The tuple should be empty initially.
    ASSERT_EQ(0, tuple.getLength());
    // The input stream has some text. This text should be appended to the
    // tuple.
    std::istringstream s;
    s.str("Some text");
    EXPECT_NO_THROW(s >> tuple);
    EXPECT_EQ("Some text", tuple.getText());
    // Now, let's assign some other text to the stream. This new text should be
    // assigned to the tuple.
    s.str("And some other");
    EXPECT_NO_THROW(s >> tuple);
    EXPECT_EQ("And some other", tuple.getText());
}

// This test checks that the tuple is correctly encoded in the wire format when
// the size of the length field is 1 byte.
TEST(OpaqueDataTuple, pack1Byte) {
    OpaqueDataTuple tuple;
    // Initially, the tuple should be empty.
    ASSERT_EQ(0, tuple.getLength());
    // The empty data doesn't make much sense, so the pack() should not
    // allow it.
    OutputBuffer out_buf(10);
    EXPECT_THROW(tuple.pack(out_buf), OpaqueDataTupleError);
    // Set the data for tuple.
    std::vector<uint8_t> data;
    for (uint8_t i = 0; i < 100; ++i) {
        data.push_back(i);
    }
    tuple.assign(data.begin(), data.size());
    // The pack should now succeed.
    ASSERT_NO_THROW(tuple.pack(out_buf));
    // The rendered buffer should be 101 bytes long - 1 byte for length,
    // 100 bytes for the actual data.
    ASSERT_EQ(101, out_buf.getLength());
    // Get the rendered data into the vector for convenience.
    std::vector<uint8_t>
        render_data(static_cast<const uint8_t*>(out_buf.getData()),
                    static_cast<const uint8_t*>(out_buf.getData()) + 101);
    // The first byte is a length byte. It should hold the length of 100.
    EXPECT_EQ(100, render_data[0]);
    // Verify that the rendered data is correct.
    EXPECT_TRUE(std::equal(render_data.begin() + 1, render_data.end(),
                           data.begin()));
    // Reset the output buffer for another test.
    out_buf.clear();
    // Fill in the tuple buffer so as it reaches maximum allowed length. The
    // maximum length is 255 when the size of the length field is one byte.
    for (uint8_t i = 100; i < 255; ++i) {
        data.push_back(i);
    }
    ASSERT_EQ(255, data.size());
    tuple.assign(data.begin(), data.size());
    // The pack() should be successful again.
    ASSERT_NO_THROW(tuple.pack(out_buf));
    // The rendered buffer should be 256 bytes long. The first byte holds the
    // opaque data length, the remaining bytes hold the actual data.
    ASSERT_EQ(256, out_buf.getLength());
    // Check that the data is correct.
    render_data.assign(static_cast<const uint8_t*>(out_buf.getData()),
                       static_cast<const uint8_t*>(out_buf.getData()) + 256);
    EXPECT_EQ(255, render_data[0]);
    EXPECT_TRUE(std::equal(render_data.begin() + 1, render_data.end(),
                           data.begin()));
    // Clear output buffer for another test.
    out_buf.clear();
    // Add one more value to the tuple. Now, the resulting buffer should exceed
    // the maximum length. An attempt to pack() should fail.
    data.push_back(255);
    tuple.assign(data.begin(), data.size());
    EXPECT_THROW(tuple.pack(out_buf), OpaqueDataTupleError);
}

// This test verifies that the tuple is decoded from the wire format.
TEST(OpaqueDataTuple, unpack1Byte) {
    OpaqueDataTuple tuple;
    const char wire_data[] = {
        0x0B,                               // Length is 11
        0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, // Hello<space>
        0x77, 0x6F, 0x72, 0x6C, 0x64        // world
    };

    ASSERT_NO_THROW(tuple.unpack(wire_data, wire_data + sizeof(wire_data)));
    EXPECT_EQ(11, tuple.getLength());
    EXPECT_EQ("Hello world", tuple.getText());
}

// This test verifies that the tuple having a length of 0, is decoded from
// the wire format.
TEST(OpaqueDataTuple, unpack1ByteZeroLength) {
    OpaqueDataTuple tuple;
    EXPECT_NO_THROW(tuple = "Hello world");
    ASSERT_NE(tuple.getLength(), 0);

    const char wire_data[] = {
        0
    };
    ASSERT_NO_THROW(tuple.unpack(wire_data, wire_data + sizeof(wire_data)));

    EXPECT_EQ(0, tuple.getLength());
}

// This test verfifies that exception is thrown if the empty buffer is being
// parsed.
TEST(OpaqueDataTuple, unpack1ByteEmptyBuffer) {
    OpaqueDataTuple tuple;
    const char wire_data[] = {
        1, 2, 3
    };
    EXPECT_THROW(tuple.unpack(wire_data, wire_data), OpaqueDataTupleError);
}

// This test verifies that exception is thrown when parsing truncated buffer.
TEST(OpaqueDataTuple, unpack1ByteTruncatedBuffer) {
   OpaqueDataTuple tuple;
    const char wire_data[] = {
        10, 2, 3
    };
    EXPECT_THROW(tuple.unpack(wire_data, wire_data + sizeof(wire_data)),
                 OpaqueDataTupleError);
}

} // anonymous namespace
