#include <time.h>
#include <locale.h>
#include <stdlib.h>
#include <unistd.h>
#include "trace_analyzer.h"
#include "fu.h"
#include "../config.h"

#define STRING_BUFFER_SIZE 255
#define NUM_INST_TYPE 10
#define NUM_FU 7

/* constant */

static const int rob_size = 60;
static const int num_simple_alu = 2;


static const int dispatch_width = 6;
static const int issue_width = 8;
static const int commit_width = 8;
static const int front_pipeline_depth = 6;
//12;

static int latency_l1_hit;
static int latency_l1_miss;
static int latency_l2_miss;
static const bool verbose = false; 
//false; //false;

static int rob_full_event = 0;
static int iq_full_event = 0;
static int ldq_access_event = 0;
static int ldq_full_event = 0;
static int stq_full_event = 0;
static int dl1_outstanding_request = 0;
static int ul2_outstanding_request = 0;

static InstQueue IQ;
static InstQueue ROB;
static InstQueue LDQ;
static InstQueue STQ;
static InstQueue FUBuffer;

/* component */

static FU fu[NUM_FU];
static InstType inst_type[NUM_INST_TYPE];
static char strbuf[STRING_BUFFER_SIZE];
static FUType inst_type_fu_type_map[NUM_INST_TYPE];
static int num_inst_type[NUM_INST_TYPE];
FILE *fp;

SimType sim_type = SimpleMode;
//SimType sim_type = DetailedMode;
/* statistics */

static  int num_fetched_insts = 0;
static  int num_committed_insts = 0;
static  bool mispred_path = false;
static  int num_mispred_insts = 0;
static  double avg_mispred_penalty = 0;
static  uint64_t cur_cycle = 0;
static  int num_il1_miss = 0;
static  int num_dl1_miss = 0;
static  int num_l2_miss = 0;
static  Inst *reserved_inst = NULL;
static  int inst_cache_miss_penalty = 0;
static  int mispred_penalty = 0;

void init_all_inst_type_fu_type_map(void){
	inst_type_fu_type_map[IntAlu] = SimpleALU;
	inst_type_fu_type_map[IntMult] = ComplexALU;
	inst_type_fu_type_map[IntDiv] = ComplexALU;
	inst_type_fu_type_map[SimdFloatAdd] = FPUnit;
	inst_type_fu_type_map[SimdFloatMult] = FPUnit;
	inst_type_fu_type_map[SimdFloatMultAcc] = FPUnit;
	inst_type_fu_type_map[MemRead] = LoadUnit;
	inst_type_fu_type_map[MemWrite] = StoreUnit;
	inst_type_fu_type_map[Branch] = SimpleALU;
	inst_type_fu_type_map[Syscall] = EtcUnit;
	
}

void init_all_inst_type(void){

	init_inst_type (&(inst_type[IntAlu]), IntAlu, 1, true);
    init_inst_type (&(inst_type[IntMult]), IntMult, 3, true);
    init_inst_type (&(inst_type[IntDiv]), IntDiv, 12, false);
    init_inst_type (&(inst_type[SimdFloatAdd]), SimdFloatAdd, 10, true);
    init_inst_type (&(inst_type[SimdFloatMult]), SimdFloatMult,12, true);
    init_inst_type (&(inst_type[SimdFloatMultAcc]), SimdFloatMultAcc, 20, true);
    init_inst_type (&(inst_type[MemRead]), MemRead, 2, true);
    init_inst_type (&(inst_type[MemWrite]), MemWrite, 3, true);
    init_inst_type (&(inst_type[Branch]), Branch, 1, true);
    init_inst_type (&(inst_type[Syscall]), Syscall, 1, true);
}

void init_all_queues(void){
	if (sim_type == DetailedMode){
		init_inst_queue (&ROB, rob_size);
		init_inst_queue (&IQ, 48);
		init_inst_queue (&LDQ, 16);
		init_inst_queue (&STQ, 16);
		init_inst_queue (&FUBuffer, 0);  
	}
	else {
		init_inst_queue (&ROB, rob_size);
		init_inst_queue (&IQ, 48);
		init_inst_queue (&LDQ, 16);
		init_inst_queue (&STQ, 16);
		init_inst_queue (&FUBuffer, 0);  
	}
}

