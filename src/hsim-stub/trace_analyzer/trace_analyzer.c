#include <time.h>
#include <locale.h>
#include <stdlib.h>
#include <unistd.h>
#include "trace_analyzer.h"
#include "fu.h"
#include "dep_mat.h"
#include "rob.h"
#include "../config.h"

#define STRING_BUFFER_SIZE 255
#define NUM_INST_TYPE 13
#define NUM_FU 7

/* constant */

static const int mem_dependency_check = 1;
static const int reg_dependency_check = 1;
static const int queue_availablity_check = 1;
static const int default_wb_latency = 2;		//after issue stage

static int dispatch_width = 3;
static int issue_width = 8;
static int wb_width = 8;
static int commit_width = 8;
static const int front_pipeline_depth = 12;
static int rob_capacity = 60;

//12;

static int latency_l1_hit;
static int latency_l1_miss;
static int latency_l2_miss;
static bool verbose =false;
static bool verbose_pipeline = false;
static uint64_t first_issue_cycle = 0;


static int rob_full_event = 0;
static int rob_access_event = 0;
static int iq_full_event = 0;
static int ldq_access_event = 0;
static int ldq_full_event = 0;
static int stq_full_event = 0;
static int dl1_outstanding_request = 0;
static int ul2_outstanding_request = 0;

static InstQueue IQ;
static ROB rob;
static InstQueue LDQ;
static InstQueue STQ;
static InstQueue FUBuffer;

/* component */

static int num_dispatched_insts_cycle = 0;
static int num_issued_insts_cycle = 0;
static int num_executed_insts_cycle = 0;
static int num_committed_insts_cycle = 0;


static FU fu[NUM_FU];
static InstType inst_type[NUM_INST_TYPE];
static char strbuf[STRING_BUFFER_SIZE];
static FUType inst_type_fu_type_map[NUM_INST_TYPE];
static int num_inst_type[NUM_INST_TYPE];
FILE *fp;

/* statistics */

static  int num_fetched_insts = 0;
static int num_issued_insts = 0;
static  int num_committed_insts = 0;
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
	inst_type_fu_type_map[Branch] = BranchUnit;
	inst_type_fu_type_map[Syscall] = EtcUnit;

}

void init_all_inst_type(void){

	init_inst_type (&(inst_type[IntAlu]), IntAlu, 1, true);
	init_inst_type (&(inst_type[IntMult]), IntMult, 3, true);
	init_inst_type (&(inst_type[IntDiv]), IntDiv, 12, true);
	init_inst_type (&(inst_type[SimdFloatAdd]), SimdFloatAdd, 10, true);
	init_inst_type (&(inst_type[SimdFloatMult]), SimdFloatMult,12, true);
	init_inst_type (&(inst_type[SimdFloatMultAcc]), SimdFloatMultAcc, 20, true);
	init_inst_type (&(inst_type[MemRead]), MemRead, 2, true);
	init_inst_type (&(inst_type[MemWrite]), MemWrite, 3, true);
	init_inst_type (&(inst_type[Branch]), Branch, 1, true);
	init_inst_type (&(inst_type[Syscall]), Syscall, 1, true);
}

void init_all_queues(void){
	
	rob_init(&rob, rob_capacity);
	if (queue_availablity_check){
		init_inst_queue (&IQ, perfmodel.core.iq_size);
		init_inst_queue (&LDQ, 16);
		init_inst_queue (&STQ, 16);
		init_inst_queue (&FUBuffer, 0);  
	}
	else {
		init_inst_queue (&IQ, 0);
		init_inst_queue (&LDQ, 0);
		init_inst_queue (&STQ, 0);
		init_inst_queue (&FUBuffer, 0);  


	}
}

void init_all_FUs(void){

	init_FU (&(fu[SimpleALU]), SimpleALU, perfmodel.numfu.intalu, perfmodel.numfu.intalu);
	init_FU (&(fu[ComplexALU]), ComplexALU, perfmodel.numfu.intmultdiv, perfmodel.numfu.intmultdiv*3);
	init_FU (&(fu[FPUnit]), FPUnit, 2, 10);
	init_FU (&(fu[LoadUnit]), LoadUnit, 1,1);
	init_FU (&(fu[StoreUnit]), StoreUnit, 1,3);
	init_FU (&(fu[BranchUnit]), BranchUnit, 1, 1);
	init_FU (&(fu[EtcUnit]), EtcUnit, 1, 1);


	/*	
	   init_FU (&(fu[SimpleALU]), SimpleALU, num_simple_alu, 100);
	   init_FU (&(fu[ComplexALU]), ComplexALU, 1, 100);
	   init_FU (&(fu[FPUnit]), FPUnit, 2, 100);
	   init_FU (&(fu[LoadUnit]), LoadUnit, 1, 100);
	   init_FU (&(fu[StoreUnit]), StoreUnit, 1, 100);
	   init_FU (&(fu[BranchUnit]), BranchUnit, 1, 100);
	   init_FU (&(fu[EtcUnit]), EtcUnit, 1, 1);
	  */ 	

}

