#include "config-host.h"
#include "bpredsim.h"
#include "perfmodel.h"
#include "config.h"

struct bpred_t *pred;

void bpredsim_initialize(void){
	pred = malloc(sizeof(struct bpred_t));
	bpred_create(pred, 2, 4096,4096, 2, 2);
	
}
int bpredsim_access(uint32_t cur_PC, uint32_t next_PC, int is_cond){

	BPHistory hist;
	pred->num_lookup++;	
	if (is_cond){
		unsigned pred_taken = bpred_lookup(pred, cur_PC, &hist);
//		printf("%d\n", pred_taken);
		unsigned actual_taken = (next_PC != (cur_PC+sizeof(uint32_t)));
		if (pred_taken != actual_taken)
			pred->num_miss++;
		bpred_update(pred, cur_PC, actual_taken, &hist, pred_taken!=actual_taken);
//		printf("bpred cur_PC:%x, next_PC:%x, pred_taken:%d actual_taken:%d\n", cur_PC, next_PC, pred_taken, actual_taken);
		return pred_taken == actual_taken;
	}
	else {
		unsigned pred_taken = bpred_lookup(pred, cur_PC, &hist);
		unsigned actual_taken = (next_PC != (cur_PC+sizeof(uint32_t)));
		bpred_uncondBranch(pred, cur_PC, &hist);
		bpred_update(pred, cur_PC, actual_taken, &hist, pred_taken!=actual_taken);
	}
	return 1;
}

int bpredsim_num_mispred(void){
	return pred->num_miss;
}


void bpredsim_end(void){
#ifndef CONFIG_HSIM
	bpred_printStat(pred);
#endif
	free(pred);

}

