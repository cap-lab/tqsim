#ifndef PERF_MODEL_H
#define PERF_MODEL

#include <stdint.h>

extern char* command;
extern double cpi;
extern uint64_t num_insts;
extern uint32_t bpred_penalty;


void perfmodel_initialize(int argc, char** argv);
void perfmodel_logwrite(char* str);
uint64_t  perfmodel_getCycle(void);
uint64_t perfmodel_getSimpleCycle(void);
void perfmodel_logging(void);

void perfmodel_end(void);

#endif