void reset_all_FUs(void){
	int i=0 ;
	for (i=0 ; i<NUM_FU ; i++){
		reset_FU(&(fu[i]));
	}
}

void reflesh_all_FUs(void){
	int i=0 ;
	for (i=0 ; i<NUM_FU ; i++){
		reflesh_FU(&(fu[i]));
	}

}



void init_trace_analyzer(void){
	rob_capacity = perfmodel.core.rob_size;
	dispatch_width = perfmodel.core.frontend_width;
	issue_width = perfmodel.core.backend_width;
	wb_width = perfmodel.core.backend_width;
	commit_width = perfmodel.core.backend_width;

	latency_l1_hit = perfmodel.il1.latency;
	latency_l1_miss = perfmodel.ul2.latency;
	latency_l2_miss = perfmodel.mem.latency;

	init_trace_statistics();	
	init_inst_elem_pool();
	init_all_queues();
	init_all_FUs();
	init_all_inst_type();
	init_all_inst_type_fu_type_map();
	init_dependency_mat(60);
}

static int num_dispatched_insts= 0;
void init_trace_statistics(void){
	int i=0;
	for (i=0 ; i< NUM_INST_TYPE ; i++){
		num_inst_type[i] = 0;
	}

	num_fetched_insts = 0;
	num_issued_insts = 0;
	num_committed_insts = 0;
	num_dispatched_insts = 0;
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
	rob_access_event = 0;
	iq_full_event = 0;
	ldq_full_event = 0;
	stq_full_event = 0;
	ldq_access_event = 0;
}

void destroy_trace_analyzer(void){
	//destroy_all_inst_elem_from_queue (&ROB); 
	destroy_all_inst_elem_from_queue (&IQ);
	destroy_all_inst_elem_from_queue (&LDQ);
	destroy_all_inst_elem_from_queue (&STQ);
	destroy_all_inst_elem_from_queue (&FUBuffer);
	destroy_inst_elem_pool();
}



void dispatch_inst(Inst *inst){
	num_dispatched_insts_cycle++;
	inst->state = Dispatch;
	if (verbose)
		fprintf(stderr,"%s dispatched\n", print_inst(strbuf, inst));
}

