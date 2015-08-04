#include "config-host.h"
#include "vcache.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

void vcache_initialize(Cache_t* cache, const char* name, int nset, int bsize, int assoc, 
		                 enum Cache_policy policy, unsigned int hit_lat,
						 unsigned int miss_lat){
	
	int i;
	cache->num_access = 0;
	cache->num_miss = 0;
	cache->num_miss_star = 0;
	cache->cycle_lastmiss = -1;


	cache->config.name = (char*) malloc(strlen(name)+1);
	strcpy(cache->config.name, name);
	cache->config.nset = nset;
	cache->config.bsize = bsize;
	cache->config.assoc = assoc;
	cache->config.policy = policy;
	cache->config.hit_lat = hit_lat;
	cache->config.miss_lat = miss_lat;
	
	cache->len_offset_bit = (int)log2((double)bsize);
	cache->len_idx_bit = (int)log2((double)nset);
	cache->len_tag_bit = 32 - cache->len_offset_bit - cache->len_idx_bit;

	cache->cache_set = (Cache_set_t**) malloc(sizeof(Cache_set_t*)*nset);
	for (i=0 ; i<nset ; i++){
		
		cache->cache_set[i] = (Cache_set_t*) (malloc (sizeof(Cache_set_t)));
		cacheset_initialize(cache->cache_set[i], assoc);
	}
	
//	fprintf(stderr, "Cache %s is initialized...\n", name);
}



void vcache_copy(Cache_set_t **target_set, Cache_set_t **source_set, int nset){
	int i;

	target_set = (Cache_set_t**) malloc(sizeof(Cache_set_t*)*nset);
	for (i=0 ; i<nset ; i++){
		//target_set[i] = (Cache_set_t*) (malloc (sizeof(Cache_set_t)));
		//cacheset_initialize(target_set[i], assoc);
	
		Cache_blk_t *target_blk = target_set[i]->first_blk;
		Cache_blk_t *source_blk = source_set[i]->first_blk;

		
		while (target_blk!=NULL){

			target_blk->valid = source_blk->valid;
			target_blk->dirty = source_blk->dirty;
			target_blk->tag = source_blk->tag;
			
			target_blk = target_blk->next_blk;
			source_blk = source_blk->next_blk;
		}
		
	}
	
	//fprintf(stderr, "Cache %s is initialized...\n", name);




	//memcpy(target_set, source_set, sizeof(Cache_set_t));
}


int vcache_access(uint64_t cur_cycle, Cache_t* cache, enum CacheAccessType type, md_addr_t addr){
	
	//find the set by using idx. Do masking...
//return 0;
	int i;
	unsigned int mask =  (1 << (cache->len_idx_bit))-1;
	unsigned int idx = ((addr >> cache->len_offset_bit) & mask);
	unsigned int tag =  (addr >> (cache->len_offset_bit + cache->len_idx_bit));

	cache->num_access++;
	
	/*
	cout << "ADDR    :" << bitset<32>(addr) << endl;
	
	cout << "MASK BIT:" << bitset<32>(mask) << endl;	
	cout << "IDX BIT :" << bitset<32>(idx) << endl;
	cout << "TAG BIT :" << bitset<32>(tag) << endl;
*/

	//cout << "ADDR    :" << hex << addr << endl;
	//cout << "MASK BIT:" << hex << mask << endl;	
	//cout << "IDX BIT :" << hex << idx << endl;
	//cout << "TAG BIT :" << hex << tag << endl;

	Cache_set_t *set = cache->cache_set[idx];
	//try to find
	
	//cache_printSetTags(set);

	Cache_blk_t *target_blk = set->first_blk;
	
	while (target_blk!=NULL){
		if (target_blk->valid && target_blk->tag == tag){
			//cout << "HIT!!" << endl;
			if (cache->config.policy == LRU)
				cacheset_moveBlkAtFirst(set, target_blk);

			return cache->config.hit_lat;
		}
		target_blk = target_blk->next_blk;
	}
	switch (cache->config.policy){
		case LRU:
		case FIFO:
			target_blk = set->last_blk;
			break;
		case Random:
			srand(time(NULL));
			int victimIdx = rand() % cache->config.assoc;
			target_blk = set->first_blk;
			for (i=0 ; i<victimIdx ; i++){
				target_blk = target_blk->next_blk;
			}
			 
			break;
	}
//	cout << "victim tag: " << hex << target_blk->tag << endl;
	target_blk->valid = true;
	target_blk->tag = tag;
	//cout << "MISS!!" << endl;
	//


	// I don't want to maintain the window in the simulator.
	// I just want to check the distance between the current instruction and the last miss instruction.
	
//	printf("%u\n", (unsigned) cur_cycle);
	if (cache->cycle_lastmiss==-1 || (cur_cycle - cache->cycle_lastmiss) > cache->config.miss_lat ){ 
		cache->num_miss_star++;
		cache->cycle_lastmiss = cur_cycle;
	}
	else {
//		printf("OVERLAP!!!");
//		cache->cycle_lastmiss = cur_cycle;
	}
	cache->num_miss++;
	//cache->cycle_lastmiss = cur_cycle;
	cacheset_moveBlkAtFirst(set, target_blk);

	return cache->config.miss_lat;
	
}



