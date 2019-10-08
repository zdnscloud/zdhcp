#include <kea/hooks/server_hooks.h>

#include <gtest/gtest.h>
#include <algorithm>


using namespace kea;
using namespace kea::hooks;
using namespace std;

namespace kea {

TEST(ServerHooksTest, RegisterHooks) {
    ServerHooks& hooks = ServerHooks::instance();
    hooks.reset();

    EXPECT_EQ(2, hooks.getCount());
    EXPECT_EQ(0, hooks.getIndex("context_create"));
    EXPECT_EQ(1, hooks.getIndex("context_destroy"));

    const int create_value = ServerHooks::CONTEXT_CREATE;
    const int destroy_value = ServerHooks::CONTEXT_DESTROY;
    EXPECT_EQ(0, create_value);
    EXPECT_EQ(1, destroy_value);

    int alpha = hooks.registerHook("alpha");
    EXPECT_EQ(2, alpha);
    EXPECT_EQ(2, hooks.getIndex("alpha"));

    int beta = hooks.registerHook("beta");
    EXPECT_EQ(3, beta);
    EXPECT_EQ(3, hooks.getIndex("beta"));

    EXPECT_EQ(4, hooks.getCount());
}

TEST(ServerHooksTest, DuplicateHooks) {
    ServerHooks& hooks = ServerHooks::instance();
    hooks.reset();

    EXPECT_THROW(hooks.registerHook("context_create"), DuplicateHook);

    int gamma = hooks.registerHook("gamma");
    EXPECT_EQ(2, gamma);
    EXPECT_THROW(hooks.registerHook("gamma"), DuplicateHook);
}

TEST(ServerHooksTest, GetHookNames) {
    ServerHooks& hooks = ServerHooks::instance();
    hooks.reset();
    vector<string> expected_names;

    expected_names.push_back("alpha");
    expected_names.push_back("beta");
    expected_names.push_back("gamma");
    expected_names.push_back("delta");
    for (size_t i = 0; i < expected_names.size(); ++i) {
        hooks.registerHook(expected_names[i].c_str());
    };

    // Update the expected names to include the pre-defined hook names.
    expected_names.push_back("context_create");
    expected_names.push_back("context_destroy");

    // Get the actual hook names
    vector<string> actual_names = hooks.getHookNames();

    // For comparison, sort the names into alphabetical order and do a straight
    // vector comparison.
    sort(expected_names.begin(), expected_names.end());
    sort(actual_names.begin(), actual_names.end());

    EXPECT_TRUE(expected_names == actual_names);
}

// Test the inverse hooks functionality (i.e. given an index, get the name).

TEST(ServerHooksTest, GetHookIndexes) {
    ServerHooks& hooks = ServerHooks::instance();
    hooks.reset();

    int alpha = hooks.registerHook("alpha");
    int beta = hooks.registerHook("beta");
    int gamma = hooks.registerHook("gamma");

    EXPECT_EQ(std::string("context_create"),
              hooks.getName(ServerHooks::CONTEXT_CREATE));
    EXPECT_EQ(std::string("context_destroy"),
              hooks.getName(ServerHooks::CONTEXT_DESTROY));
    EXPECT_EQ(std::string("alpha"), hooks.getName(alpha));
    EXPECT_EQ(std::string("beta"), hooks.getName(beta));
    EXPECT_EQ(std::string("gamma"), hooks.getName(gamma));

    // Check for an invalid index
    EXPECT_THROW(hooks.getName(-1), NoSuchHook);
    EXPECT_THROW(hooks.getName(42), NoSuchHook);
}

// Test the reset functionality.

TEST(ServerHooksTest, Reset) {
    ServerHooks& hooks = ServerHooks::instance();
    hooks.reset();

    int alpha = hooks.registerHook("alpha");
    int beta = hooks.registerHook("beta");
    int gamma = hooks.registerHook("gamma");

    EXPECT_EQ(std::string("alpha"), hooks.getName(alpha));
    EXPECT_EQ(std::string("beta"), hooks.getName(beta));
    EXPECT_EQ(std::string("gamma"), hooks.getName(gamma));

    // Check the counts before and after a reset.
    EXPECT_EQ(5, hooks.getCount());
    hooks.reset();
    EXPECT_EQ(2, hooks.getCount());

    // ... and check that the hooks are as expected.
    EXPECT_EQ(0, hooks.getIndex("context_create"));
    EXPECT_EQ(1, hooks.getIndex("context_destroy"));
}

// Check that getting an unknown name throws an exception.

TEST(ServerHooksTest, UnknownHookName) {
    ServerHooks& hooks = ServerHooks::instance();
    hooks.reset();

    EXPECT_THROW(static_cast<void>(hooks.getIndex("unknown")), NoSuchHook);
}

// Check that the count of hooks is correct.

TEST(ServerHooksTest, HookCount) {
    ServerHooks& hooks = ServerHooks::instance();
    hooks.reset();

    hooks.registerHook("alpha");
    hooks.registerHook("beta");
    hooks.registerHook("gamma");
    hooks.registerHook("delta");

    EXPECT_EQ(6, hooks.getCount());
}

}; 
