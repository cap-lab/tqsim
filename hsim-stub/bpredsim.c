#include "config-host.h"
#include "bpredsim.h"
#include "perfmodel.h"
#include "config.h"

struct bpred_t *pred;

void bpredsim_initialize(void){
	pred = malloc(sizeof(struct bpred_t));
	bpred_create(pred, 4096, 2, 2);
}
int bpredsim_access(uint32_t cur_PC, uint32_t next_PC, int is_cond){

	pred->num_lookup++;	
	if (is_cond){
		unsigned ret = bpred_lookup(pred, cur_PC);
		unsigned taken = (next_PC != (cur_PC+sizeof(uint32_t)));
		if (ret != taken)
			pred->num_miss++;
		bpred_update(pred, cur_PC, taken);
		return ret == taken;

	}
	return 1;
}

int bpredsim_nummiss(void){
	return pred->num_miss;
}

uint64_t bpredsim_penaltysum(void){
	return bpred_penalty * pred->num_miss;
}

void bpredsim_end(void){
#ifndef CONFIG_HSIM
	bpred_printStat(pred);
#endif
	free(pred);

}

