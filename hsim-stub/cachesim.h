#ifndef CACHESIM_H
#define CACHESIM_H

#include <stdint.h>
#include "cache.h"

extern Cache_t* il1_cache;
extern Cache_t* dl1_cache;
extern Cache_t* ul2_cache;
extern Cache_t* dl1_cache_dp;		//for depanal

void cachesim_initialize(void);
int cachesim_iaccess(uint64_t cur_cycle, uint32_t addr);
int cachesim_daccess(uint64_t cur_cycle, uint32_t addr);

uint64_t cachesim_il1Penalty(void);
uint64_t cachesim_ul2Penalty(void);

uint64_t cachesim_nummiss(void);
void cachesim_end(void);



#endif