void dispatch_insts(InstQueue *trace_buffer){
	int w = dispatch_width;
	while (w>0){
		InstElem *inst_elem = front_inst_elem(trace_buffer);
		if (!inst_elem){
			break;
		}
		decode_inst(inst_elem->inst);
		convert_microop(trace_buffer, inst_elem->inst);
		inst_elem = front_inst_elem(trace_buffer);

		rob_access_event++;
		if (is_rob_full(&rob)){
			rob_full_event++;
			break;
		}
		else if  (is_full_queue(&IQ)){
			iq_full_event++;
			break;      //dispatch is impossible
		}
		else if (inst_elem->inst->type == MemRead && is_full_queue(&LDQ)){
			ldq_full_event++;
			break;
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
		rob_insert_inst(&rob, inst);
		clean_dependency(&rob, inst);
		InstElem *inst_elem_for_iq = push_back_inst(&IQ, inst);
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

		if (reg_dependency_check)
			find_regs_dependency(inst);
		if (mem_dependency_check)
			find_mems_dependency(inst);
		dispatch_inst(inst);
		inst->dispatched_cycle = cur_cycle;
		w--;
	}
}


bool can_forward(Inst *inst){
	if (inst->type == MemRead){
		if (inst->state >= Complete && inst->remaining_latency  <= 1){
			return true;
		}
		return false;
	}

	else if (inst->state >= Complete){
		return true;
	}	

	else {
		return false;
	}
}



void find_regs_dependency(Inst *inst_b){
	int cur_idx = inst_b->rob_idx;
	int i,j;

	if (cur_idx == rob.start_idx){
		return;	
	}
	int count = inst_b->num_src_reg;
	while (count){
		cur_idx = (cur_idx-1+rob.capacity)%rob.capacity;
		Inst *inst_a = rob_get_inst(&rob, cur_idx);
		for (i = 0; i < inst_b->num_src_reg; i++)	{
			if (inst_b->src_reg[i] == -1){
				continue;
			}
			for (j = 0; j < inst_a->num_dst_reg; j++)	{
				if (inst_b->src_reg[i] == inst_a->dst_reg[j])	{
					set_dependency(&rob, inst_a, inst_b);
					inst_b->src_reg[i] = -1;
					count--;
				}
			}
		}
		if (cur_idx == rob.start_idx)
			break;
	}

}


void find_mems_dependency(Inst *inst_b){
	int cur_idx = inst_b->rob_idx;
	int i,j;
	uint32_t mask = ~(0);

	if (cur_idx == rob.start_idx){
		return;	
	}
	int count = inst_b->num_eff_addr;
	while (count){
		cur_idx = (cur_idx-1+rob.capacity)%rob.capacity;
		Inst *inst_a = rob_get_inst(&rob, cur_idx);
		for (i = 0; i < inst_b->num_eff_addr; i++)	{
			if (inst_b->eff_addr[i] == 0)
				continue;
			for (j = 0; j < inst_a->num_eff_addr; j++) {
				if ((inst_b->eff_addr[i] & mask) ==  (inst_a->eff_addr[j] & mask))		
				{
					inst_b->eff_addr[i] = 0;
					set_dependency(&rob, inst_a, inst_b);
					count --;
				}
			}
		}
	
		if (cur_idx == rob.start_idx)
			break;
	}

}


bool has_dependency(Inst *inst_b){

	int cur_idx = inst_b->rob_idx;

	//you are the oldest one
	if (cur_idx == rob.start_idx){
		return false;	
	}

	while (true){
		cur_idx = (cur_idx-1+rob.capacity)%rob.capacity;
		Inst *inst_a = rob_get_inst(&rob, cur_idx);
		if (is_dependent(&rob, inst_a, inst_b) && !can_forward(inst_a)){
			char buf1[50];
			char buf2[50];
			if (verbose)
				fprintf(stderr,"%s is waiting for %s \n", print_inst(buf1, inst_b), print_inst(buf2, inst_a));
			return true;
		}
		if (cur_idx == rob.start_idx){
			break;
		}

	}
	return false;
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
			Inst *inst = inst_elem->inst;
			if (inst_elem->inst->state == Dispatch){

				if (has_dependency(inst)){
					inst_elem = next_inst_elem;
					continue;
				}

				FUType target_fu = inst_type_fu_type_map[inst_elem->inst->type];
				bool success = try_FU(&fu[target_fu]);
				if (success){
					if (!first_issue_cycle) first_issue_cycle = cur_cycle;
					remove_from_list(&IQ, inst_elem, true);
					inst->issued_cycle = cur_cycle;
					inst->state = Issue;
					int latency =  inst_type[inst->type].latency;
					inst->remaining_latency = latency;	
					num_issued_insts++;
					num_issued_insts_cycle++;
					push_back_inst(&FUBuffer, inst);
					w--;
					if (verbose)
						fprintf(stderr,"%s Issued (latency %d) \n", print_inst(strbuf, inst), inst->remaining_latency);
				}
				else {
					if (verbose)
						fprintf(stderr,"%s cannot Issued (lack of functional unit) \n", print_inst(strbuf, inst));

				}
			}
		}
		else {
			break;
		}
		inst_elem = next_inst_elem;
	}
}

void execute_insts(void){
	InstElem *inst_elem = FUBuffer.oldest_inst_elem;
	while (inst_elem){
		InstElem *next_inst_elem = inst_elem->next_inst_elem;
		if (inst_elem->inst->state == Issue){
			execute_inst(inst_elem->inst);
		}
		inst_elem = next_inst_elem;
	}
}

void writeback_insts(void){
	int w= wb_width;
	InstElem *inst_elem = FUBuffer.oldest_inst_elem;
	while (w && inst_elem){
		InstElem *next_inst_elem = inst_elem->next_inst_elem;

		writeback_inst(inst_elem->inst);
		if (inst_elem->inst->state == Complete && inst_elem->inst->remaining_latency == 0){
			remove_from_list(&FUBuffer, inst_elem, true);
			w--;
		}
		inst_elem = next_inst_elem;
	}
}

void writeback_inst(Inst *inst){
	if (inst->state == Complete){
		inst->remaining_latency --;
		if (verbose)
			fprintf(stderr,"%s Writebacking (latency %d) \n", print_inst(strbuf, inst), inst->remaining_latency);


	}
}



