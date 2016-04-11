#include "buffer_manager.h"
#include "trace_analyzer.h"
#include "trace_analyzer_thread_if.h"


void seq_main(char* filename);
void pa_main(char* filename);
int main(int argc, char** argv);


void pa_main(char* filename){

	uint64_t total_cycle = 0;	
	uint64_t cycle = 0;
	int bpred_penalty = 0;
	int sum_bpred_penalty = 0;
	int num_analysis = 0;

	InstQueue TraceBuffer;

	init_trace_file(filename);
	init_trace_buffer(&TraceBuffer);

	while (fill_trace_buffer_from_file(&TraceBuffer)){
		trace_analyzer_thread_start(&TraceBuffer);
		trace_analyzer_thread_end(&cycle, &bpred_penalty);
		total_cycle += cycle;
		sum_bpred_penalty += bpred_penalty;
		num_analysis++;

	}

	destroy_trace_buffer(&TraceBuffer);
	destroy_trace_file();
	destroy_trace_analyzer();

	printf("%" PRIu64 "cycles / %d cycles\n", total_cycle, sum_bpred_penalty/num_analysis);


}


void seq_main(char* filename){
	uint64_t total_cycle = 0;	
	uint64_t cycle = 0;
	int bpred_penalty = 0;
	int sum_bpred_penalty = 0;
	int num_analysis = 0;

	InstQueue TraceBuffer;

	init_trace_file(filename);
	init_trace_buffer(&TraceBuffer);

	while (fill_trace_buffer_from_file(&TraceBuffer)){
		trace_analysis(&TraceBuffer, 0, &cycle, &bpred_penalty);
		total_cycle += cycle;
		sum_bpred_penalty += bpred_penalty;
		num_analysis++;
	}

	destroy_trace_buffer(&TraceBuffer);
	destroy_trace_file();
	destroy_trace_analyzer();

	printf("%" PRIu64 "cycles / %d cycles\n", total_cycle, sum_bpred_penalty/num_analysis);

}


int main(int argc, char** argv)
{
	if (argc == 2){
		seq_main(argv[1]);
	}
	else if (argc == 3){
		pa_main(argv[1]);
	}
	else {
		printf("Usage: ta [filename]\n");
	}
	return 0;
}
