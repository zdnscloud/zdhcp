#include <kea/hooks/callout_handle.h>
#include <kea/hooks/callout_manager.h>
#include <kea/hooks/library_manager.h>
#include <kea/hooks/server_hooks.h>
#include <kea/configure/json_conf.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <memory>


using namespace kea;
using namespace kea::hooks;
using namespace kea::configure;
using namespace std;

namespace kea {

const char* BASIC_CALLOUT_LIBRARY = "./lib/libbasic_callout_library.so";
const char* FULL_CALLOUT_LIBRARY = "./lib/libfull_callout_library.so";
const char* FRAMEWORK_EXCEPTION_LIBRARY = "./lib/libframework_exception_library.so";
const char* INCORRECT_VERSION_LIBRARY = "./lib/libincorrect_version_library.so";
const char* LOAD_CALLOUT_LIBRARY = "./lib/libload_callout_library.so";
const char* LOAD_ERROR_CALLOUT_LIBRARY = "./lib/libload_error_callout_library.so";
const char* NOT_PRESENT_LIBRARY = "./lib/libnothere.so";
const char* NO_VERSION_LIBRARY = "./lib/libno_version_library.so";
const char* CALLOUT_PARAMS_LIBRARY = "./lib/libcallout_params_library.so";

class LibraryManagerTest : public ::testing::Test {
public:
    LibraryManagerTest() {
        ServerHooks& hooks = ServerHooks::instance();
        hooks.reset();
        hookpt_one_index_ = hooks.registerHook("hookpt_one");
        hookpt_two_index_ = hooks.registerHook("hookpt_two");
        hookpt_three_index_ = hooks.registerHook("hookpt_three");
        callout_manager_.reset(new CalloutManager());
    }

    ~LibraryManagerTest() {
    }

    void executeCallCallouts(int r0, int d1, int r1, int d2, int r2, int d3,
            int r3) {
        static const char* COMMON_TEXT = " callout returned the wrong value";
        static const char* RESULT = "result";

        int result;

        CalloutHandle handle(*callout_manager_);
        handle.setArgument(RESULT, -1);

        callout_manager_->callCallouts(ServerHooks::CONTEXT_CREATE, handle);
        handle.getArgument(RESULT, result);
        EXPECT_EQ(r0, result) << "context_create" << COMMON_TEXT;

        handle.setArgument("data_1", d1);
        callout_manager_->callCallouts(hookpt_one_index_, handle);
        handle.getArgument(RESULT, result);
        EXPECT_EQ(r1, result) << "hookpt_one" << COMMON_TEXT;

        handle.setArgument("data_2", d2);
        callout_manager_->callCallouts(hookpt_two_index_, handle);
        handle.getArgument(RESULT, result);
        EXPECT_EQ(r2, result) << "hookpt_two" << COMMON_TEXT;

        handle.setArgument("data_3", d3);
        callout_manager_->callCallouts(hookpt_three_index_, handle);
        handle.getArgument(RESULT, result);
        EXPECT_EQ(r3, result) << "hookpt_three" << COMMON_TEXT;
    }

    std::unique_ptr<CalloutManager> callout_manager_;
    int hookpt_one_index_;
    int hookpt_two_index_;
    int hookpt_three_index_;
};

class PublicLibraryManager : public kea::hooks::LibraryManager {
public:
    PublicLibraryManager(const std::string& name, int index, CalloutManager& manager)
        : LibraryManager(name, index, manager)
    {}

