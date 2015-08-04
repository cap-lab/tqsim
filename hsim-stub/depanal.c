#include "perfmodel.h"
#include "depanal.h"
#include "config.h"
#include "statistics.h"
#include "cachesim.h"
#include "bpredsim.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define FRONT_PIPELINE_DEPTH 6

static struct Rob *rob;
static struct InstStream *instStream;

static int usingIntALU = 0;
static int usingRdWrPort = 0;
static int usingIntMultDiv = 0;

static uint64_t curCycle = 0;
static uint64_t numExpiredInsts = 0;
static uint64_t remainingPenalty = 0;

static double depanal_cpi;
static double depanal_bpred_penalty;

static FILE *logfile = 0;
static FILE *tracefile = 0;
static int issuedIntheCycle = 0;
static int commitIntheCycle = 0;

static int num_esti_bpred_penalty = 0;
static double sum_esti_bpred_penalty = 0;

static int last_branch_finish=0;
static int misPred = 0;
static int misPredPenalty = 0;
static int numMisPred = 0;


static int bTrace = 0;

#include <pthread.h>

static int total_num_esti_cpi = 0;
static double total_sum_esti_cpi = 0;

static int total_num_esti_bpred_penalty = 0;
static double total_sum_esti_bpred_penalty = 0;


static int run_pthread = 0;
pthread_t thread1;

struct depanal_result dd_result;

void depanal_thread_start(void){
	run_pthread = 1;
	pthread_create (&thread1, NULL, (void *) &depanal_getCPI, &dd_result);

}
void depanal_thread_finish(void){
//	printf("depanal_thread_finish\n");
	if (run_pthread){
		pthread_join(thread1, (void **)NULL);

		//update.
		//printf("%f...\n", d_result.cpi);
		total_sum_esti_cpi += dd_result.cpi;
		total_num_esti_cpi++;
		cpi = total_sum_esti_cpi / (double)total_num_esti_cpi;

		if (dd_result.bpred_penalty < 100){
			total_sum_esti_bpred_penalty += dd_result.bpred_penalty;
			total_num_esti_bpred_penalty ++;
			bpred_penalty = total_sum_esti_bpred_penalty / (double) total_num_esti_bpred_penalty;
		}

		run_pthread = 0;
	}
}



void
finishInst (struct Inst *curInst)
{
	//printf("l%d\n", (int)curInst->latency);


	if (curInst->type == IntALU)
		usingIntALU--;
	else if (curInst->type == Mem)
		usingRdWrPort--;
	//      else if (curInst->type == Branch)
	//              usingIntALU--;
	else if (curInst->type == IntMult || curInst->type == IntDiv)
		usingIntMultDiv--;
	curInst->state = 2;

	if (curInst->type == Branch && curInst->misPred){

		if (FRONT_PIPELINE_DEPTH+curCycle - curInst->fetchCycle < 100){
			sum_esti_bpred_penalty += (FRONT_PIPELINE_DEPTH+curCycle - curInst->fetchCycle);
			num_esti_bpred_penalty++;
			depanal_bpred_penalty = sum_esti_bpred_penalty / (double)num_esti_bpred_penalty;
		}
		last_branch_finish = curCycle;
		misPred = 0;

	}
}


void
commitInst (struct Inst *curInst)
{
  if (logfile)
    {
      char name[64];
      getAssem (curInst, name);
      fprintf (logfile, "%x %s: (%" PRIu64 " , %" PRIu64 ", %" PRIu64 ")\n",
	       curInst->PC, name, curInst->fetchCycle, curInst->readyCycle,
	       curCycle);
      printInsts (curInst);
    }
/*	
	if (curInst->type == Branch && curInst->misPred){
		remainingPenalty += 6+(curCycle - curInst->readyCycle);
//		printf("%d\n", remainingPenalty);
		while (remainingPenalty){
			incCycleWithFreeze();
		}
	}
*/
  dequeueInst (curInst);
  numExpiredInsts++;
}

