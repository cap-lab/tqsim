#ifndef BPREDSIM_H
#define BPREDSIM_H

#include <stdlib.h>
#include "bpred_bimod.h"

extern struct bpred_t* pred;

void bpredsim_initialize(void);

//next_PC is actual PC
int bpredsim_access(uint32_t cur_PC, uint32_t next_PC, int is_cond);
int bpredsim_num_mispred(void);
void bpredsim_end(void);

#endif