    /// Public methods that call protected methods on the superclass.
    using LibraryManager::registerStandardCallouts;
    using LibraryManager::runLoad;
    using LibraryManager::runUnload;
};


TEST_F(LibraryManagerTest, NoLibrary) {
    EXPECT_FALSE(LibraryManager::openLibrary(std::string(NOT_PRESENT_LIBRARY)));
}

TEST_F(LibraryManagerTest, OpenClose) {
    void* handle = LibraryManager::openLibrary(std::string(BASIC_CALLOUT_LIBRARY));
    EXPECT_TRUE(handle != nullptr);
    EXPECT_TRUE(LibraryManager::closeLibrary(handle));
}


TEST_F(LibraryManagerTest, NoVersion) {
    void* handle = LibraryManager::openLibrary(std::string(NO_VERSION_LIBRARY));
    EXPECT_TRUE(handle != nullptr);
    EXPECT_FALSE(LibraryManager::checkVersion(handle));
    EXPECT_TRUE(LibraryManager::closeLibrary(handle));

    handle = LibraryManager::openLibrary(std::string(INCORRECT_VERSION_LIBRARY));
    EXPECT_TRUE(handle != nullptr);
    EXPECT_FALSE(LibraryManager::checkVersion(handle));
    EXPECT_TRUE(LibraryManager::closeLibrary(handle));

    handle = LibraryManager::openLibrary(std::string(FRAMEWORK_EXCEPTION_LIBRARY));
    EXPECT_TRUE(handle != nullptr);
    EXPECT_FALSE(LibraryManager::checkVersion(handle));
    EXPECT_TRUE(LibraryManager::closeLibrary(handle));

    handle = LibraryManager::openLibrary(std::string(BASIC_CALLOUT_LIBRARY));
    EXPECT_TRUE(handle != nullptr);
    EXPECT_TRUE(LibraryManager::checkVersion(handle));
    EXPECT_TRUE(LibraryManager::closeLibrary(handle));

}

TEST_F(LibraryManagerTest, RegisterStandardCallouts) {
    PublicLibraryManager lib_manager(std::string(BASIC_CALLOUT_LIBRARY), 0, *callout_manager_);
    EXPECT_TRUE(lib_manager.loadLibrary(JsonObject(nullptr)));
    executeCallCallouts(10, 5, 15, 7, 105, 17, 88);
    EXPECT_TRUE(lib_manager.unloadLibrary());
}

// Test that the "load" function is called correctly.

TEST_F(LibraryManagerTest, CheckLoadCalled) {
    PublicLibraryManager lib_manager(std::string(LOAD_CALLOUT_LIBRARY), 0, *callout_manager_);
    EXPECT_TRUE(lib_manager.loadLibrary(JsonObject(nullptr)));
    EXPECT_TRUE(callout_manager_->calloutsPresent(ServerHooks::CONTEXT_CREATE));
    EXPECT_TRUE(callout_manager_->calloutsPresent(hookpt_one_index_));
    EXPECT_TRUE(callout_manager_->calloutsPresent(hookpt_two_index_));
    EXPECT_TRUE(callout_manager_->calloutsPresent(hookpt_three_index_));
    EXPECT_FALSE(callout_manager_->calloutsPresent(ServerHooks::CONTEXT_DESTROY));

    executeCallCallouts(5, 5, 25, 7, 32, 10, 320);
    EXPECT_TRUE(lib_manager.unloadLibrary());
}

TEST_F(LibraryManagerTest, CheckLoadException) {
    PublicLibraryManager lib_manager(std::string(FRAMEWORK_EXCEPTION_LIBRARY), 0, *callout_manager_);
    EXPECT_FALSE(lib_manager.loadLibrary(JsonObject(nullptr)));
    EXPECT_TRUE(lib_manager.unloadLibrary());
}

TEST_F(LibraryManagerTest, CheckLoadError) {
    PublicLibraryManager lib_manager(std::string(LOAD_ERROR_CALLOUT_LIBRARY), 0, *callout_manager_);
    EXPECT_FALSE(lib_manager.loadLibrary(JsonObject(nullptr)));
    EXPECT_FALSE(lib_manager.unloadLibrary());
}


TEST_F(LibraryManagerTest, LibUnload) {
    PublicLibraryManager lib_manager(std::string(FULL_CALLOUT_LIBRARY), 0, *callout_manager_);
    EXPECT_TRUE(lib_manager.loadLibrary(JsonObject(nullptr)));
    EXPECT_TRUE(callout_manager_->calloutsPresent(hookpt_one_index_));
    EXPECT_TRUE(callout_manager_->calloutsPresent(hookpt_two_index_));
    EXPECT_TRUE(callout_manager_->calloutsPresent(hookpt_three_index_));

    lib_manager.unloadLibrary();
    EXPECT_FALSE(callout_manager_->calloutsPresent(hookpt_one_index_));
    EXPECT_FALSE(callout_manager_->calloutsPresent(hookpt_two_index_));
    EXPECT_FALSE(callout_manager_->calloutsPresent(hookpt_three_index_));
}

TEST_F(LibraryManagerTest, LoadMultipleLibraries) {
    LibraryManager lib_manager_1(std::string(FULL_CALLOUT_LIBRARY), 0, *callout_manager_);
    EXPECT_TRUE(lib_manager_1.loadLibrary(JsonObject(nullptr)));

    LibraryManager lib_manager_2(std::string(NO_VERSION_LIBRARY), 1, *callout_manager_);
    EXPECT_FALSE(lib_manager_2.loadLibrary(JsonObject(nullptr)));

    LibraryManager lib_manager_3(std::string(INCORRECT_VERSION_LIBRARY), 1, *callout_manager_);
    EXPECT_FALSE(lib_manager_3.loadLibrary(JsonObject(nullptr)));

    LibraryManager lib_manager_4(std::string(BASIC_CALLOUT_LIBRARY), 1, *callout_manager_);
    EXPECT_TRUE(lib_manager_4.loadLibrary(JsonObject(nullptr)));

    executeCallCallouts(10, 3, 33, 2, 62, 3, 183);

    EXPECT_TRUE(lib_manager_1.unloadLibrary());
    executeCallCallouts(10, 5, 15, 7, 105, 17, 88);
    EXPECT_TRUE(lib_manager_4.unloadLibrary());
}

TEST_F(LibraryManagerTest, LoadWithParameter) {
    LibraryManager lib_manager(std::string(CALLOUT_PARAMS_LIBRARY), 0, *callout_manager_);
    std::string lib_conf = R"({"svalue":"string value", "ivalue":42, "bvalue":true})";
    EXPECT_TRUE(lib_manager.loadLibrary(JsonConf::parseString(lib_conf)->root()));
    EXPECT_TRUE(lib_manager.unloadLibrary());
}


TEST_F(LibraryManagerTest, validateLibraries) {
    EXPECT_TRUE(LibraryManager::validateLibrary(BASIC_CALLOUT_LIBRARY));
    EXPECT_TRUE(LibraryManager::validateLibrary(FULL_CALLOUT_LIBRARY));
    EXPECT_FALSE(LibraryManager::validateLibrary(FRAMEWORK_EXCEPTION_LIBRARY));
    EXPECT_FALSE(LibraryManager::validateLibrary(INCORRECT_VERSION_LIBRARY));
    EXPECT_TRUE(LibraryManager::validateLibrary(LOAD_CALLOUT_LIBRARY));
    EXPECT_TRUE(LibraryManager::validateLibrary(LOAD_ERROR_CALLOUT_LIBRARY));
    EXPECT_FALSE(LibraryManager::validateLibrary(NOT_PRESENT_LIBRARY));
    EXPECT_FALSE(LibraryManager::validateLibrary(NO_VERSION_LIBRARY));
    EXPECT_TRUE(LibraryManager::validateLibrary(CALLOUT_PARAMS_LIBRARY));
}
};