void set_writeback_latency(Inst *inst){
	int latency = default_wb_latency;
	if (inst->type == MemRead){
		if (inst->cache_miss){
			latency += latency_l1_miss; 
		}
		else {
			//latency += latency_l1_hit;
		}
		latency += 1;
	}
//	latency += inst->num_eff_addr;
	inst->remaining_latency = latency;	
}

void execute_inst(Inst *inst){

	num_executed_insts_cycle++;
	inst->remaining_latency--;
	if (verbose)
		fprintf(stderr,"%s Issued (latency %d) \n", print_inst(strbuf, inst), inst->remaining_latency);

	if (inst->remaining_latency <= 0){
		inst->state = Complete;
		inst->completed_cycle = cur_cycle;
		set_writeback_latency(inst);
		FUType target_fu = inst_type_fu_type_map[inst->type];
		release_FU(&fu[target_fu]);
		if (verbose)
			fprintf(stderr,"%s Completed\n", print_inst(strbuf, inst));
	}
	if ( inst->type == Branch && inst->state == Complete && inst->bpred_mispred){
		int penalty = (front_pipeline_depth + cur_cycle - inst->dispatched_cycle);
		if (penalty > 40){		//IQ size
			penalty = 40;
		}
		double sum_mispred_penalty = avg_mispred_penalty * num_mispred_insts + penalty;
		num_mispred_insts++;
		avg_mispred_penalty = sum_mispred_penalty /num_mispred_insts;
	}


}


