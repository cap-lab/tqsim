#ifndef __depanal_h
#define __depanal_h

#include <inttypes.h>
#include <stdio.h>
#include "darm/darm.h"

enum InstType {IntALU, IntMult, IntDiv, Branch, Mem, Syscall, Coproc, Etc};

struct Inst {
	int seq;
	int rd;
	int rm;
	int rn;
	int rt;

	int num_reglist;
	int reglist[16];

	enum InstType type;

	int PC;
	int misPred;		//1 when it is mispredicted branch

	int _numrs;
	int _rs[16];
	int _rdep[16];		//store the dependent inst seq.

	int _numrd;
	int _rd[16];

	int code;
	struct Inst *prevInst;
	struct Inst *nextInst;
	uint64_t fetchCycle;
	uint64_t readyCycle;		//readyCycle. aka, dispatch cycle
	uint64_t latency;		//given Latency
	uint32_t effAddr[10];		//effective address if available. Assume that 10 is the maximum. I guess
	int numEffAddr;

	int rw;
	int condflag;
	int cond;			//conditional branch?

	int state;			//STOP, RUNNING
	int shift;
	int dep;

	struct Inst *waitingInst;

	darm_t d;
	darm_str_t str;
};

struct depanal_result {
	double cpi;
	double bpred_penalty;
};

struct Rob{
	struct Inst *oldestInst;
	struct Inst *newestInst;
	int size;
};


struct InstStream{
	struct Inst *oldestInst;
	struct Inst *newestInst;
	int size;
};

void main(int argc,char *argv[]);
void incCycle(void);
void incCycleWithFreeze(void);
void freezeInsts(void);
void addInstToROB(struct Inst *inst);
void addInstToInstStream(struct Inst *inst);
void setDependency(struct Inst *inst);
void initInst(struct Inst *inst,int seq,int PC,int code);
void setSrcDst(struct Inst *inst);
void setLatency(struct Inst *inst);
void setInstType(struct Inst *inst);
int isShift(struct Inst *inst);
int isUpdateCondFlag(struct Inst *inst);
void setInstDstSrc(struct Inst *inst);
void printWindow(void);
void getInstName(struct Inst *inst,char *rstr);
void cleanExpiredInsts(void);
void runInst(struct Inst *curInst);
int isReady(struct Inst *curInst);
int isDep(struct Inst *prevInst,struct Inst *curInst);
void dequeueInst(struct Inst *inst);
void printInsts(struct Inst *inst);
void getAssem(struct Inst *inst,char *rstr);
void commitInst(struct Inst *curInst);
void finishInst(struct Inst *curInst);

void depanal_init(void);
void depanal_pushBranchCorrect(int correct);
void depanal_pushCode(int seq, int pc, int code);	
void depanal_pushAddr(int addr, int rw, int hit);	
void depanal_getCPI(struct depanal_result *result);	
void depanal_close(void);
struct Inst* popInstFromInstStream(void);
int depanal_test(void);

void depanal_thread_start(void);
void depanal_thread_finish(void);

//static int num_esti_cpi = 0;
//static double sum_esti_cpi = 0;

//static int num_esti_bpred_penalty = 0;
//static double sum_esti_bpred_penalty = 0;

//static int run_pthread = 0;
//pthread_t thread1;


#endif
