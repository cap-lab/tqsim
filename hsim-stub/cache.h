#ifndef CACHE_H
#define CACHE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>


/* address type definition */
typedef uint32_t md_addr_t;

/* cache replacement policy */
typedef enum Cache_policy {
	LRU,      /* replace least recently used block (perfect LRU) */
	Random,   /* replace a random block */
	FIFO      /* replace the oldest block in the set */
} Cache_policy;

typedef enum CacheAccessType{
	Read,
	Write
} CacheAccessType;

typedef struct Cache_config{
	char* name;
	int nset;
	int bsize;
	int assoc;
	enum Cache_policy policy;
	unsigned int hit_lat;
	unsigned int miss_lat;
} Cache_config;

typedef struct Cache_blk_t{
	bool valid;
	bool dirty;
	md_addr_t tag;
	uint8_t* data;
	struct Cache_blk_t *prev_blk;
	struct Cache_blk_t *next_blk;
} Cache_blk_t;


//maintain an array list of cache_blk_t
typedef struct Cache_set_t{
	Cache_blk_t *first_blk;
	Cache_blk_t *last_blk;
} Cache_set_t;

typedef struct Cache_t{
	Cache_set_t **cache_set;	
	Cache_config config;

	uint64_t num_access;

	uint64_t num_miss;
	uint64_t num_miss_star;

	unsigned int len_tag_bit;
    unsigned int len_idx_bit;
	unsigned int len_offset_bit;
	
	uint64_t cycle_lastmiss;

	uint32_t offset_mask;
	uint32_t addr_mask;

} Cache_t;

void cacheblock_initialize(Cache_t *cache, Cache_blk_t* block);

void cacheset_initialize(Cache_t *cache, Cache_set_t* set, int assoc);
/* when a block is accessed, the block comes at the first place */
void cacheset_moveBlkAtFirst(Cache_set_t* set, Cache_blk_t* blk);
void cacheset_printSetTags(Cache_set_t* set);

void cache_initialize(Cache_t* cache, const char* name, int nset, int bsize, int assoc, 
					enum Cache_policy policy, unsigned int hit_lat, unsigned int miss_lat);

void cache_copy(Cache_set_t **target_set, Cache_set_t **source_set, int nset);
int cache_access(uint64_t cur_cycle, Cache_t* cache, enum CacheAccessType type, md_addr_t addr);

int cache_readTraceFile(Cache_t* cache, const char* filename);
int mcache_readTraceFile(Cache_t* cache1, Cache_t* cache2, const char* filename);

void cache_print(Cache_t* cache);
void cache_close(Cache_t* cache);

void cache_invalidate_data(Cache_t* cache);
int cache_access_data(uint64_t cur_cycle, Cache_t* cache, enum CacheAccessType type, md_addr_t addr, uint8_t* blk, int size);
int cache_update_data(uint64_t cur_cycle, Cache_t* cache, md_addr_t addr, uint8_t* blk);

#endif