void init_all_FUs(void){
    init_FU (&(fu[SimpleALU]), SimpleALU, num_simple_alu);
    init_FU (&(fu[ComplexALU]), ComplexALU, 1);
	init_FU (&(fu[FPUnit]), FPUnit, 2);
	init_FU (&(fu[LoadUnit]), LoadUnit, 1);
    init_FU (&(fu[StoreUnit]), StoreUnit, 1);
	init_FU (&(fu[BranchUnit]), BranchUnit, 1);
	init_FU (&(fu[EtcUnit]), EtcUnit, 1);
}

void reset_all_FUs(void){
    int i=0 ;
    for (i=0 ; i<NUM_FU ; i++){
        reset_FU(&(fu[i]));
    }
}



void init_trace_analyzer(void){

	latency_l1_hit = perfmodel.il1.latency;
	latency_l1_miss = perfmodel.ul2.latency + perfmodel.il1.latency *2;
	latency_l2_miss = perfmodel.mem.latency + perfmodel.ul2.latency *2;
	init_trace_statistics();	
	init_inst_elem_pool();
	init_all_queues();
	init_all_FUs();
	init_all_inst_type();
	init_all_inst_type_fu_type_map();
}

static int num_dispatched_insts= 0;
void init_trace_statistics(void){
	int i=0;
	for (i=0 ; i< NUM_INST_TYPE ; i++){
		num_inst_type[i] = 0;
	}


	num_fetched_insts = 0;
	num_committed_insts = 0;
	num_dispatched_insts = 0;
	mispred_path = false;
	num_mispred_insts = 0;
	avg_mispred_penalty = 0;
	cur_cycle = 0;
	num_il1_miss = 0;
	num_dl1_miss = 0;
	num_l2_miss = 0;
	reserved_inst = NULL;
	inst_cache_miss_penalty = 0;
	mispred_penalty = 0;
	dl1_outstanding_request = 0;
	ul2_outstanding_request = 0;
	rob_full_event = 0;
	iq_full_event = 0;
	ldq_full_event = 0;
	stq_full_event = 0;
	ldq_access_event = 0;
}

void destroy_trace_analyzer(void){
    destroy_all_inst_elem_from_queue (&ROB); 
	destroy_all_inst_elem_from_queue (&IQ);
	destroy_all_inst_elem_from_queue (&LDQ);
    destroy_all_inst_elem_from_queue (&STQ);
	destroy_all_inst_elem_from_queue (&FUBuffer);
	destroy_inst_elem_pool();
}



void dispatch_inst(Inst *inst){
	inst->state = Dispatched;
    if (verbose)
        printf("%s dispatched\n", print_inst(strbuf, inst));
}

void dispatch_insts(InstQueue *trace_buffer){
    int w = dispatch_width;
    while (w>0){
		InstElem *inst_elem = front_inst_elem(trace_buffer);
		if (!inst_elem){
			break;
		}
/*	
		if (sim_type == DetailedMode){
			if (inst_elem->inst->inst_cache_miss){
				if (inst_elem->inst->inst_cache_miss == L1Miss){
					num_il1_miss++;
					inst_cache_miss_penalty = latency_l1_miss + front_pipeline_depth;
				}
				else if (inst_elem->inst->inst_cache_miss == L2Miss){
					num_il1_miss++;
					num_l2_miss++;
					inst_cache_miss_penalty = latency_l2_miss + front_pipeline_depth;
				}
				inst_elem->inst->inst_cache_miss = 0;
				break;
			}
		}
*/
        if (is_full_queue(&ROB)){
			rob_full_event++;
			break;
		}
		else if  (is_full_queue(&IQ)){
			iq_full_event++;
            break;      //dispatch is impossible
        }
		else if (inst_elem->inst->type == MemRead){
			ldq_access_event++;
			if (is_full_queue(&LDQ)){
				ldq_full_event++;
				break;
			}
		}
		else if (inst_elem->inst->type == MemWrite && is_full_queue(&STQ)){
			stq_full_event++;
			break;
		}

		Inst *inst = pop_front_inst(trace_buffer);
		
		if (inst->type == Unsupported){
			num_dispatched_insts++;
			num_committed_insts++;
			free(inst);
			continue;
		}


		InstElem *inst_elem_for_rob = push_back_inst(&ROB, inst);
		InstElem *inst_elem_for_iq = push_back_inst(&IQ, inst);
		
		inst->rob_elem = inst_elem_for_rob;
		inst->iq_elem = inst_elem_for_iq;

		if (inst->type == MemRead){
			InstElem *inst_elem_for_ldq = push_back_inst(&LDQ, inst);
			inst->ldq_elem = inst_elem_for_ldq;
		}
		if(inst->type == MemWrite){
			InstElem *inst_elem_for_stq = push_back_inst(&STQ, inst);
			inst->stq_elem = inst_elem_for_stq;
		}
		num_dispatched_insts++;
		dispatch_inst(inst);
		
		inst->dispatched_cycle = cur_cycle;
		w--;

		if (sim_type == DetailedMode){
			if (inst->type == Branch && inst->bpred_mispred){
				mispred_path = true;
				break;
			}
		}

	}
}

