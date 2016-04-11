#include <pthread.h>

#include "trace_analyzer.h"
#include "trace_analyzer_thread_if.h"

static int num_thread = 0;
static pthread_t thread_inst;
AnalysisArg arg;

void trace_analysis_wrapper(void *data){
	AnalysisArg *args = (AnalysisArg*)data;
	trace_analysis(args->trace_buffer, 0, &(args->cycle), &(args->bpred_penalty));
}


void trace_analyzer_thread_start(InstQueue *trace_buffer){
	num_thread++;
	arg.trace_buffer = trace_buffer;
	pthread_create(&thread_inst, NULL, (void *) &trace_analysis_wrapper, &arg);
}

void trace_analyzer_thread_end(uint64_t *cycle, int *bpred_penalty){
	if (num_thread){
		pthread_join(thread_inst, (void **)NULL);
		*cycle = arg.cycle;
		*bpred_penalty = arg.bpred_penalty;
  		num_thread--;
	}
}




