#ifndef __PERF_MODEL_H
#define __PERF_MODEL_H

#include <stdint.h>

extern char* command;

extern double effective_dispatch_width;
extern uint64_t num_insts;
extern uint32_t bpred_penalty;

void perfmodel_initialize(int argc, char** argv);
void perfmodel_logwrite(char* str);
uint64_t  perfmodel_getCycle(void);
uint64_t perfmodel_getSimpleCycle(void);

void perfmodel_sample_start(void);
void perfmodel_sample_end(void);
void perfmodel_sample_wrapup(void);
void perfmodel_update(uint64_t cycle, int bpred_penalty);

void perfmodel_logging(void);
void perfmodel_end(void);

#endif
