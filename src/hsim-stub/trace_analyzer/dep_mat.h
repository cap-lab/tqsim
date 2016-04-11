#ifndef __DEPENDENCY_MAT_H
#define __DEPENDENCY_MAT_H

#include "inst.h"
#include "rob.h"

extern bool dep_mat[60][60];
bool in_order(int start, int x, int y);
void init_dependency_mat(int rob_size);
bool is_dependent(ROB *rob, Inst *inst_a, Inst *inst_b);
void set_dependency(ROB *rob, Inst *inst_a, Inst *inst_b);
void clean_dependency(ROB *rob, Inst *inst_b);

#endif
