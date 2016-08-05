#ifndef __TRACE_ANALYZER_H
#define __TRACE_ANALYZER_H

#include "inst.h"
#include "inst_queue.h"

/* mode 1: l2 cache simulation 
 * mode 2: no l2 cache simulation
 * */

typedef enum _SimType  { DetailedMode, SimpleMode  } SimType;
extern SimType sim_type;

void init_all_inst_type_fu_type_map(void);
void init_all_inst_type(void);	

void init_all_queues(void);
void init_all_FUs(void);

void reset_all_FUs(void);
void reflesh_all_FUs(void);

void init_trace_analyzer(void);
void init_trace_statistics(void);
void destroy_trace_analyzer(void);

void find_regs_dependency(Inst *inst_b);
bool has_dependency(Inst *inst);

void find_mems_dependency(Inst *inst_b);
void set_writeback_latency(Inst *inst);



Inst* has_mem_dependency(Inst *inst);
bool can_forward(Inst *inst);
void print_instruction_queue(InstQueue *queue);
bool is_executable_mem_inst(Inst *inst);
void read_trace_file(void);

void writeback_inst(Inst *inst);
void writeback_insts(void);
void dispatch_inst(Inst *inst);
void dispatch_insts(InstQueue *trace_analyzer);  //init stage-> dispatch stage
void issue_insts(void); // dispatch -> issued
void execute_insts(void); // issued -> completed
void commit_insts(void); // completed->committed
void execute_inst(Inst *inst);
void trace_analysis(InstQueue *trace_buffer, int maxcycle, uint64_t *cycle, int *bpred_penalty);

void print_buffer_status(void);

void convert_microop(InstQueue *trace_buffer, Inst *inst);
void convert_push_microop(InstQueue *trace_buffer, Inst *inst);
void convert_pop_microop(InstQueue *trace_buffer, Inst *inst);


#endif
