#include "config-host.h"
#include "perfmodel.h"
#include "cachesim.h"
#include "bpredsim.h"
#include "config.h"
#include "statistics.h"

#include <stdio.h>
#include "trace_analyzer/trace_analyzer.h"

//static FILE *perfmodel_log;

double sum_effective_dispatch_width;
int sum_bpred_penalty;
int num_sampling;
int num_total_branch;

uint64_t num_insts;
double effective_dispatch_width;
uint32_t bpred_penalty;

char *command;
clock_t start_time, end_time;

void perfmodel_initialize(int argc, char ** argv){

	int i=0;
	num_insts = 0;
#ifdef CONFIG_HSIM
	load_configfile(getenv("HSIM_ARCH_CONFIG_FILE"));
#else
	load_configfile(getenv("ARCH_CONFIG_FILE"));
#endif

	start_time = clock();
	int command_length = 0;

	for (i=0  ; i<argc ; i++){
		command_length += strlen(argv[i])+1;
	}
	
	command = malloc(sizeof(char)*command_length);
	

	for (i=1  ; i<argc ; i++){
		char buf[command_length];
		sprintf(buf, "%s ", argv[i]);
		if (i==1) strcpy(command, buf);
		else strcat(command, buf);
	}

	bpred_penalty = perfmodel.bpred.mispredict_penalty;
	effective_dispatch_width = perfmodel.sample.initial_effective_dispatch_width;

	cachesim_initialize();
	bpredsim_initialize();
}

uint64_t perfmodel_getCycle(void){
		return num_insts / effective_dispatch_width  	
			+ cachesim_il1_num_miss() * cachesim_il1_miss_latency() 
			+ cachesim_ul2_num_miss() * cachesim_ul2_miss_latency() 
			+ bpredsim_num_mispred()* bpred_penalty;
}

uint64_t perfmodel_getSimpleCycle(void)
{
	return num_insts / effective_dispatch_width  
		+ cachesim_il1_num_miss() * cachesim_il1_miss_latency() 
		+ bpredsim_num_mispred()* bpred_penalty;
}


typedef struct _SampledStat{
	int num_insts; 
	int num_il1_miss;
	int num_dl1_miss;
	int num_ul2_miss;
	int num_mispred;

} SampledStat;

SampledStat sample_stat;

double sum_ipc = 0;
int num_ipc = 0;
static int num_file = 0;
static int enable_save_files = 0;
void perfmodel_sample_start(void){
	sample_stat.num_insts = num_insts;
	sample_stat.num_il1_miss = cachesim_il1_num_miss();
	sample_stat.num_dl1_miss = cachesim_dl1_num_miss();
	sample_stat.num_ul2_miss = cachesim_ul2_num_miss();
	sample_stat.num_mispred = bpredsim_num_mispred();

	if (enable_save_files){
		char strbuf[255];
		sprintf(strbuf, "tracefile%d", num_file++);
		init_trace_file(strbuf, "w");	
	}
}

void perfmodel_sample_end(void){

	sample_stat.num_insts = num_insts -  sample_stat.num_insts;
	sample_stat.num_il1_miss = cachesim_il1_num_miss() - sample_stat.num_il1_miss;
	sample_stat.num_ul2_miss = cachesim_ul2_num_miss() - sample_stat.num_ul2_miss;
	sample_stat.num_mispred = bpredsim_num_mispred() - sample_stat.num_mispred;
	if (enable_save_files){
		destroy_trace_file();
	}

}

void perfmodel_update(uint64_t _cycle, int _bpred_penalty){

	int l_num_insts =   sample_stat.num_insts;
	int l_num_mispred =  sample_stat.num_mispred;


	int penalty_sum = 0;
	double D = (double)l_num_insts / (_cycle);
	//printf("%f\n", D);

	sum_ipc += D;
	num_ipc ++;

	if (_bpred_penalty != 0){
		sum_bpred_penalty += l_num_mispred * _bpred_penalty;
		num_total_branch  += l_num_mispred;
	}

	sum_effective_dispatch_width += 1.0/D;
	num_sampling++;

	effective_dispatch_width =  num_sampling / sum_effective_dispatch_width;
	if (num_total_branch)
		bpred_penalty = sum_bpred_penalty / num_total_branch;


}

void perfmodel_logging(void){
	FILE *fp = fopen("/home/shkang/tqemu_experiment", "a");
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	uint64_t estimated_cycle =  perfmodel_getCycle();

	end_time = clock();

	fprintf(fp, "%d-%d-%d %d:%d:%d | ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	fprintf(fp, "%s |", command);
	fprintf(fp, "%f ms |", (end_time-start_time)/(double)(CLOCKS_PER_SEC/1000));
	fprintf(fp, "%d/%d |", perfmodel.sample.length, perfmodel.sample.default_period);
	fprintf(fp, "num_cycle = %" PRIu64 " | ", estimated_cycle);
	fprintf(fp, "effective D = %f |",  effective_dispatch_width);
	fprintf(fp, "bpred mispenalty = %" PRIu32 "\n", bpred_penalty);
	fclose(fp);

}

void perfmodel_end(void){
//	printf("Final IPC : %f (trace_analyzer) %f \n", (double)sum_ipc / num_ipc, (double)num_insts / perfmodel_getCycle());
	perfmodel_logging();
	free(command);
	cachesim_end();
	bpredsim_end();
	//fclose(perfmodel_log);
}