int
isDep (struct Inst *prevInst, struct Inst *curInst)
{

  int i, j;
  curInst->waitingInst = 0;
  //register name dependency
  for (i = 0; i < curInst->_numrs; i++)
    {
      for (j = 0; j < prevInst->_numrd; j++)
	{
	  if (curInst->seq != prevInst->seq
	      && curInst->_rs[i] == prevInst->_rd[j])
	    {
	      curInst->waitingInst = prevInst;
	      return 0;
	    }
	}
    }

  //conditional dependency
  if (curInst->cond && prevInst->condflag)
    {
      curInst->waitingInst = prevInst;
      return 0;
    }

  //find effective address dependency
  for (i = 0; i < curInst->numEffAddr; i++)
    {
      for (j = 0; j < prevInst->numEffAddr; j++)
	{
	  if (prevInst->type == Mem && curInst->type == Mem
	      && !curInst->rw && prevInst->rw &&
	      abs (curInst->effAddr[i] - prevInst->effAddr[j]) <
	      perfmodel.dl1.size_blk)
	    {
	      curInst->waitingInst = prevInst;
	      return 0;
	    }
	}
    }
  curInst->waitingInst = NULL;
  return 1;
}

int
isReady (struct Inst *curInst)
{
  //find sources
  //
  if (curInst->type == Syscall && rob->oldestInst != curInst)
    {
      return 0;
    }
  struct Inst *prevInst = curInst->prevInst;
  while (prevInst)
    {

      if (prevInst->state == 2 || prevInst->state == 3)
	{
	  prevInst = prevInst->prevInst;
	  continue;
	}

      int readyDeps = isDep (prevInst, curInst);
      if (!readyDeps)
	{
	  return 0;
	}

      prevInst = prevInst->prevInst;
    }

  if ((curInst->type == IntALU && perfmodel.numfu.intalu == usingIntALU) ||
      (curInst->type == Mem && perfmodel.numfu.mem_rwport == usingRdWrPort) ||
      (curInst->type == IntMult
       && perfmodel.numfu.intmultdiv == usingIntMultDiv)
      || (curInst->type == IntDiv
	  && perfmodel.numfu.intmultdiv == usingIntMultDiv))
    {
      return 0;
    }
  return 1;
}

void
runInst (struct Inst *curInst)
{
  //start executing

  if (issuedIntheCycle <= perfmodel.core.issue_width)
    {
      issuedIntheCycle++;
      curInst->state = 1;
      curInst->readyCycle = curCycle;
      if (curInst->type == IntALU)
	usingIntALU++;
      else if (curInst->type == Mem)
	{
	  usingRdWrPort++;
	}
      //              else if (curInst->type == Branch){
      //                      usingIntALU++;
      //              }
      else if (curInst->type == IntMult || curInst->type == IntDiv)
	{
	  usingIntMultDiv++;
	}
      else if (curInst->type == Syscall)
	{
	  remainingPenalty += perfmodel.etc.syscall_penalty;
	  while (remainingPenalty)
		  incCycleWithFreeze();
			  
	}
      else if (curInst->type == Coproc)
	{
	}
    }


}


void
cleanExpiredInsts (void)
{
  struct Inst *curInst = rob->oldestInst;
 
  while (curInst && commitIntheCycle < perfmodel.core.dispatch_width)
    {


      if (curInst->state == 0 && isReady (curInst))
	{
		curInst->waitingInst = NULL;
	  runInst (curInst);
	}

      if (curInst->state == 1
	  && curInst->readyCycle + curInst->latency - 1 <= curCycle)
	{
	  finishInst (curInst);
	}

      if (curInst->state == 2
	  && curInst->readyCycle + curInst->latency <= curCycle
	  && rob->oldestInst == curInst)
	{
	  struct Inst *tmpInst1 = curInst;
	  struct Inst *tmpInst2 = curInst->nextInst;
	  commitInst (tmpInst1);
	  curInst = tmpInst2;
	  commitIntheCycle++;
	  continue;
//
//                              commitInst(curInst);

	}

      curInst = curInst->nextInst;
    }
}



