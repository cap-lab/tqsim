#ifndef __hsim_mmap_h
#define __hsim_mmap_h

#include "hsim_common.h"
#include "hsim_memtype.h"

typedef struct mem_region {
    Address base;
    uint32_t size;
    uint64_t latency;
    MemoryType type;
    struct mem_region *next;
    struct mem_region *prev;
} mem_region;

void memmap_initialize(void);
void memmap_destroy(void);
void memmap_add_region(mem_region * pRegion);
MemoryType memmap_get_type(Address addr);
int memmap_is_shared(Address addr);
int memmap_is_local(Address addr);
mem_region *memmap_find_mem_region(Address addr);




#endif
