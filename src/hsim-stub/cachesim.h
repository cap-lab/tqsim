#ifndef CACHESIM_H
#define CACHESIM_H

#include <stdint.h>
#include "cache.h"


typedef enum _CacheMissType {L1Hit, L1Miss, L2Miss} CacheMissType;

extern Cache_t* il1_cache;
extern Cache_t* dl1_cache;
extern Cache_t* ul2_cache;
extern Cache_t* dl1_cache_dp;		//for depanal

void cachesim_initialize(void);
CacheMissType cachesim_iaccess(uint64_t cur_cycle, uint32_t addr);
CacheMissType cachesim_daccess(uint64_t cur_cycle, uint32_t addr, int is_write);

uint64_t cachesim_il1_num_miss(void);
uint64_t cachesim_dl1_num_miss(void);
uint64_t cachesim_ul2_num_miss(void);

int cachesim_il1_miss_latency(void);
int cachesim_dl1_miss_latency(void);
int cachesim_ul2_miss_latency(void);

int cachesim_il1_hit_latency(void);
int cachesim_dl1_hit_latency(void);
int cachesim_ul2_hit_latency(void);


void cachesim_end(void);



#endif
