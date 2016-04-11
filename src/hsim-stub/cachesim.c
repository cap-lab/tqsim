#include "config-host.h"
#include "cache.h"
#include "cachesim.h"
#include "config.h"

Cache_t* il1_cache;
Cache_t* dl1_cache;
Cache_t* ul2_cache;

int fetchBufferSize = 64;
uint32_t fetchBufferMask;

#define fetchbufferalignpc(addr) (addr & ~(fetchBufferMask))

void cachesim_initialize(void){
	fetchBufferMask =  fetchBufferSize - 1;

	il1_cache = dl1_cache = ul2_cache = 0;

	if (perfmodel.existIL1){
		il1_cache = (Cache_t*) malloc(sizeof(Cache_t));
		cache_initialize(il1_cache, "il1",perfmodel.il1.num_set , 
				perfmodel.il1.size_blk, perfmodel.il1.assoc, LRU, 
				perfmodel.il1.latency,
				perfmodel.existUL2?perfmodel.ul2.latency+perfmodel.il1.latency*2 : perfmodel.mem.latency+perfmodel.il1.latency*2);
	}

	if (perfmodel.existDL1){
		dl1_cache = (Cache_t*) malloc(sizeof(Cache_t));
		cache_initialize(dl1_cache, "dl1",perfmodel.dl1.num_set , 
				perfmodel.dl1.size_blk, perfmodel.dl1.assoc, LRU,  
				perfmodel.dl1.latency,
				perfmodel.existUL2?perfmodel.ul2.latency+perfmodel.dl1.latency*2:perfmodel.mem.latency+perfmodel.dl1.latency*2);
	}

	if (perfmodel.existUL2){
		ul2_cache = (Cache_t*) malloc(sizeof(Cache_t));
		cache_initialize(ul2_cache, "ul2",perfmodel.ul2.num_set , 
				perfmodel.ul2.size_blk, perfmodel.ul2.assoc, LRU, perfmodel.ul2.latency, 
//				perfmodel.mem.latency + perfmodel.ul2.latency*2 );
				perfmodel.mem.latency+perfmodel.ul2.latency*2);

	}

}

CacheMissType cachesim_iaccess(uint64_t cur_cycle, uint32_t addr){
	int eaddr = fetchbufferalignpc(addr);
	CacheMissType miss_type;
	if (il1_cache){
		if (cache_access(cur_cycle, il1_cache, Read, eaddr)){
			miss_type = L1Hit;
		}
		else {
			miss_type = L1Miss;
		}
	}
#ifndef CONFIG_HSIM
    if (ul2_cache && miss_type == L1Miss){
		if (!cache_access(cur_cycle, ul2_cache, Read, eaddr)){
			miss_type = L2Miss;
		}
	}
#endif
	return miss_type;
}

CacheMissType cachesim_daccess(uint64_t cur_cycle, uint32_t addr, int is_write){
	CacheMissType miss_type;

	if (dl1_cache){
		if (cache_access(cur_cycle, dl1_cache, is_write?Write:Read, addr)){
			miss_type = L1Hit;
		}
		else {
			miss_type = L1Miss;
		}
	}
#ifndef CONFIG_HSIM
    if (ul2_cache && miss_type == L1Miss){
		if (!cache_access(cur_cycle, ul2_cache, is_write?Write:Read, addr)){
			miss_type = L2Miss;
		}
	}
#endif
	return miss_type;

}


uint64_t cachesim_il1_num_miss(void){
	if (il1_cache){
		return il1_cache->num_read_miss;
	}
	return 0;
}
uint64_t cachesim_dl1_num_miss(void){
	if (dl1_cache){
		return dl1_cache->num_read_miss;
	}
	return 0;

}
uint64_t cachesim_ul2_num_miss(void){
	if (ul2_cache){
		return ul2_cache->num_miss_star;
	}
	return 0;
}


int cachesim_il1_hit_latency(void){
	if (il1_cache){
		return il1_cache->config.hit_lat;
	}
	return 0;

}
int cachesim_dl1_hit_latency(void){
	if (dl1_cache){
		return dl1_cache->config.hit_lat;
	}
	return 0;

}
int cachesim_ul2_hit_latency(void){
	if (ul2_cache){
		return ul2_cache->config.hit_lat;
	}
	return 0;

}

int cachesim_il1_miss_latency(void){
	if (il1_cache){
		return il1_cache->config.miss_lat;
	}
	return 0;

}
int cachesim_dl1_miss_latency(void){
	if (dl1_cache){
		return dl1_cache->config.miss_lat;
	}
	return 0;

}
int cachesim_ul2_miss_latency(void){
	if (ul2_cache){
		return ul2_cache->config.miss_lat;
	}
	return 0;

}



void cachesim_end(void){
#ifdef CONFIG_HSIM
//	if (il1_cache)
//		cache_print(il1_cache);
#else
//	if (il1_cache)
//		cache_print(il1_cache);
//	if (dl1_cache)
//		cache_print(dl1_cache);
//	if (ul2_cache)
//		cache_print(ul2_cache);

#endif
	if (il1_cache)
		cache_close(il1_cache);
	if (dl1_cache)
		cache_close(dl1_cache);
	if (ul2_cache)
		cache_close(ul2_cache);
}
