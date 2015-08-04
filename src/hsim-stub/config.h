#define CONFIG_TIMING

#ifndef CONFIG_HSIM
#define CONFIG_SAMPLING
#endif

#ifndef PERFMODEL_CONFIG_H
#define PERFMODEL_CONFIG_H

struct sample_config {
	int length;
	int default_period;
	double initial_cpi;
};

struct core_config {
	int dispatch_width;
	int issue_width;
	int window_size;
};

struct numfu_config{
	int intalu;
	int intmultdiv;
	int mem_rwport;
};

struct latency_config{
	int intalu;
	int branch;
	int intmult;
	int intdiv;
	int mem;
	int coproc;

};

struct etc_config{
	int syscall_penalty;
};

struct cache_config {
	int perfect;
	int num_set;
	int size_blk;
	int assoc;
	char replacement_policy[8];	
	int latency;
};

struct memory_config{
	int latency;
};

struct bpred_config{
	int bimod_table_size;
	int mispredict_penalty;
};

struct perfmodel_config {
	struct sample_config sample;
	struct core_config core;
	struct numfu_config numfu;
	struct latency_config latency;
	struct etc_config etc;
	struct cache_config dl1;
	struct cache_config il1;
	struct cache_config ul2;
	struct memory_config mem;
	struct bpred_config bpred;
	
	int existDL1;
	int existIL1;
	int existUL2;
	

};

int load_configfile(const char* filename);
void init(void);

extern struct perfmodel_config perfmodel;



#endif