void
getInstName (struct Inst *inst, char *rstr)
{
	  strcpy (rstr, inst->str.mnemonic);
}

void
getAssem (struct Inst *inst, char *rstr)
{
	  strcpy (rstr, inst->str.total);
}



void
printInsts (struct Inst *inst)
{
  int i;
  if (logfile)
    {
//      getAssem(inst->code, buf2);

//      fprintf(stderr, "%s", buf2);

      fprintf (logfile, "/Src");

      for (i = 0; i < inst->_numrs; i++)
	{
	  fprintf (logfile, "{%d}", inst->_rs[i]);

	}
      fprintf (logfile, "/Dst");

      for (i = 0; i < inst->_numrd; i++)
	{
	  fprintf (logfile, "{%d}", inst->_rd[i]);

	}
      fprintf (logfile, "\n");
    }

}


//scan rob and remove expired insts
void
printWindow (void)
{
	struct Inst *curInst = rob->oldestInst;
	printf ("%" PRIu64 " cycle [%d][%d][", curCycle, instStream->size, rob->size);

	while (curInst)
	{
		if (curInst->type == Branch){
		char instname[64];
		getInstName (curInst, instname);
		if (curInst->numEffAddr == 0)
			printf ("[%d:%s(%x)(%" PRIu64 " , %" PRIu64 ", %" PRIu64 ") ",
					curInst->seq, instname, curInst->code, curInst->fetchCycle,
					curInst->readyCycle, curInst->latency);
		else
			printf ("[%d:%s(%x)[%X](%" PRIu64 " , %" PRIu64 ", %" PRIu64 ") ",
					curInst->seq, instname, curInst->code, curInst->numEffAddr,
					curInst->fetchCycle, curInst->readyCycle, curInst->latency);
		printf ("<state%d>", curInst->state);
		
		if (curInst->misPred)
			printf ("<mispredicted>");
		if (curInst->waitingInst)
			printf ("<waiting %d>", curInst->waitingInst->seq);
		printf("]");
		}
		curInst = curInst->nextInst;
	}
	printf ("]\n");

}

void
setInstDstSrc (struct Inst *inst)
{

	int regs[16];
	inst->rd = inst->d.Rd;
	inst->rn = inst->d.Rn;
	inst->rm = inst->d.Rm;
	inst->rt = inst->d.Rt;
	inst->num_reglist = darm_reglist2 (inst->d.reglist, regs);
	memcpy (inst->reglist, regs, sizeof (regs));
}



int
isUpdateCondFlag (struct Inst *inst)
{
      return inst->d.S;

}

int
isShift (struct Inst *inst)
{

      return inst->d.shift_type;

}

static char ALUInstList[][12] =
    { "uopReg_uop", "adc", "add", "and", "abs", "bic", "clz", "cmn", "cmp",
    "cvt", "eor", "mov", "mrs", "msr", "mvn", "orr", "rsb", "rsc", "sbc",
      "sub", "teq", "tst",
    "ror", "rrx"
  };
  static char ALUShiftInstList[][12] = { "lsl", "lsr", "asr", "asl" };
  static char condFlag[][3] =
    { "eq", "ne", "cs", "hs", "cc", "lo", "mi", "pl", "vs", "vc", "hi", "ls",
    "ge", "lt", "gt", "le"
  };


