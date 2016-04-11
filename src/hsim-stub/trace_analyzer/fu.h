#ifndef FU_H_INCLUDED
#define FU_H_INCLUDED

#include "common.h"

typedef enum _FUType {
    SimpleALU, ComplexALU, FPUnit, LoadUnit, StoreUnit, BranchUnit,EtcUnit
} FUType;

typedef struct _FU {
	FUType type;
	int capacity;
	int num_issue;
	int num_active;
	int num_stage;
	int num_access;
	int num_available;
	int num_use;
} FU;

void init_FU(FU *fu, FUType type, int capacity, int num_stage);
void reset_FU(FU *fu);
void reflesh_FU(FU *fu);
void release_FU(FU *fu);
bool try_FU(FU *fu);
double get_available_rate_FU(FU *fu);
#endif // FU_H_INCLUDED
