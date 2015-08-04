#include "config-host.h"
#include "perfmodel.h"
#include "cachesim.h"
#include "bpredsim.h"
#include "config.h"
#include "statistics.h"
#include "depanal.h"

#include <stdio.h>

//static FILE *perfmodel_log;

double cpi;
uint64_t num_insts;
uint32_t bpred_penalty;
char *command;
clock_t start_time, end_time;


void perfmodel_initialize(int argc, char ** argv){

	int i=0;
	num_insts = 0;
#ifdef CONFIG_HSIM
//	printf("loading...hsim_arch_config_file...\n");
	load_configfile(getenv("HSIM_ARCH_CONFIG_FILE"));
#else
//	printf("loading....arch_config_file...\n");
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
	cpi = perfmodel.sample.initial_cpi;

	cachesim_initialize();
	bpredsim_initialize();
}

uint64_t perfmodel_getCycle(void){
	return num_insts * cpi + cachesim_il1Penalty() + cachesim_ul2Penalty() + bpredsim_penaltysum();


}

uint64_t perfmodel_getSimpleCycle(void){
	return num_insts * cpi + cachesim_il1Penalty() + bpredsim_penaltysum();
}


void perfmodel_logwrite(char* str){
	//fprintf(perfmodel_log, "%s\n", str);
}

void perfmodel_logging(void){
	//open the file
	//
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
	fprintf(fp, "effective D = %f |",  cpi);
	fprintf(fp, "bpred mispenalty = %" PRIu32 "\n", bpred_penalty);
	fclose(fp);

}

void perfmodel_end(void){
	free(command);
	cachesim_end();
	bpredsim_end();
	//fclose(perfmodel_log);
}
