#include <stdio.h>
#include <stdlib.h>
#include "bpred_simple.h"


void bpred_create(struct bpred_t *pred, int _localPredictorSize, int _localCtrBits, int _instShiftAmt)
{
	
	pred->localPredictorSize = 	_localPredictorSize;
	pred->localCtrBits = _localCtrBits;
	pred->instShiftAmt = _instShiftAmt;

    pred->localPredictorSets = pred->localPredictorSize / pred->localCtrBits;
    pred->indexMask = pred->localPredictorSets - 1;
    pred->localCtrs = (SatCounter*)malloc(sizeof(SatCounter)*pred->localPredictorSets);

	pred->num_lookup = 0;
	pred->num_miss = 0;

	int i;
    for ( i = 0; i < pred->localPredictorSets; ++i)
        init_counter(&(pred->localCtrs[i]), pred->localCtrBits/2, pred->localCtrBits);
}    
     
    
void bpred_reset(struct bpred_t *pred)
{
	int i;
    for (i = 0; i < pred->localPredictorSets; ++i) {
        reset_counter(&(pred->localCtrs[i]));
    }
}

unsigned bpred_lookup(struct bpred_t *pred, Addr branch_addr)
{
    unsigned taken;
    uint8_t counter_val;
    unsigned local_predictor_idx = bpred_getLocalIndex(pred, branch_addr);
    counter_val = read_counter(&(pred->localCtrs[local_predictor_idx]));
    taken = bpred_getPrediction(pred, counter_val);
    return taken;
}

void bpred_update(struct bpred_t *pred, Addr branch_addr, int taken)
{
    unsigned local_predictor_idx;
    local_predictor_idx = bpred_getLocalIndex(pred, branch_addr);

    if (taken) {
        increment_counter(&(pred->localCtrs[local_predictor_idx]));
    } else {
        decrement_counter(&(pred->localCtrs[local_predictor_idx]));
    }
}

inline unsigned bpred_getPrediction(struct bpred_t *pred, uint8_t count)
{
    return (count >> (pred->localCtrBits - 1));
}


#define MD_BR_SHIFT 2
#define BIMOD_HASH(ADDR)    ((((ADDR) >> 19) ^ ((ADDR) >> MD_BR_SHIFT)) & (2048-1))

inline unsigned bpred_getLocalIndex(struct bpred_t *pred, Addr branch_addr)
{
    return BIMOD_HASH(branch_addr);
}

void bpred_printStat(struct bpred_t *pred){
	printf("--------------\n");
	printf("Bpred \n");
	printf("--------------\n");
	printf("num_lookups: %d\n", pred->num_lookup);
	printf("num_misses: %d\n",pred->num_miss);
}


