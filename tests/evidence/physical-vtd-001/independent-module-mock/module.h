#pragma once
#include <Drivers.h>
struct module_info { const char* name; uint32 flags; status_t (*ops)(int32,...); };
status_t get_module(const char*,module_info**);
