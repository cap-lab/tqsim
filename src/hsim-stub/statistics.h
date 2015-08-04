struct SimStat{
	int numIntInsts ;
	int numMemInsts ;
	int numIntMultInsts ;
	int numIntDivInsts ;
	int numBranchInsts ;
	int numSyscallInsts ;
	int numCoprocInsts ;
	int numEtcInsts;
};

extern struct SimStat simStat;

void initStatistics(void);
void printStatistics(void);
