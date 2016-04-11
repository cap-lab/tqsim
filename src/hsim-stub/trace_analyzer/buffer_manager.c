#include "trace_analyzer.h"
#include "buffer_manager.h"

static FILE *fp = NULL;
static Inst* on_the_fly_inst = NULL;




void init_trace_buffer(InstQueue *trace_buffer){
	init_inst_queue (trace_buffer, 1000001);
}


void destroy_trace_buffer(InstQueue *trace_buffer){
	destroy_all_inst_elem_from_queue (trace_buffer);
}


void init_trace_file(char* filename, const char *mode ){
	fp = fopen(filename, mode);
}

void destroy_trace_file(void){
	fclose(fp);
}


bool push_code(InstQueue *trace_buffer, unsigned int seq, uint32_t pc, unsigned int code, int miss_type){
	Inst *inst = (Inst*)malloc(sizeof(Inst));
	init_inst(inst, seq, pc, code);
	inst->inst_cache_miss = miss_type;
//	decode_inst(inst);

	if (fp){
		fprintf(fp, "c %X %X %X %d\n", seq, pc, code, miss_type);
	}

	if (is_full_queue(trace_buffer)){
		on_the_fly_inst = inst;
		return false;
	}
	else {
		push_back_inst(trace_buffer, inst);
		return true;
	}
}

bool push_bpred(InstQueue *trace_buffer, bool correct){

	if (fp){
		fprintf(fp, "b %d\n", correct);
	}

	Inst *inst = trace_buffer->newest_inst_elem->inst;
	if (inst != NULL){
//		inst->type = Branch;
		set_inst_bpred_mispred(inst, !correct);
		return true;
	}
	else {
		return false;
	}
}

bool push_memaddr(InstQueue *trace_buffer, uint32_t addr, int miss_type){

	if (fp){
		fprintf(fp, "a %X %d\n", addr, miss_type);
	}


	Inst *inst = trace_buffer->newest_inst_elem->inst;

	if (inst != NULL)
	{
		set_inst_eff_addr(inst, addr, miss_type);
		return true;
	}
	else {
		return false;
	}

}



/* for test purpose */
int fill_trace_buffer_from_file (InstQueue *trace_buffer){

    char line[256];

	if (on_the_fly_inst){
		push_back_inst(trace_buffer, on_the_fly_inst);
		on_the_fly_inst = NULL;

	}
    
	while (fgets(line, sizeof(line), fp)) {
        char *ptr;
        ptr = strtok(line, " ");
        if (ptr != NULL){
                if (ptr[0] == 'c'){
                    ptr = strtok(NULL, " ");
                    unsigned int seq = strtoul(ptr, NULL, 16);
                    ptr = strtok(NULL, " ");
                    uint32_t pc = strtoul(ptr, NULL, 16);
                    ptr = strtok(NULL, " ");
                    uint32_t code = strtoul(ptr, NULL, 16);
					ptr = strtok(NULL, " ");
                    int miss_type = strtoul(ptr, NULL, 10);
					
					if (!push_code(trace_buffer, seq, pc, code, miss_type)){
						break;
					}

                }
                else if (ptr[0] == 'b'){
                    ptr = strtok(NULL, " ");
                    unsigned int correct = strtoul(ptr, NULL, 10);
					push_bpred(trace_buffer, correct);
                }
                else if (ptr[0] == 'a'){
                    ptr = strtok(NULL, " ");
                    uint32_t addr = strtoul(ptr, NULL, 16);
					ptr = strtok(NULL, " ");
					strtok(NULL, " ");
					int miss_type = strtoul(ptr, NULL, 10);
					push_memaddr(trace_buffer, addr, miss_type);

                }
        }
    }
	return get_current_queue_size(trace_buffer);
}