Inst* has_mem_dependency(Inst *inst){
	InstElem *inst_elem =  inst->rob_elem;
	while (inst_elem){
		Inst *w_inst = inst_elem->inst;
		if (is_mem_dependent(w_inst, inst)){
			return w_inst;
		}
		inst_elem = inst_elem->prev_inst_elem;
	}
	return NULL;
}


bool meet_regs_dependency(Inst *inst){
	InstElem *inst_elem = inst->rob_elem;
	InstElem *prev_inst_elem = inst_elem->prev_inst_elem;

	if (inst->reg_depend_inst){
		if (inst->reg_depend_inst->state != Finished && (inst->reg_depend_inst->state == Running && inst->reg_depend_inst->remaining_latency > 1)){
			return false;
		}
		else {
			inst->reg_depend_inst = NULL;
		}
	}

	while (prev_inst_elem){
		Inst *p_inst =prev_inst_elem->inst;
		if (is_reg_dependent(p_inst, inst) && p_inst->state != Finished  && (p_inst->state == Running && p_inst->remaining_latency > 1)){
			inst->reg_depend_inst = p_inst;
			return false;	
		}
		prev_inst_elem = prev_inst_elem->prev_inst_elem;
	}
	return true;
}


//out-of-order
void issue_insts(void){
     int w = issue_width;

	 if (is_empty_queue(&IQ)){
		return;		 
	 }
     //move instruction from IQ/ to FU
	 InstElem *inst_elem = front_inst_elem(&IQ);
	 InstElem *next_inst_elem = NULL;
	 while (w>0){
		 if (inst_elem){
			 next_inst_elem = inst_elem->next_inst_elem;
			 if (inst_elem->inst->state == Dispatched){
				 if (meet_regs_dependency(inst_elem->inst)){
					 FUType target_fu = inst_type_fu_type_map[inst_elem->inst->type];
					 bool success = try_FU(&fu[target_fu]);
					 if (success){
						 Inst *inst = inst_elem->inst;
						 remove_from_list(&IQ, inst_elem, true);
						 inst->state = Running;
						 int latency =  inst_type[inst->type].latency;
						 //assuming infinite write buffer

						 if (inst->type == MemWrite){
							// latency += latency_l1_hit;
						 }
						 if (inst->type == MemRead){
							 switch(inst->cache_miss){
								 case L1Hit:
								//	 latency += latency_l1_hit; // * inst->num_eff_addr;
									 break;
								 case L1Miss:
									 num_dl1_miss++;
									 latency += latency_l1_miss; 
									 break;

								 case L2Miss:
									 num_dl1_miss++;
									 num_l2_miss++;
									 if (sim_type == DetailedMode)
										 latency += latency_l2_miss; 
									 else 
										 latency += latency_l1_miss;
									 break;

								 default:
									 fprintf(stderr, "Unsupported Cache Miss Type %d \n", inst_elem->inst->cache_miss);
									 exit(-1);
							 }
						 }
						 inst->remaining_latency = latency;	
						 push_back_inst(&FUBuffer, inst);
						 if (verbose)
							 printf("%s Running (latency %d) \n", print_inst(strbuf, inst), inst->remaining_latency);
					 }
				 }
				 else {
					 if (verbose)
						 printf("%s Waiting for dependency\n", print_inst(strbuf, inst_elem->inst));

				 }
			 }
			 w--;
		 }
		 else {
			 break;
		 }
		 inst_elem = next_inst_elem;
	 }

}

