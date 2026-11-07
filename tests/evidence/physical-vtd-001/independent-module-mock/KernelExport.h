#pragma once
#include <Drivers.h>
#include <module.h>
#include <cstddef>
using area_id=int32;
#define B_PAGE_SIZE 4096
#define B_ANY_KERNEL_ADDRESS 1
#define B_UNCACHED_MEMORY 2
#define B_KERNEL_READ_AREA 4
area_id map_physical_memory(const char*,uint64,size_t,uint32,uint32,void**);
status_t delete_area(area_id);
