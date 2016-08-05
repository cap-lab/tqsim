#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "hsim_mmap.h"
#include "hsim_syscall.h"
#include "hsim_stub.h"

mem_region *mem_region_head = 0;

void memmap_initialize(void)
{
	mem_region_head = 0;
}


void memmap_destroy(void)
{
	mem_region *pRegion = mem_region_head;

	while (pRegion != 0) {
		mem_region *pDel = pRegion;
		pRegion = pRegion->next;
		free(pDel);
	}
}


//mem_region is connected as a linked list

void memmap_add_region(mem_region * pRegion)
{
	pRegion->prev = 0;
	pRegion->next = 0;

	if (mem_region_head == 0) {
		// if it is head
		mem_region_head = pRegion;
	} else {
		// connect it to tail
		mem_region *prevRegion = mem_region_head;

		while (prevRegion->next != 0)
			prevRegion = prevRegion->next;
		prevRegion->next = pRegion;
		pRegion->prev = prevRegion;
	}
}

int memmap_is_local(Address addr)
{
	int ret = 0;
	if (addr < MEM_LOCAL_BASE + MEM_LOCAL_SIZE)
		ret = 1;
	return ret;
}

int memmap_is_shared(Address addr){
	mem_region *pRegion = memmap_find_mem_region(addr);

	if (addr >= SYS_REG_BASE && addr <  SYS_REG_BASE + NUM_SYS_REG * sizeof(uint32_t)) {
		return 1;
	}

	if (pRegion != 0){
		if (pRegion->type == MEM_SCRATCHPAD ||
				pRegion->type == MEM_SHARED_C ||
				pRegion->type == MEM_SHARED_NC){
			return 1;
		}
	}
	return 0;

}


MemoryType memmap_get_type(Address addr)
{

	if (addr >= SYS_REG_BASE && addr <  SYS_REG_BASE + NUM_SYS_REG * sizeof(uint32_t)) {
		return MEM_INTERNAL;
	}
	mem_region *pRegion = memmap_find_mem_region(addr);

	if (pRegion != 0)
		return pRegion->type;
	else
		return MEM_UNKNOWN;
}



mem_region *memmap_find_mem_region(Address addr)
{
    mem_region *pRegion = mem_region_head;

    while (pRegion != 0) {
	if (addr >= pRegion->base && addr < pRegion->base + pRegion->size)
	    break;
	pRegion = pRegion->next;
    }
    // pull up recent region to top
    if (pRegion != 0 && pRegion != mem_region_head) {
	// remove selected item
	pRegion->prev->next = pRegion->next;
	if (pRegion->next != 0)
	    pRegion->next->prev = pRegion->prev;
	// insert selected to top
	pRegion->next = mem_region_head;
	mem_region_head->prev = pRegion;
	mem_region_head = pRegion;
    }
    return pRegion;
}


