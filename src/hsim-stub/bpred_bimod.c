#include <stdio.h>
#include <stdlib.h>
#include "bpred_bimod.h"

static uint64_t mask(int nbits)
{
	return (nbits == 64) ? (uint64_t)-1LL : (1ULL << nbits) - 1;
}


void bpred_create(struct bpred_t *pred, 
		unsigned _instShiftAmt,
		int _globalPredictorSize,
		int _choicePredictorSize,
		int _choiceCtrBits,
		int _globalCtrBits)
{
	pred->instShiftAmt = _instShiftAmt;
	pred->globalHistoryReg = 0;
	pred->globalHistoryBits = _globalPredictorSize;
	pred->choicePredictorSize = _choicePredictorSize;
	pred->choiceCtrBits = _choiceCtrBits;
	pred->globalPredictorSize = _globalPredictorSize;
	pred->globalCtrBits = _globalCtrBits;

	pred->choiceCounters = (SatCounter*)malloc(sizeof(SatCounter)*pred->choicePredictorSize);
	pred->takenCounters = (SatCounter*)malloc(sizeof(SatCounter)*pred->globalPredictorSize);
	pred->notTakenCounters = (SatCounter*)malloc(sizeof(SatCounter)*pred->globalPredictorSize);

	pred->num_lookup = 0;
	pred->num_miss = 0;

	int i;
	for ( i = 0; i < pred->choicePredictorSize; ++i)
	{
		init_counter(&(pred->choiceCounters[i]), 0, pred->choiceCtrBits);
	}

	for ( i= 0 ; i< pred->globalPredictorSize ; ++i){
		init_counter(&(pred->takenCounters[i]), 0, pred->globalCtrBits);
		init_counter(&(pred->notTakenCounters[i]), 0, pred->globalCtrBits);
	}

	pred->historyRegisterMask = mask(pred->globalHistoryBits);
	pred->choiceHistoryMask = pred->choicePredictorSize - 1;
	pred->globalHistoryMask = pred->globalPredictorSize - 1;

	pred->choiceThreshold = (1ull  << (pred->choiceCtrBits - 1)) - 1;
	pred->takenThreshold = (1ull << (pred->choiceCtrBits - 1)) - 1;
	pred->notTakenThreshold = (1ull << (pred->choiceCtrBits - 1)) - 1;
}    


/*
 * For an unconditional branch we set its history such that
 * everything is set to taken. I.e., its choice predictor
 * chooses the taken array and the taken array predicts taken.
 */
void bpred_uncondBranch(struct bpred_t *pred, Addr pc, BPHistory *history)
{
    history->globalHistoryReg = pred->globalHistoryReg;
    history->takenUsed = true;
    history->takenPred = true;
    history->notTakenPred = true;
    history->finalPred = true;
	history->valid = true;
    bpred_updateGlobalHistReg(pred, true);
}



void bpred_squash(struct bpred_t *pred, BPHistory *history)
{
    pred->globalHistoryReg = history->globalHistoryReg;
	history->valid = false;
}


unsigned bpred_lookup(struct bpred_t *pred, Addr branchAddr,  BPHistory *history)

{

   unsigned choiceHistoryIdx = ((branchAddr >> pred->instShiftAmt)
                                & pred->choiceHistoryMask);
    unsigned globalHistoryIdx = (((branchAddr >> pred->instShiftAmt)
                                ^ pred->globalHistoryReg)
                                & pred->globalHistoryMask);

	int choicePrediction = 
		read_counter(&(pred->choiceCounters[choiceHistoryIdx])) > pred->choiceThreshold;

    int takenGHBPrediction = 
		read_counter(&(pred->takenCounters[globalHistoryIdx])) > pred->takenThreshold;

    int notTakenGHBPrediction = 
		read_counter(&(pred->notTakenCounters[globalHistoryIdx])) > pred->notTakenThreshold;

 
   	int finalPrediction;

    history->globalHistoryReg = pred->globalHistoryReg;
    history->takenUsed = choicePrediction;
    history->takenPred = takenGHBPrediction;
    history->notTakenPred = notTakenGHBPrediction;
	history->valid = true;

    if (choicePrediction) {
        finalPrediction = takenGHBPrediction;
    } else {
        finalPrediction = notTakenGHBPrediction;
    }

    history->finalPred = finalPrediction;
    bpred_updateGlobalHistReg(pred, finalPrediction);

    return finalPrediction;
}




