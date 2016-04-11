#ifndef __TRACE_ANALYZER_THREAD_IF_H
#define __TRACE_ANALYZER_THREAD_IF_H

#include "inst_queue.h"

typedef struct _AnalysisArg {
	InstQueue *trace_buffer;
	uint64_t cycle;
	int bpred_penalty;
} AnalysisArg;

void trace_analysis_wrapper(void *data);
void trace_analyzer_thread_start(InstQueue *trace_buffer);
void trace_analyzer_thread_end(uint64_t *cycle, int *bpred_penalty);


#endif


