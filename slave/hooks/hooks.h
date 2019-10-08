#pragma once

#include <kea/hooks/callout_handle.h>
#include <kea/configure/json_conf.h>

namespace {

const int KEA_HOOKS_VERSION = 3;

const char* const LOAD_FUNCTION_NAME = "load";
const char* const UNLOAD_FUNCTION_NAME = "unload";
const char* const VERSION_FUNCTION_NAME = "version";

typedef int (*version_function_ptr)();
typedef int (*load_function_ptr)(const kea::configure::JsonObject&);
typedef int (*unload_function_ptr)();

}