void
setInstType (struct Inst *inst)
{
  if (inst == NULL)
    return;
  char instName[64];

  int i;
  getInstName (inst, instName);

  for (i = 0; i < sizeof (condFlag) / sizeof (condFlag[0]); i++)
  {
	  if (strstr (instName, condFlag[i]))
	  {
		  inst->cond = 1;
		  break;
	  }
  }


  for (i = 0; i < sizeof (ALUInstList) / sizeof (ALUInstList[0]); i++)
  {
	  if (strstr (instName, ALUInstList[i]))
	  {
		  if (isShift (inst) == 1)
		  {
			  inst->shift = 1;
		  }
		  inst->type = IntALU;
		  simStat.numIntInsts++;
	  }
  }

  for (i = 0; i < sizeof (ALUShiftInstList) / sizeof (ALUShiftInstList[0]);
		  i++)
  {
	  if (strstr (instName, ALUShiftInstList[i]))
	  {
		  inst->type = IntALU;
		  simStat.numIntInsts++;
	  }
  }


  if (strstr (instName, "ld") || strstr (instName, "pop"))
  {
	  inst->type = Mem;
	  simStat.numMemInsts++;
	  inst->rw = 0;

  }

  else if (strstr (instName, "st") || strstr (instName, "push"))
  {
	  inst->type = Mem;
	  simStat.numMemInsts++;
	  inst->rw = 1;
  }

  else if (instName[0] == 'b')
  {
	  inst->type = Branch;
	  simStat.numBranchInsts++;
  }

  else if (strstr (instName, "mla") ||
		  strstr (instName, "mul") || strstr (instName, "mls"))
  {
	  inst->type = IntMult;
	  simStat.numIntMultInsts++;

  }
  else if (strstr (instName, "div") || strstr (instName, "sqrt"))
  {
	  inst->type = IntDiv;
	  simStat.numIntDivInsts++;
  }

  else if (strstr (instName, "mrc") || strstr (instName, "mcr"))
  {
	  inst->type = Coproc;
	  simStat.numCoprocInsts++;
  }
  else if (strstr (instName, "svc"))
  {
	  inst->type = Syscall;
	  simStat.numSyscallInsts++;
  }

  else if (isUpdateCondFlag (inst) == 1 ||
		  strstr (instName, "cmp") || strstr (instName, "cmn") ||
		  strstr (instName, "tst") || strstr (instName, "teq"))
  {
	  inst->condflag = 1;
  }

  else if (inst->type == Etc)
  {
	  simStat.numEtcInsts++;
  }

}


void
dequeueInst (struct Inst *inst)
{

  struct Inst *curInst = rob->oldestInst;

  //find curInst position
  while (curInst && curInst != inst)
    {
      curInst = curInst->nextInst;
    }
  if (curInst)
    {
      struct Inst *tmp = curInst;	// to be deleted
      if (curInst->nextInst)
	curInst->nextInst->prevInst = curInst->prevInst;

      if (curInst->prevInst)
	curInst->prevInst->nextInst = curInst->nextInst;

      if (rob->oldestInst == tmp)
	{
	  rob->oldestInst = tmp->nextInst;
	}
      if (rob->newestInst == tmp)
	{
	  rob->newestInst = tmp->prevInst;
	}
      rob->size--;
      free (tmp);
    }
  else
    {
      fprintf (stderr, "ERROR!!!\n");
    }


}

void
setLatency (struct Inst *inst)
{

  if (inst == NULL)
    return;

  switch (inst->type)
    {
    case IntALU:
      inst->latency = perfmodel.latency.intalu;
    case Branch:
      inst->latency = perfmodel.latency.branch;
      break;
    case IntMult:
      inst->latency = perfmodel.latency.intmult;
      break;
    case IntDiv:
      inst->latency = perfmodel.latency.intdiv;
      break;
    case Mem:
      inst->latency = perfmodel.latency.mem;
      break;
    case Syscall:
      inst->latency = 1;
      break;
    case Coproc:
      inst->latency = perfmodel.latency.coproc;
      break;
    case Etc:
      inst->latency = 1;
      break;

    }

  inst->latency = inst->latency + 2;
//      if (inst->shift)
//              inst->latency +=1 ;

}