void vcache_print(Cache_t* cache){

		fprintf(stderr, "--------------\n");	
		fprintf(stderr, "Cache %s\n", cache->config.name);
		fprintf(stderr, "--------------\n");	
		fprintf(stderr, "num_access: %" PRIu64 "\n",cache->num_access);
		fprintf(stderr, "num_miss: %" PRIu64 "\n", cache->num_miss);
		fprintf(stderr, "num_miss_star: %" PRIu64 "\n", cache->num_miss_star);

}

void vcache_close(Cache_t* cache){

	if (cache->cache_set!=NULL){
		int i;
      	for (i=0 ; i<cache->config.nset ; i++){
			Cache_blk_t *blk = cache->cache_set[i]->first_blk;
			
			while (blk!=NULL){
				Cache_blk_t *nextblk = blk->next_blk;
				free(blk);
				blk = nextblk;
			}
			free(cache->cache_set[i]);
	    }
		free(cache->cache_set);
	}
	free(cache);
	
}

void vcacheblock_initialize(Cache_blk_t* block){
	block->valid = false;
	block->dirty = false;
	block->tag = 0;
	block->prev_blk = NULL;
	block->next_blk = NULL;
}


void vcacheset_initialize(Cache_set_t *set, int assoc){
	int i;	
	for (i=0 ; i<assoc ; i++){
		Cache_blk_t *blk = (Cache_blk_t*) malloc(sizeof(Cache_blk_t));
		//printf("%X ", blk);
		cacheblock_initialize(blk);
		if (i==0){
			set->first_blk = blk;
			set->last_blk = blk;
		}
		else {
			blk->prev_blk = set->last_blk;
			blk->next_blk = NULL;
			set->last_blk->next_blk = blk; 
			set->last_blk = blk;
		}
	}
	//cache_printSetTags(set);

}


void vcacheset_moveBlkAtFirst(Cache_set_t* set, Cache_blk_t* blk){

	if (set->first_blk == blk){
		return;
	}
	
	//remove
	if (blk->prev_blk != NULL){
		blk->prev_blk->next_blk = blk->next_blk;
	}
	else {
		set->first_blk = blk->next_blk;
	}
	if (blk->next_blk != NULL){
		blk->next_blk->prev_blk = blk->prev_blk;
	}
	else {
		set->last_blk = blk->prev_blk;
	}

	//add
	blk->prev_blk = NULL;
	blk->next_blk = set->first_blk;

	set->first_blk->prev_blk = blk;
	set->first_blk = blk;

}


void vcacheset_printSetTags(Cache_set_t* set){

	Cache_blk_t *target_blk = set->first_blk;
	while (target_blk!=NULL){
		printf("%X ", target_blk->tag);
		target_blk = target_blk->next_blk;
	}
	printf("\n");
}


