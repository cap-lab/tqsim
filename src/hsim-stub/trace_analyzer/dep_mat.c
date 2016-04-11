#include "dep_mat.h"

bool dep_mat[60][60];

bool in_order(int start, int x, int y){
	if (start <=x && start<=y && x<y){
		return true;
	}

	else if (x<start && y<start && x<y){
		return true;
	}
	else if (start<=y && x<start){
		return true;
	}	
	else {
		return false;
	}
}


void init_dependency_mat(int rob_size){

}
bool is_dependent(ROB *rob, Inst *inst_a, Inst *inst_b){
	int idx_a = inst_a->rob_idx;
	int idx_b = inst_b->rob_idx;

	if (in_order(rob->start_idx, idx_a, idx_b)){
		return dep_mat[idx_b][idx_a];
	}
	return false;
}
void set_dependency(ROB *rob, Inst *inst_a, Inst *inst_b){
	dep_mat[inst_b->rob_idx][inst_a->rob_idx]= 1;	
}
void clean_dependency(ROB *rob, Inst *inst_b){
	memset(dep_mat[inst_b->rob_idx], 0, sizeof(bool)*rob->capacity);
}


