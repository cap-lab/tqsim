#ifndef INST_H_INCLUDED
#define INST_H_INCLUDED

#include "common.h"
#include "fu.h"
#include "darm/darm.h"
#include "../cachesim.h"
struct _InstElem;

typedef enum _InstState {Ready, Dispatch, Issue, Complete, Retire} InstState ;
//typedef enum _CacheMissType {L1Hit, L1Miss, L2Miss} CacheMissType;
typedef enum _InstTypeName {IntAlu, IntMult, IntDiv, SimdFloatAdd, SimdFloatMult, SimdFloatMultAcc, MemRead, MemWrite, 
Branch, Syscall, Pop, Push, Unsupported} InstTypeName;


typedef struct _InstType{
	InstTypeName name;
	int latency;
	bool pipelined;
} InstType;

typedef struct _Inst {
    uint64_t seq;
    uint32_t pc;
    uint32_t code;
	InstState state;
    InstTypeName type;
    int remaining_latency;
	
	bool stack_inst;
	bool is_cond_inst;
	bool update_cond_flag;
	bool shift_inst;
	bool mshr;

	/* for dependency test */
	int imm;
	int num_src_reg;
	int num_dst_reg;
	int num_eff_addr;
	int8_t src_reg[16];
	int8_t dst_reg[16];
	uint32_t eff_addr[16];

	/* for branch handling */
	CacheMissType inst_cache_miss;
	CacheMissType cache_miss;
	bool bpred_mispred;

	uint64_t dispatched_cycle;
	uint64_t issued_cycle;
	uint64_t completed_cycle;
	uint64_t committed_cycle;

	/* is necessary? */
	struct _InstElem *iq_elem;
	struct _InstElem *ldq_elem;
	struct _InstElem *stq_elem;
	//struct _InstElem *rob_elem;


	bool decoded;
	int rob_idx;
	char* str;
	//bool is_regs_analysis;

} Inst;

void init_inst_type(InstType *type, InstTypeName name, int latency, bool pipelined);
void init_inst(Inst *inst, uint32_t seq, uint32_t pc, uint32_t code);
void init_inst_src_dst_reg(Inst *inst, darm_t *d);
void set_inst_bpred_mispred(Inst *inst, bool mispred);
void set_inst_eff_addr(Inst *inst, uint32_t addr, CacheMissType cache_miss_type);



void decode_inst(Inst *inst);
char *print_inst(char *buf, Inst *inst);
#endif // INST_H_INCLUDED
