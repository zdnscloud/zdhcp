#include <kea/util/staged_value.h>
#include <gtest/gtest.h>

namespace kea{

using namespace kea::util;

TEST(StagedValueTest, assignAndCommit) {
    // Initally the value should be set to a default
    StagedValue<int> value;
    value.setValue(std::unique_ptr<int>(new int(4)));
    ASSERT_EQ(4, value.getValue());

    value.commit();
    ASSERT_EQ(4, value.getValue());

    value.setValue(std::unique_ptr<int>(new int(10)));
    ASSERT_EQ(10, value.getValue());

    value.setValue(std::unique_ptr<int>(new int(20)));
    ASSERT_EQ(20, value.getValue());
    value.commit();
    EXPECT_EQ(20, value.getValue());
}

TEST(StagedValueTest, revert) {
    StagedValue<int> value;
    value.setValue(std::unique_ptr<int>(new int(123)));
    value.commit();

    value.setValue(std::unique_ptr<int>(new int(500)));
    ASSERT_EQ(500, value.getValue());

    value.revert();
    EXPECT_EQ(123, value.getValue());

    value.revert();
    EXPECT_EQ(123, value.getValue());
}

TEST(StagedValueTest, reset) {
    StagedValue<int> value;
    value.setValue(std::unique_ptr<int>(new int(123)));
    value.commit();

    value.setValue(std::unique_ptr<int>(new int(500)));

    value.reset();
    ASSERT_THROW(value.getValue(), Unexpected);
    value.revert();
    ASSERT_THROW(value.getValue(), Unexpected);
}

TEST(StagedValueTest, commit) {
    StagedValue<int> value;
    value.setValue(std::unique_ptr<int>(new int(123)));
    value.commit();

    value.commit();
    EXPECT_EQ(123, value.getValue());
}

TEST(StagedValueTest, conversionOperator) {
    StagedValue<int> value;
    value.setValue(std::unique_ptr<int>(new int(244)));
    EXPECT_EQ(244, value);
}

// This test checks that the assignment operator works correctly.
TEST(StagedValueTest, assignmentOperator) {
    StagedValue<int> value;
    value = std::unique_ptr<int>(new int(111));
    EXPECT_EQ(111, value);
}


} // end of anonymous namespace
