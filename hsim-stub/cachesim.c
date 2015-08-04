#include "config-host.h"
#include "cache.h"
#include "cachesim.h"
#include "config.h"

//#include "../tcg/tcg-opc.h"

Cache_t* il1_cache;
Cache_t* dl1_cache;
Cache_t* ul2_cache;

//Cache_t* dl1_cache_dp;		//for depanal

int fetchBufferSize = 64;
uint32_t fetchBufferMask;

#define fetchbufferalignpc(addr) (addr & ~(fetchBufferMask))

void cachesim_initialize(void){
	fetchBufferMask =  fetchBufferSize - 1;

	il1_cache = dl1_cache = ul2_cache = 0;
	

	//ssim-outorder
//	cache_initialize(il1_cache, "il1",512 , 32, 1, LRU, 0, 10);
//	cache_initialize(dl1_cache, "dl1",128, 32, 4, LRU, 0, 10);
//	cache_initialize(ul2_cache, "ul2",1024 , 64, 4, LRU, 0, 150);

	//gem5


	if (perfmodel.existIL1){
		il1_cache = (Cache_t*) malloc(sizeof(Cache_t));
		cache_initialize(il1_cache, "il1",perfmodel.il1.num_set , 
				perfmodel.il1.size_blk, perfmodel.il1.assoc, LRU, 0, 
				perfmodel.existUL2?perfmodel.ul2.latency:perfmodel.mem.latency);
	}

	if (perfmodel.existDL1){
		dl1_cache = (Cache_t*) malloc(sizeof(Cache_t));
		cache_initialize(dl1_cache, "dl1",perfmodel.dl1.num_set , 
				perfmodel.dl1.size_blk, perfmodel.dl1.assoc, LRU, 0, 
				perfmodel.existUL2?perfmodel.ul2.latency:perfmodel.mem.latency);
	}

	//	cache_initialize(dl1_cache_dp, "dl1_dp",perfmodel.dl1.num_set , perfmodel.dl1.size_blk, 
	//									  perfmodel.dl1.assoc, LRU, 0, perfmodel.dl1.miss_penalty);

	if (perfmodel.existUL2){
		ul2_cache = (Cache_t*) malloc(sizeof(Cache_t));
		cache_initialize(ul2_cache, "ul2",perfmodel.ul2.num_set , 
				perfmodel.ul2.size_blk, perfmodel.ul2.assoc, LRU, 0, 
				perfmodel.mem.latency);
	}

}

int cachesim_iaccess(uint64_t cur_cycle, uint32_t addr){
	int eaddr = fetchbufferalignpc(addr);
	int lat = 0;
	if (il1_cache)
		lat += cache_access(cur_cycle, il1_cache, Read, eaddr);
   
#ifndef CONFIG_HSIM
    if (ul2_cache && lat > il1_cache->config.hit_lat){
		lat += cache_access(cur_cycle, ul2_cache, Read, eaddr);
	}
#endif
	return lat;
}

int cachesim_daccess(uint64_t cur_cycle, uint32_t addr){
	
	int lat = 0;
	if (dl1_cache)
		lat += cache_access(cur_cycle, dl1_cache, Read, addr);

#ifndef CONFIG_HSIM
    if (ul2_cache && lat > dl1_cache->config.hit_lat){
		lat += cache_access(cur_cycle, ul2_cache, Read, addr);
	}
#endif

	return lat;
}

/*
int cachesim_daccess_dp(uint64_t cur_cycle, uint32_t addr){
	
	int lat = cache_access(cur_cycle, dl1_cache_dp, Read, addr);
	return lat;
}


void cachesim_warm_dcache_dp(void){
	cache_copy(dl1_cache_dp->cache_set, dl1_cache->cache_set, perfmodel.dl1.num_set); 
}
*/

uint64_t cachesim_il1Penalty(void){
	if (il1_cache)
		return il1_cache->num_miss * il1_cache->config.miss_lat;			
	return 0;
}

uint64_t cachesim_ul2Penalty(void){
	if (ul2_cache)
		return ul2_cache->num_miss_star * ul2_cache->config.miss_lat;			
	return 0;
}


uint64_t cachesim_nummiss(void){
	int val = 0;
	if (il1_cache)
		val += il1_cache->num_miss;
	if (ul2_cache)
		val += ul2_cache->num_miss_star;
	return val;
}


void cachesim_end(void){
#ifdef CONFIG_HSIM
	if (il1_cache)
		cache_print(il1_cache);
#else
	if (il1_cache)
		cache_print(il1_cache);
	if (dl1_cache)
		cache_print(dl1_cache);
	if (ul2_cache)
		cache_print(ul2_cache);

#endif
	if (il1_cache)
		cache_close(il1_cache);
	if (dl1_cache)
		cache_close(dl1_cache);
	if (ul2_cache)
		cache_close(ul2_cache);
}
