#ifndef __BPRED_BIMOD_H
#define __BPRED_BIMOD_h

/* @file
 * Implementation of a bi-mode branch predictor
 */

#include "sat_counter.h"
#include <stdbool.h>

typedef uint32_t Addr;

typedef struct _BPHistory {
	unsigned globalHistoryReg;
	// was the taken array's prediction used?
	// true: takenPred used
	// false: notPred used
	bool takenUsed;
	// prediction of the taken array
	// true: predict taken
	// false: predict not-taken
	bool takenPred;
	// prediction of the not-taken array
	// true: predict taken
	// false: predict not-taken
	bool notTakenPred;
	// the final taken/not-taken prediction
	// true: predict taken
	// false: predict not-taken
	bool finalPred;
	bool valid;

} BPHistory;


struct bpred_t {

    // choice predictors
    SatCounter *choiceCounters;
    // taken direction predictors
    SatCounter *takenCounters;
    // not-taken direction predictors
    SatCounter *notTakenCounters;

    unsigned globalHistoryReg;
    unsigned globalHistoryBits;
    unsigned historyRegisterMask;

    unsigned choicePredictorSize;
    unsigned choiceCtrBits;
    unsigned choiceHistoryMask;
    unsigned globalPredictorSize;
    unsigned globalCtrBits;
    unsigned globalHistoryMask;

    unsigned choiceThreshold;
    unsigned takenThreshold;
    unsigned notTakenThreshold;

	unsigned num_lookup;
	unsigned num_miss;

	unsigned instShiftAmt;
};

void bpred_create(struct bpred_t *pred,
		unsigned instShiftAmt,
		int _globalPredictorSize,
		int _choicePredictorSize,
		int _choiceCtrBits,
		int _globalCtrBits);

void bpred_uncondBranch(struct bpred_t *pred, Addr pc, BPHistory *history);
void bpred_squash(struct bpred_t *pred, BPHistory *history);
unsigned bpred_lookup(struct bpred_t *pred, Addr branchAddr,  BPHistory *history);
void bpred_btbUpdate(struct bpred_t *pred, Addr branchAddr, BPHistory *history);
void bpred_update(struct bpred_t *pred, Addr branchAddr, bool taken, BPHistory *history, bool squashed);
void bpred_retireSquashed(struct bpred_t *pred, BPHistory *history);
void bpred_updateGlobalHistReg(struct bpred_t *pred, bool taken);
void bpred_printStat(struct bpred_t *pred);

#endif
