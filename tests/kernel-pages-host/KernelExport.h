// SPDX-License-Identifier: MIT
#pragma once
#include <stddef.h>
#include <stdint.h>
using status_t=int32_t; using area_id=int32_t; using uint32=uint32_t;
constexpr int B_OK=0, B_NO_MEMORY=-1, B_BAD_ADDRESS=-2, B_CURRENT_TEAM=-3;
constexpr unsigned B_PAGE_SIZE=4096, B_ANY_KERNEL_ADDRESS=1, B_FULL_LOCK=2;
constexpr unsigned B_KERNEL_READ_AREA=4, B_KERNEL_WRITE_AREA=8;
struct physical_entry { uint64_t address; size_t size; };
area_id create_area(const char*,void**,unsigned,size_t,unsigned,unsigned);
status_t get_memory_map_etc(int,const void*,size_t,physical_entry*,uint32*);
status_t delete_area(area_id);

constexpr int B_NOT_ALLOWED=-4;
