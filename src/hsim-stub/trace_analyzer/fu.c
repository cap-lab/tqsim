#include "fu.h"

static const int infinite_mode = false;

void init_FU(FU *fu, FUType type, int capacity, int num_stage){
    fu->type = type;
    fu->capacity = capacity;
	fu->num_active = 0;
	fu->num_issue = 0;
	fu->num_access = 0;
	fu->num_available = 0;
	fu->num_stage = num_stage;
	fu->num_use = 0;
//	fu->num_stage = 1;
}

double get_available_rate_FU(FU *fu){
	return (double)fu->num_available / fu->num_access;
}


void release_FU(FU *fu){
	fu->num_active--;
}

void reset_FU(FU *fu){
	fu->num_active = 0;
	fu->num_issue = 0;
	fu->num_access = 0;
	fu->num_available = 0;
	fu->num_use = 0;

}

void reflesh_FU(FU *fu){
	fu->num_issue = 0;
}



bool try_FU(FU *fu){
	fu->num_access++;
	
    if (infinite_mode || (fu->num_issue < fu->capacity && fu->num_active < fu->num_stage)){
        fu->num_issue++;		//this cycle
		fu->num_active++;		//
		fu->num_available++;	//statistics
		fu->num_use++;
        return true;
    }
    else{
        return false;
    }
}
