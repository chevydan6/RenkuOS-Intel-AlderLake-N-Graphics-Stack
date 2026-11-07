#pragma once
#include <cstdint>
using uint8=uint8_t; using uint32=uint32_t; using uint64=uint64_t; using int32=int32_t; using status_t=int32_t;
#define B_OK 0
#define B_ERROR -1
#define B_BAD_VALUE -2
#define B_BUSY -3
#define B_MODULE_INIT 1
#define B_MODULE_UNINIT 2
#define B_KEEP_LOADED 1
#define B_DEVICE_OP_CODES_END 10000