void
setSrcDst (struct Inst *inst)
{

  char instName[64];
  int i;
  getInstName (inst, instName);
  inst->_numrs = 0;
  inst->_numrd = 0;

  if (instName == NULL)
    return;

  if (strstr (instName, "ld") || strstr (instName, "pop"))
    {
      if (inst->rt != -1)
	inst->_rd[inst->_numrd++] = inst->rt;
      if (inst->rn != -1)
	inst->_rs[inst->_numrs++] = inst->rn;

      for (i = 0; i < inst->num_reglist; i++)
	{
	  inst->_rd[inst->_numrd++] = inst->reglist[i];
	}
    }
  else if (strstr (instName, "st") || strstr (instName, "push"))
    {
      if (inst->rt != -1)
	inst->_rs[inst->_numrs++] = inst->rt;

      if (inst->rn != -1)
	inst->_rs[inst->_numrs++] = inst->rn;

      for (i = 0; i < inst->num_reglist; i++)
	{
	  inst->_rs[inst->_numrs++] = inst->reglist[i];
	}
    }
  else
    {
      if (inst->rd != -1)
	inst->_rd[inst->_numrd++] = inst->rd;

      if (inst->rm != -1)
	inst->_rs[inst->_numrs++] = inst->rm;

      if (inst->rn != -1)
	inst->_rs[inst->_numrs++] = inst->rn;
    }


  //is required?

  if (strstr (instName, "push") || strstr (instName, "pop"))
    {
      inst->_rd[inst->_numrd++] = 13;
      inst->_rs[inst->_numrs++] = 13;
    }
/*
	//sure?
	if ( inst->type == Syscall){
		inst->_rd[inst->_numrd++] = 0;
	}
	*/

}

void
initInst (struct Inst *inst, int seq, int PC, int code)
{
  inst->seq = seq;
  inst->code = code;
  inst->fetchCycle = curCycle;
  inst->readyCycle = 0;
  inst->latency = 0;
  inst->state = 0;
  inst->PC = PC;
  inst->misPred = 0;
  inst->numEffAddr = 0;
  inst->type = Etc;
  inst->rw = 0;
  inst->cond = 0;
  inst->condflag = 0;
  inst->dep = 0;
  inst->waitingInst = 0;
  inst->prevInst = 0;
  inst->nextInst = 0;

  darm_armv7_disasm (&(inst->d), inst->code);
  darm_str2 (&(inst->d), &(inst->str), 1); 
  setInstDstSrc (inst);
  setSrcDst (inst);
  setInstType (inst);
  setLatency (inst);
  setDependency (inst);

}

void
setDependency (struct Inst *inst)
{


}

void
addInstToROB (struct Inst *inst)
{

  inst->nextInst = 0;
  inst->prevInst = rob->newestInst;
  inst->fetchCycle = curCycle;

  if (rob->size > 0)
    {
      rob->newestInst->nextInst = inst;
    }
  else
    {
      rob->oldestInst = inst;
    }
  rob->newestInst = inst;
  rob->size++;

}

void
addInstToInstStream (struct Inst *inst)
{

  inst->nextInst = 0;
  inst->prevInst = instStream->newestInst;

  if (instStream->size > 0)
    {
      instStream->newestInst->nextInst = inst;
    }
  else
    {
      instStream->oldestInst = inst;
    }
  instStream->newestInst = inst;
  instStream->size++;

}



int lastSeq = -1;


void
freezeInsts (void)
{
  struct Inst *curInst = rob->oldestInst;
  while (curInst)
    {
      if (curInst->state > 0)
		curInst->latency++;

      curInst = curInst->nextInst;
    }
}

