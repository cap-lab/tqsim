#ifndef BPRED_SIMPLE_H
#define BPRED_SIMPLE_H

#include "sat_counter.h"
typedef uint32_t Addr;

struct bpred_t {
	 /** Size of the local predictor. */
	unsigned localPredictorSize;
	unsigned localPredictorSets;
	unsigned localCtrBits;
	unsigned instShiftAmt;
	unsigned indexMask;
	SatCounter *localCtrs;
	int num_lookup;
	int num_miss;
};

void bpred_create(struct bpred_t *pred, int localPredictorSize, int localCtrBits, int instShiftAmt);
void bpred_reset(struct bpred_t *pred);
unsigned bpred_lookup(struct bpred_t *pred, Addr branch_addr);
void bpred_update(struct bpred_t *pred, Addr branch_addr, int taken);
unsigned bpred_getPrediction(struct bpred_t *pred, uint8_t count);
unsigned bpred_getLocalIndex(struct bpred_t *pred, Addr branch_addr);
void bpred_printStat(struct bpred_t *pred);

#endif

