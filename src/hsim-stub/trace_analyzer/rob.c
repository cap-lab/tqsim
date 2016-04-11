#include "rob.h"

void rob_init(ROB *rob, int _size){
	rob->start_idx = 0;
	rob->end_idx = 0;
	rob->size = 0;
	rob->capacity = _size;
}

void rob_insert_inst(ROB *rob, Inst *inst){
	rob->entries[rob->end_idx].inst = inst;
	inst->rob_idx = rob->end_idx;
	rob->end_idx = (rob->end_idx+1)%rob->capacity;	
	rob->size++;
//	printf("%d\n", rob->size);
}
void rob_remove_inst(ROB *rob){
	rob->start_idx = (rob->start_idx+1)%rob->capacity;
	rob->size--;
}

Inst* rob_get_inst(ROB *rob, int idx){
	return rob->entries[idx].inst;
}

Inst* rob_get_oldest_inst(ROB *rob){
	return rob->entries[rob->start_idx].inst;
}

bool is_rob_full(ROB *rob){
	return (rob->size == rob->capacity);
}

bool is_rob_empty(ROB *rob){
	return (rob->size == 0);
}