void
incCycleWithFreeze (void)
{

//      printf("curCycle = %d\n", curCycle);
//      printWindow();
  freezeInsts ();
//      cleanExpiredInsts();
  curCycle++;
  issuedIntheCycle = 0;
  if (remainingPenalty > 0)
    {
      remainingPenalty--;
    }

  if (curCycle > 100 && numExpiredInsts > 0)
    {
//      cpi =    (curCycle) / (double) numExpiredInsts;
         depanal_cpi = (curCycle-misPredPenalty) / (double) numExpiredInsts;
//	depanal_cpi = (curCycle) / (double) numExpiredInsts;

	}
  if (curCycle % 100000 == 0)
    {
      printf ("%" PRIu64 " cycle:  rob size = %d  cpi = %f\n", curCycle,
	      rob->size, depanal_cpi);
    }
/*
 *	Errorneous condition
 */

/*		
	if (rob->newestInst && lastSeq ==  rob->newestInst->fetchCycle){
		printWindow();
	}
	if (rob->newestInst)
		lastSeq = rob->newestInst->fetchCycle;
*/

//      cleanExpiredInsts();
//      cleanup_rdeps();
}

void
incCycle (void)
{
//	printWindow();
	if (rob->size > 0){
		cleanExpiredInsts ();
	}
	  curCycle++;
	issuedIntheCycle = 0;
    commitIntheCycle = 0;
	if (remainingPenalty > 0)
    {
      remainingPenalty--;
    }

 // printf("%d %d\n", (int)(numExpiredInsts-x), (int)curCycle); 
  if (curCycle > 100 && numExpiredInsts > 0)
    {
      depanal_cpi = (curCycle-misPredPenalty) / (double) numExpiredInsts;
//         depanal_cpi = (curCycle) / (double) numExpiredInsts;


	}
//  if (curCycle % 100000 == 0)
//   {
//     printf ("%" PRIu64 " cycle:  rob size = %d  cpi = %f\n", curCycle,   rob->size, depanal_cpi);
//    }

}



//static int num_esti_cpi_all = 0;
//static double sum_esti_cpi_all = 0;
void
depanal_init (void)
{
  initStatistics ();

  num_esti_bpred_penalty = 0;
  sum_esti_bpred_penalty = 0;

  instStream = (struct InstStream *) malloc (sizeof (struct InstStream));
  instStream->size = 0;
  instStream->newestInst = 0;
  instStream->oldestInst = 0;

  
  if(bTrace){
	  printf("Tracing is initialized...\n");
	  tracefile = fopen("tracefile", "w");
  }

}

void
depanal_pushCode (int seq, int pc, int code)
{
	/*
	{
  instStream->size = 0;
  free(instStream->newestInst);
  instStream->newestInst = 0;
  instStream->oldestInst = 0;
	}
	*/

  struct Inst *inst = NULL;
  inst = (struct Inst *) malloc (sizeof (struct Inst));
  initInst (inst, seq, pc, code);
  addInstToInstStream (inst);

  if (bTrace)
	  fprintf(tracefile, "c %d %X %X\n", seq, pc, (unsigned int)code);
}

void
depanal_pushBranchCorrect (int correct)
{
	if (instStream->newestInst->type == Branch)
		instStream->newestInst->misPred = !correct;

	if (bTrace)
	  fprintf(tracefile, "bc %d\n", correct);

}

void
depanal_pushAddr (int addr, int rw, int hit)
{
	int lat=0;
    instStream->newestInst->effAddr[instStream->newestInst->numEffAddr++] =   addr;

	if (bTrace)
	  fprintf(tracefile, "a %X %d %d\n",addr, rw, hit);

  if (!hit)
  {
	  lat = dl1_cache->config.miss_lat;
	//pay only dl1 miss penalty 
  }

  if (rw == 0){// &&  instStream->newestInst->latency < perfmodel.dl1.miss_penalty   ){
    instStream->newestInst->latency = instStream->newestInst->latency + lat;
  }
  //assume infinite write buffer
  else
    instStream->newestInst->latency += 0;

}

