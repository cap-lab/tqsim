#ifndef STATISTICS_H
#define STATISTICS_H

#include "statistics.h"
#include <stdio.h>

struct SimStat simStat;

void initStatistics(void){
	simStat.numIntInsts = 0;
	simStat.numMemInsts = 0;
	simStat.numIntMultInsts =0;
	simStat.numIntDivInsts = 0;
	simStat.numBranchInsts = 0;
	simStat.numSyscallInsts = 0;
	simStat.numCoprocInsts = 0;
	simStat.numEtcInsts= 0;
}

void printStatistics(void){

	printf("# int insts = %d\n", simStat.numIntInsts);
	printf("# Mem insts = %d\n", simStat.numMemInsts);
	printf("# int mult insts = %d\n", simStat.numIntMultInsts);
	printf("# int div insts = %d\n", simStat.numIntDivInsts);
	printf("# branch insts = %d\n", simStat.numBranchInsts);
	printf("# coproc insts = %d\n", simStat.numCoprocInsts);
	printf("# syscall insts = %d\n", simStat.numSyscallInsts);
	printf("# etc insts = %d\n", simStat.numEtcInsts);
	


}

#endif
