#ifndef INST_QUEUE_H_INCLUDED
#define INST_QUEUE_H_INCLUDED

#include "inst.h"


typedef struct _InstElem{
	Inst *inst;
	struct _InstElem *prev_inst_elem;
	struct _InstElem *next_inst_elem;
} InstElem;

typedef struct _InstQueue{
	InstElem *oldest_inst_elem;
	InstElem *newest_inst_elem;
	int size;
	int capacity;
} InstQueue;

void init_inst_elem_pool(void);
void init_inst_elem (InstElem *elem, Inst *inst);
void init_inst_queue (InstQueue *queue, int initial_capacity);
InstElem* push_back_inst_elem(InstQueue *queue, InstElem *elem);
InstElem* push_back_inst(InstQueue *queue, Inst *inst);
InstElem* pop_front_inst_elem(InstQueue *queue);
Inst* pop_front_inst(InstQueue *queue);
InstElem* front_inst_elem(InstQueue *queue);
void push_front_inst(InstQueue *queue, Inst *inst);
void remove_from_list(InstQueue *queue, InstElem *cur_inst_elem, bool push_back_inst_elem_pool);

bool is_full_queue(InstQueue *queue);
bool is_empty_queue(InstQueue *queue);
int get_current_queue_size(InstQueue *queue);

InstElem* get_inst_elem_from_pool(void);	
void print_inst_queue(InstQueue *queue);
void destroy_inst_elem_pool(void);
void destroy_all_inst_elem_from_queue(InstQueue *queue);

#endif // INST_QUEUE_H_INCLUDED
