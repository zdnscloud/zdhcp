#include <kea/hooks/callout_handle.h>
#include <kea/hooks/callout_manager.h>
#include <kea/hooks/server_hooks.h>

#include <gtest/gtest.h>
#include <memory>

using namespace kea::hooks;
using namespace std;

namespace kea {

class CalloutHandleTest : public ::testing::Test {
public:

    /// @brief Constructor
    ///
    /// Sets up a callout manager to be referenced by the CalloutHandle in
    /// these tests. (The "4" for the number of libraries in the
    /// CalloutManager is arbitrary - it is not used in these tests.)
    CalloutHandleTest() : manager_(new CalloutManager())
    {}

    CalloutManager& getCalloutManager() {
        return (*manager_);
    }

private:
    std::unique_ptr<CalloutManager> manager_;
};

TEST_F(CalloutHandleTest, ArgumentDistinctSimpleType) {
    CalloutHandle handle(getCalloutManager());

    int a = 42;
    handle.setArgument("integer1", a);
    EXPECT_EQ(42, a);

    int b = 0;
    handle.getArgument("integer1", b);
    EXPECT_EQ(42, b);

    // Add another integer (another random value).
    int c = 142;
    handle.setArgument("integer2", c);
    EXPECT_EQ(142, c);

    int d = 0;
    handle.getArgument("integer2", d);
    EXPECT_EQ(142, d);

    // Add a short (random value).
    short e = -81;
    handle.setArgument("short", e);
    EXPECT_EQ(-81, e);

    short f = 0;
    handle.getArgument("short", f);
    EXPECT_EQ(-81, f);
}

TEST_F(CalloutHandleTest, ArgumentUnknownName) {
    CalloutHandle handle(getCalloutManager());

    // Set an integer
    int a = 42;
    handle.setArgument("integer1", a);
    EXPECT_EQ(42, a);

    // Check we can retrieve it
    int b = 0;
    handle.getArgument("integer1", b);
    EXPECT_EQ(42, b);

    // Check that getting an unknown name throws an exception.
    int c = 0;
    EXPECT_THROW(handle.getArgument("unknown", c), NoSuchArgument);
}

TEST_F(CalloutHandleTest, ArgumentIncorrectType) {
    CalloutHandle handle(getCalloutManager());

    int a = 42;
    handle.setArgument("integer1", a);
    EXPECT_EQ(42, a);

    // Check we can retrieve it
    long b = 0;
    EXPECT_THROW(handle.getArgument("integer1", b), boost::bad_any_cast);
}

struct Alpha {
    int a;
    int b;
    Alpha(int first = 0, int second = 0) : a(first), b(second) {}
};

struct Beta {
    int c;
    int d;
    Beta(int first = 0, int second = 0) : c(first), d(second) {}
};

TEST_F(CalloutHandleTest, ComplexTypes) {
    CalloutHandle handle(getCalloutManager());

    Alpha aleph(1, 2);
    EXPECT_EQ(1, aleph.a);
    EXPECT_EQ(2, aleph.b);
    handle.setArgument("aleph", aleph);

    Beta beth(11, 22);
    EXPECT_EQ(11, beth.c);
    EXPECT_EQ(22, beth.d);
    handle.setArgument("beth", beth);

    Alpha aleph2;
    EXPECT_EQ(0, aleph2.a);
    EXPECT_EQ(0, aleph2.b);
    handle.getArgument("aleph", aleph2);
    EXPECT_EQ(1, aleph2.a);
    EXPECT_EQ(2, aleph2.b);

    Beta beth2;
    EXPECT_EQ(0, beth2.c);
    EXPECT_EQ(0, beth2.d);
    handle.getArgument("beth", beth2);
    EXPECT_EQ(11, beth2.c);
    EXPECT_EQ(22, beth2.d);

    EXPECT_THROW(handle.getArgument("aleph", beth), boost::bad_any_cast);
}

TEST_F(CalloutHandleTest, PointerTypes) {
    CalloutHandle handle(getCalloutManager());

    Alpha aleph(5, 10);
    const Beta beth(15, 20);

    Alpha* pa = &aleph;
    const Beta* pcb = &beth;

    handle.setArgument("non_const_pointer", pa);
    handle.setArgument("const_pointer", pcb);

    Alpha* pa2 = 0;
    handle.getArgument("non_const_pointer", pa2);
    EXPECT_TRUE(pa == pa2);

    const Beta* pcb2 = 0;
    handle.getArgument("const_pointer", pcb2);
    EXPECT_TRUE(pcb == pcb2);

    const Alpha* pca3;
    EXPECT_THROW(handle.getArgument("non_const_pointer", pca3),
                 boost::bad_any_cast);

    Beta* pb3;
    EXPECT_THROW(handle.getArgument("const_pointer", pb3),
                 boost::bad_any_cast);
}

TEST_F(CalloutHandleTest, ContextItemNames) {
    CalloutHandle handle(getCalloutManager());

    vector<string> expected_names;

    expected_names.push_back("faith");
    handle.setArgument("faith", 42);
    expected_names.push_back("hope");
    handle.setArgument("hope", 43);
    expected_names.push_back("charity");
    handle.setArgument("charity", 44);

    vector<string> actual_names = handle.getArgumentNames();

    sort(actual_names.begin(), actual_names.end());
    sort(expected_names.begin(), expected_names.end());
    EXPECT_TRUE(expected_names == actual_names);
}

TEST_F(CalloutHandleTest, DeleteArgument) {
    CalloutHandle handle(getCalloutManager());

    int one = 1;
    int two = 2;
    int three = 3;
    int four = 4;
    int value;      // Return value

    handle.setArgument("one", one);
    handle.setArgument("two", two);
    handle.setArgument("three", three);
    handle.setArgument("four", four);

    // Delete "one".
    handle.getArgument("one", value);
    EXPECT_EQ(1, value);
    handle.deleteArgument("one");

    EXPECT_THROW(handle.getArgument("one", value), NoSuchArgument);
    handle.getArgument("two", value);
    EXPECT_EQ(2, value);
    handle.getArgument("three", value);
    EXPECT_EQ(3, value);
    handle.getArgument("four", value);
    EXPECT_EQ(4, value);

    // Delete "three".
    handle.getArgument("three", value);
    EXPECT_EQ(3, value);
    handle.deleteArgument("three");

    EXPECT_THROW(handle.getArgument("one", value), NoSuchArgument);
    handle.getArgument("two", value);
    EXPECT_EQ(2, value);
    EXPECT_THROW(handle.getArgument("three", value), NoSuchArgument);
    handle.getArgument("four", value);
    EXPECT_EQ(4, value);
}

TEST_F(CalloutHandleTest, DeleteAllArguments) {
    CalloutHandle handle(getCalloutManager());

    int one = 1;
    int two = 2;
    int three = 3;
    int four = 4;
    int value;      // Return value

    handle.setArgument("one", one);
    handle.setArgument("two", two);
    handle.setArgument("three", three);
    handle.setArgument("four", four);

    handle.deleteAllArguments();

    EXPECT_THROW(handle.getArgument("one", value), NoSuchArgument);
    EXPECT_THROW(handle.getArgument("two", value), NoSuchArgument);
    EXPECT_THROW(handle.getArgument("three", value), NoSuchArgument);
    EXPECT_THROW(handle.getArgument("four", value), NoSuchArgument);
}

TEST_F(CalloutHandleTest, StatusField) {
    CalloutHandle handle(getCalloutManager());

    EXPECT_EQ(CalloutHandle::NEXT_STEP_CONTINUE, handle.getStatus());

    handle.setStatus(CalloutHandle::NEXT_STEP_SKIP);
    EXPECT_EQ(CalloutHandle::NEXT_STEP_SKIP, handle.getStatus());

    handle.setStatus(CalloutHandle::NEXT_STEP_DROP);
    EXPECT_EQ(CalloutHandle::NEXT_STEP_DROP, handle.getStatus());

    handle.setStatus(CalloutHandle::NEXT_STEP_CONTINUE);
    EXPECT_EQ(CalloutHandle::NEXT_STEP_CONTINUE, handle.getStatus());
}
}; 