void execute_insts(void){
    //proceed instructions in the executing state
    InstElem *inst_elem = FUBuffer.oldest_inst_elem;
    while (inst_elem){
            InstElem *next_inst_elem = inst_elem->next_inst_elem;
            execute_inst(inst_elem->inst);
            if (inst_elem->inst->state == Finished){
                remove_from_list(&FUBuffer, inst_elem, true);
            }
            inst_elem = next_inst_elem;
    }
}

//precondition : inst is about to be executed
void cache_access_reordering(Inst *inst){

	//search rob.
	InstElem *earlier_inst_elem = inst->rob_elem->prev_inst_elem;
	while (earlier_inst_elem){
		Inst *p_inst = earlier_inst_elem->inst;
		if ( p_inst->state < Running && inst->state == Running &&  
			is_mem_dependent (p_inst, inst) && 
			inst->cache_miss < p_inst->cache_miss){
			CacheMissType tmp = inst->cache_miss;
			inst->cache_miss = p_inst->cache_miss;
			p_inst->cache_miss = tmp;
			break;
		}
		earlier_inst_elem = earlier_inst_elem->prev_inst_elem;
	}
}

bool is_executable_mem_inst(Inst *inst){

	if (has_mem_dependency(inst)){
		return false;
	}

	if (!inst->cache_miss){
		return true;
	}
	else {
		return true;
	}
}

void execute_inst(Inst *inst){

	if (inst->type == MemRead || inst->type == MemWrite){
		if (!is_executable_mem_inst(inst)){
			return;
		}
	}

	inst->remaining_latency--;

	if (inst->remaining_latency <= 0){
		inst->state = Finished;
		if (verbose)
			printf("%s Finished\n", print_inst(strbuf, inst));
	}
	if (inst->remaining_latency <= 1 &&  inst->type == Branch && inst->bpred_mispred){
		double sum_mispred_penalty = avg_mispred_penalty * num_mispred_insts + (front_pipeline_depth+ cur_cycle - inst->dispatched_cycle);
		num_mispred_insts++;
		avg_mispred_penalty = sum_mispred_penalty /num_mispred_insts;
		mispred_path = false;
	}


}

//finished stage -> committed stage
void commit_insts(void){
	int w = commit_width;

	while (w>0){
		InstElem *inst_elem = ROB.oldest_inst_elem;
		if (inst_elem){
			if (inst_elem->inst->state == Finished){				
				if (verbose){
					printf("%s Committed\n", print_inst(strbuf, inst_elem->inst));
				}
				if (inst_elem->inst->ldq_elem){
					remove_from_list(&LDQ, inst_elem->inst->ldq_elem, true);
				}
				if (inst_elem->inst->stq_elem){
					remove_from_list(&STQ, inst_elem->inst->stq_elem, true);
				}

				Inst *inst = pop_front_inst(&ROB);
				num_inst_type[inst->type]++;
				free(inst);
				num_committed_insts++;
				w--;
			}
			else{
				break;
			}
		}
		else{
			break;
		}
	}
}

void print_buffer_status(void){
	//	printf("Remaining Trace Buffer = %d\n", get_current_queue_size(&TraceBuffer));
	
	printf("Remaining IQ = %d\n", get_current_queue_size(&IQ));
	printf("Remaining ROB = %d\n", get_current_queue_size(&ROB));
	printf("Remaining FUBuffer = %d\n", get_current_queue_size(&FUBuffer));
	printf("Remaining LDQ = %d\n", get_current_queue_size(&LDQ));
	printf("Remaining STQ = %d\n", get_current_queue_size(&STQ));
}

void trace_analysis(InstQueue *trace_buffer, int max_cycle, uint64_t* last_cycle, int *bpred_penalty){
	int cycle = 0;
	init_trace_statistics();
	num_fetched_insts = get_current_queue_size(trace_buffer);
	while (true){
		if (max_cycle && cycle >= max_cycle){
			break;
		}
		commit_insts();
		execute_insts();
		issue_insts();

		if (inst_cache_miss_penalty){
			inst_cache_miss_penalty--;
		}
		if (mispred_penalty){
			mispred_penalty--;
		}

		 if (!inst_cache_miss_penalty && !mispred_penalty && !mispred_path)
			dispatch_insts(trace_buffer);
		reset_all_FUs();
		cycle++;
		cur_cycle++;

		if (num_committed_insts == num_fetched_insts){
			break;
		}
	}
	*last_cycle = cur_cycle;
	*bpred_penalty = avg_mispred_penalty;

}