//static int t = 0;

void
depanal_getCPI (struct depanal_result *result)
{
//	depanal_init();
  rob = (struct Rob *) malloc (sizeof (struct Rob));
  rob->size = 0;
  rob->newestInst = 0;
  rob->oldestInst = 0;
  usingIntALU = 0;
  usingRdWrPort = 0;
  usingIntMultDiv = 0;

  curCycle = 0;
  numExpiredInsts = 0;
  misPredPenalty = 0;
  misPred = 0;
  numMisPred = 0;
  depanal_cpi = 0;
  
  depanal_bpred_penalty = 0;
  num_esti_bpred_penalty = 0;
  sum_esti_bpred_penalty = 0;

 
  remainingPenalty = 0;
  issuedIntheCycle = 0;


  int curSeq = 0;

  while (instStream->size > 0 || rob->size > 0)
  {
	  // printf("%d %d\n", instStream->size, rob->size);
	  struct Inst *inst = NULL;

	  if (instStream->size > 0)
	  {

		  inst = popInstFromInstStream ();
		  addInstToROB (inst);
		  curSeq++;


		  if (inst->misPred && inst->type == Branch ){
			  misPred = 1;
			  numMisPred++;
		  }

		  int p = 0;
		  while ( misPred){
			  incCycle ();
			  p++;
			  curSeq = 0;
		  }
		  misPredPenalty+=p;

		  if (curSeq == perfmodel.core.dispatch_width)
		  {
			  curSeq = 0;
			  incCycle ();
		  }
	  }

	  else
	  {
		  incCycle ();
	  }

	  while (rob->size >= perfmodel.core.window_size)
	  {
		  curSeq = 0;
		  incCycle ();
	  }
  }
  result->cpi = depanal_cpi;
  result->bpred_penalty = depanal_bpred_penalty; 
  //printf("%f :%f\n", depanal_cpi, depanal_bpred_penalty);
  depanal_close();	
}


void
depanal_close (void)
{
  if (bTrace){
	  printf("Tracing is finished...\n");
	  fclose(tracefile);
  }
  free (rob);
  free (instStream);
}

struct Inst *
popInstFromInstStream (void)
{

  struct Inst *curInst = instStream->oldestInst;

  if (curInst)
    {
      struct Inst *tmp = curInst;	// to be deleted
      if (curInst->nextInst)
	curInst->nextInst->prevInst = curInst->prevInst;

      if (curInst->prevInst)
	curInst->prevInst->nextInst = curInst->nextInst;

      if (instStream->oldestInst == tmp)
	{
	  instStream->oldestInst = tmp->nextInst;
	}
      if (instStream->newestInst == tmp)
	{
	  instStream->newestInst = tmp->prevInst;
	}
      instStream->size--;
	  tmp->prevInst = 0;
	  tmp->nextInst = 0;
      return tmp;
    }
  else
    {
      fprintf (stderr, "ERROR!!!\n");
    }
  return 0;

}

/*
//it is the main function previously
int
depanal_test (void)
{

  load_configfile ("armv7.cfg");
  depanal_init ();
  const char *filename = "basicmath_trace";

  //load a trace file
  FILE *fp = fopen (filename, "r");
  int seq, pc, code = -1;
  while (EOF != fscanf (fp, "%d:%x:%x", &seq, &pc, &code))
    {
      if (!(seq % 1000000))
	{
	  printf ("%d...\n", seq);
	}

      if (seq != -1)
	depanal_pushCode (seq, pc, code);
      else
	depanal_pushAddr (code, pc, 0);
    }

  fclose (fp);



  double CPI = depanal_getCPI ();
  printf ("CPI = %f\n", CPI);


  printf ("# committed insts : %" PRIu64 "\n", numExpiredInsts);
  printf ("Execution cycle : %" PRIu64 "\n", curCycle);

  printStatistics ();
  return 0;
}
*/
