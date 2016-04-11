#ifndef __ROB_H
#define __ROB_H

#include "inst.h"

typedef struct ROBEntry_
{
	Inst* inst;
} ROBEntry;

typedef struct ROB_{
	ROBEntry entries[60];
	int start_idx;
	int end_idx;
	int size;
	int capacity;
} ROB;

void rob_init(ROB *rob, int _size);
void rob_insert_inst(ROB *rob, Inst *inst);
void rob_remove_inst(ROB *rob);
Inst* rob_get_oldest_inst(ROB *rob);
Inst* rob_get_inst(ROB *rob, int idx);
bool is_rob_full(ROB *rob);
bool is_rob_empty(ROB *rob);


#endif