//Completed stage -> committed stage
void commit_insts(void){
	int w = commit_width;

	while (w>0){
		if (!is_rob_empty(&rob)){
			Inst *inst = rob_get_oldest_inst(&rob);
			if (inst->state == Complete && inst->remaining_latency == 0){				
				inst->committed_cycle = cur_cycle;
				if (verbose){
					fprintf(stderr,"%s Committed %" PRIu64 " cycles\n", print_inst(strbuf, inst), cur_cycle - inst->dispatched_cycle + front_pipeline_depth);
				}
				if (inst->ldq_elem){
					remove_from_list(&LDQ, inst->ldq_elem, true);
				}
				if (inst->stq_elem){
					remove_from_list(&STQ, inst->stq_elem, true);
				}

				//Inst *inst = pop_front_inst(&ROB);
				rob_remove_inst(&rob);
				num_inst_type[inst->type]++;
				if (verbose_pipeline){
					fprintf(stderr,"%s] %" PRIu64 " %" PRIu64" %" PRIu64" %" PRIu64 "\n", print_inst(strbuf, inst), inst->dispatched_cycle, inst->issued_cycle, inst->completed_cycle, inst->committed_cycle);
				}
				free(inst);
				num_committed_insts++;
				num_committed_insts_cycle++;
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
	//vfprintf(stderr,"Remaining Trace Buffer = %d\n", get_current_queue_size(&TraceBuffer));
	fprintf(stderr,"Remaining IQ = %d\n", get_current_queue_size(&IQ));
	//fprintf(stderr,"Remaining ROB = %d\n", get_current_queue_size(&ROB));
	fprintf(stderr,"Remaining FUBuffer = %d\n", get_current_queue_size(&FUBuffer));
	fprintf(stderr,"Remaining LDQ = %d\n", get_current_queue_size(&LDQ));
	fprintf(stderr,"Remaining STQ = %d\n", get_current_queue_size(&STQ));
}


void convert_microop(InstQueue *trace_buffer, Inst *inst){
	
	switch(inst->type){
		case Pop:
			convert_pop_microop(trace_buffer, inst);
			free(inst);
			break;

		case Push:
			convert_push_microop(trace_buffer, inst);
			free(inst);
			break;
		default:
			break;
	}
	
}

void convert_push_microop(InstQueue *trace_buffer, Inst *inst){
	int i;
	pop_front_inst(trace_buffer);

	//sp update
	Inst *m_inst = malloc(sizeof(Inst));
	init_inst(m_inst, inst->seq, inst->pc, inst->code);
		m_inst->decoded = true;

	m_inst->type = IntAlu;
	m_inst->dst_reg[m_inst->num_dst_reg++] = 13;
	m_inst->src_reg[m_inst->num_src_reg] = 13;
	//m_inst->raw_inst[m_inst->num_src_reg] = NULL;
	m_inst->num_src_reg++;
	m_inst->str = "sub ";

	push_front_inst(trace_buffer, m_inst);

	for (i=inst->num_eff_addr-1 ; i>=0 ; i--){
		m_inst = malloc(sizeof(Inst));
		init_inst(m_inst, inst->seq, inst->pc, inst->code);
		m_inst->decoded = true;
		//write from regs to memory
		m_inst->type = MemWrite;
		m_inst->str = "str_op ";

		m_inst->eff_addr[m_inst->num_eff_addr++] = inst->eff_addr[i];
     	m_inst->src_reg[m_inst->num_src_reg] = inst->src_reg[i];
		//m_inst->raw_inst[inst->num_src_reg] = NULL;                                                                                                                                                                                      
        m_inst->num_src_reg++;            

		if (i== 0 && m_inst->cache_miss){
			m_inst->cache_miss = inst->cache_miss;			
		}
		push_front_inst(trace_buffer, m_inst);
		num_fetched_insts++;
	}
}

void convert_pop_microop(InstQueue *trace_buffer, Inst *inst){
	int i;
	
	pop_front_inst(trace_buffer);
	
	//sp update
	Inst *m_inst = malloc(sizeof(Inst));
	init_inst(m_inst, inst->seq, inst->pc, inst->code);
	m_inst->decoded = true;

	m_inst->type = IntAlu;
	m_inst->str = "sub ";
	m_inst->dst_reg[m_inst->num_dst_reg++] = 13;

	m_inst->src_reg[m_inst->num_src_reg] = 13;
	//m_inst->raw_inst[m_inst->num_src_reg] = NULL;
	m_inst->num_src_reg++;
	push_front_inst(trace_buffer, m_inst);

	for (i=inst->num_eff_addr-1 ; i>=0 ; i--){
		m_inst = malloc(sizeof(Inst));
		init_inst(m_inst, inst->seq, inst->pc, inst->code);
		m_inst->decoded = true;

		//read from memory to regs
		m_inst->type = MemRead;
		m_inst->str = "ldr_op ";

		m_inst->eff_addr[m_inst->num_eff_addr++] = inst->eff_addr[i];
		m_inst->dst_reg[m_inst->num_dst_reg++] = inst->dst_reg[i];

		m_inst->src_reg[m_inst->num_src_reg] = 13;
		//m_inst->raw_inst[m_inst->num_src_reg] = NULL;
		m_inst->num_src_reg++;


		if (i== 0 && m_inst->cache_miss){
			m_inst->cache_miss = inst->cache_miss;			
		}
		push_front_inst(trace_buffer, m_inst);

		num_fetched_insts++;
	}
	
}


void trace_analysis(InstQueue *trace_buffer, int max_cycle, uint64_t* last_cycle, int *bpred_penalty){
	int cycle = 0;
	uint64_t actual_last_cycle = 0;
	init_trace_statistics();
	reset_all_FUs();
//	print_buffer_status();
	num_fetched_insts = get_current_queue_size(trace_buffer);
//	ffprintf(stderr,stderr, "Remaining Trace Buffer = %d\n", get_current_queue_size(trace_buffer));

	while (true){
		num_issued_insts_cycle = num_dispatched_insts_cycle = num_executed_insts_cycle = num_committed_insts_cycle = 0;
		if (verbose)
			fprintf(stderr,"%" PRIu64 " cycle\n",   cur_cycle);
		if (max_cycle && cycle >= max_cycle){
			break;
		}
		commit_insts();
		writeback_insts();
		execute_insts();
		issue_insts();
		dispatch_insts(trace_buffer);
		reflesh_all_FUs();
		cycle++;
		cur_cycle++;


		if (!actual_last_cycle){

			if (dispatch_width == 1 && issue_width == 1){
				if (num_issued_insts == num_fetched_insts){
					actual_last_cycle = cur_cycle - first_issue_cycle;
				}
			}
			else {
				if (num_dispatched_insts == num_fetched_insts){
					actual_last_cycle = cur_cycle;
				}
			}
		}

		if (num_committed_insts == num_fetched_insts){
			break;
		}

	}
//	print_buffer_status();
	*last_cycle = actual_last_cycle;
//	fprintf(stderr, "last cycle: %" PRIu64, *last_cycle);
	*bpred_penalty = avg_mispred_penalty;
//	ffprintf(stderr,stderr, "**%d %d\n", actual_last_cycle, avg_mispred_penalty);
//	fprintf(stderr,"%d %d\n", num_fetched_insts, num_committed_insts);
//	fprintf(stderr,"Remaining Trace Buffer = %d\n", get_current_queue_size(trace_buffer));


//	fprintf(stderr,"%f %d %d\n",get_available_rate_FU(&fu[0]), fu[0].num_use, num_fetched_insts ); 
}