void bpred_btbUpdate(struct bpred_t *pred, Addr branchAddr, BPHistory *history)
{
    pred->globalHistoryReg &= (pred->historyRegisterMask & ~(1ull));
}


/* Only the selected direction predictor will be updated with the final
 * outcome; the status of the unselected one will not be altered. The choice
 * predictor is always updated with the branch outcome, except when the
 * choice is opposite to the branch outcome but the selected counter of
 * the direction predictors makes a correct final prediction.
 */
void bpred_update(struct bpred_t *pred, Addr branchAddr, bool taken, BPHistory *history, bool squashed)
{
    if (history && history->valid) {

        unsigned choiceHistoryIdx = ((branchAddr >> pred->instShiftAmt)
                                    & pred->choiceHistoryMask);
        unsigned globalHistoryIdx = (((branchAddr >> pred->instShiftAmt)
                                    ^ history->globalHistoryReg)
                                    & pred->globalHistoryMask);


        if (history->takenUsed) {
            // if the taken array's prediction was used, update it
            if (taken) {
                increment_counter(&(pred->takenCounters[globalHistoryIdx]));
   
            } else {
		      decrement_counter(&(pred->takenCounters[globalHistoryIdx]));
   

            }
        } else {
            // if the not-taken array's prediction was used, update it
            if (taken) {
			    increment_counter(&(pred->notTakenCounters[globalHistoryIdx]));
   
            } else {
				decrement_counter(&(pred->notTakenCounters[globalHistoryIdx]));
   

            }
        }

        if (history->finalPred == taken) {
            /* If the final prediction matches the actual branch's
             * outcome and the choice predictor matches the final
             * outcome, we update the choice predictor, otherwise it
             * is not updated. While the designers of the bi-mode
             * predictor don't explicity say why this is done, one
             * can infer that it is to preserve the choice predictor's
             * bias with respect to the branch being predicted; afterall,
             * the whole point of the bi-mode predictor is to identify the
             * atypical case when a branch deviates from its bias.
             */
            if (history->finalPred == history->takenUsed) {
                if (taken) {
				    increment_counter(&(pred->choiceCounters[choiceHistoryIdx]));
   

                } else {
				    decrement_counter(&(pred->choiceCounters[choiceHistoryIdx]));
   

                }
            }
        } else {
            // always update the choice predictor on an incorrect prediction
            if (taken) {
                 increment_counter(&(pred->choiceCounters[choiceHistoryIdx]));
            } else {
			     decrement_counter(&(pred->choiceCounters[choiceHistoryIdx]));
            }
        }

        if (squashed) {
            if (taken) {
                pred->globalHistoryReg = (history->globalHistoryReg << 1) | 1;
            } else {
                pred->globalHistoryReg = (history->globalHistoryReg << 1);
            }
            pred->globalHistoryReg &= pred->historyRegisterMask;
        } else {
            history->valid = false;
        }
    }
}


void bpred_retireSquashed(struct bpred_t *pred, BPHistory *history)
{
	history->valid = false;
}

void bpred_updateGlobalHistReg(struct bpred_t *pred, bool taken)
{
    pred->globalHistoryReg = taken ? (pred->globalHistoryReg << 1) | 1 :
                               (pred->globalHistoryReg << 1);
    pred->globalHistoryReg &= pred->historyRegisterMask;
}

void bpred_printStat(struct bpred_t *pred){
	fprintf(stderr, "Bpred] ");
	fprintf(stderr, "num_lookups: %d / ", pred->num_lookup);
	fprintf(stderr, "num_misses: %d\n",pred->num_miss);
}


