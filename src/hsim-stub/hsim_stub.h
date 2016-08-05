#ifndef __hsim_stub_h
#define __hsim_stub_h

#include "hsim_common.h"

#define MEM_LOCAL_BASE 0x00000000
#define MEM_LOCAL_SIZE 0x30000000


void set_affinity(int alloc_id);
void hsim_start(void);
void hsim_end(uint64_t cycle);
void comm_read_env(void);

uint64_t hsim_internal_access(uint64_t cur_cycle, Address address, int rw, uint64_t * data,  int size, AccessType blocking);
uint64_t mem_access(uint64_t cycle, Address address, int rw, uint64_t * data,  int size, AccessType  blocking);
uint64_t hsim_access(uint64_t cur_cycle, Address address, int rw, uint64_t * data,  int size, AccessType blocking);
void hsim_notice(uint64_t cur_cycle);
void hsim_syscall_init(void);
#endif
