#include "inst.h"
#include "inst_queue.h"

static const int inst_elem_pool_size = 5000;
static InstQueue inst_elem_pool;

void init_inst_elem_pool(void){
    int i;
    init_inst_queue (&inst_elem_pool, inst_elem_pool_size);
    for (i=0 ; i<inst_elem_pool_size ; i++){
		InstElem *inst_elem  = (InstElem*)malloc(sizeof(InstElem));
		init_inst_elem(inst_elem, NULL);	
		push_back_inst_elem(&inst_elem_pool, inst_elem);
    }
//	printf("Instruction element pool (%d entries, %X) is initialized ...\n", inst_elem_pool.size, (unsigned int)inst_elem_pool.oldest_inst_elem);
}


InstElem* get_inst_elem_from_pool(void){
	InstElem *inst_elem = inst_elem_pool.oldest_inst_elem;
    if (inst_elem){
		remove_from_list(&inst_elem_pool, inst_elem, false);
    }
    else{
		inst_elem = (InstElem*)malloc(sizeof(InstElem));
    }

	init_inst_elem(inst_elem, NULL);
	return inst_elem;
}

void destroy_inst_elem_pool(void){
	destroy_all_inst_elem_from_queue (&inst_elem_pool);
}


void destroy_all_inst_elem_from_queue(InstQueue *queue){
	InstElem *elem = queue->oldest_inst_elem;
	while (elem){
		InstElem *next_elem = elem->next_inst_elem;
		free(elem);
        queue->size--;
		elem = next_elem;
    }
}


void init_inst_elem (InstElem *elem, Inst *inst){
    elem->inst = inst;
    elem->next_inst_elem = NULL;
    elem->prev_inst_elem = NULL;
}

void init_inst_queue (InstQueue *queue, int initial_capacity){
    queue->oldest_inst_elem = NULL;
    queue->newest_inst_elem = NULL;
    queue->size = 0;
    queue->capacity = initial_capacity;
}

InstElem* push_back_inst_elem(InstQueue *queue, InstElem *elem)
{
	if (queue->capacity == 0 || queue->size < queue->capacity){
  		elem->next_inst_elem = NULL;
  		elem->prev_inst_elem = queue->newest_inst_elem;
  
		if (queue->size > 0){
			queue->newest_inst_elem->next_inst_elem = elem;
		}
		else {
      		queue->oldest_inst_elem = elem;
		}
		queue->newest_inst_elem = elem;
  		queue->size++;
 
		return elem;

	}
	else {
		printf("FULL\n");
		return NULL;
	}
}

InstElem* push_back_inst(InstQueue *queue, Inst *inst)
{
	if (queue->capacity == 0 || queue->size < queue->capacity){
		 InstElem *elem = get_inst_elem_from_pool();
  		 elem->inst = inst;
		 elem->next_inst_elem = NULL;
  	 	 elem->prev_inst_elem = queue->newest_inst_elem;
		 push_back_inst_elem(queue, elem);
		 return elem;
	}
	else {
		printf("FULL2\n");
		return NULL;
	}
}



InstElem* front_inst_elem(InstQueue *queue){
        return queue->oldest_inst_elem;
}

void print_inst_queue (InstQueue *queue){
	
	InstElem *elem = queue->oldest_inst_elem;
	char buf[255];
	while (elem){
		printf("[%s]\n", print_inst(buf, elem->inst));
		elem = elem->next_inst_elem;
	}
	printf("\n");
}

void remove_from_list(InstQueue *queue, InstElem *cur_inst_elem, bool push_back_inst_elem_pool){
    if (cur_inst_elem->next_inst_elem)
        cur_inst_elem->next_inst_elem->prev_inst_elem = cur_inst_elem->prev_inst_elem;

    if (cur_inst_elem->prev_inst_elem)
        cur_inst_elem->prev_inst_elem->next_inst_elem = cur_inst_elem->next_inst_elem;

    if (queue->oldest_inst_elem && queue->oldest_inst_elem == cur_inst_elem)
    {
        queue->oldest_inst_elem = cur_inst_elem->next_inst_elem;
    }
    if (queue->newest_inst_elem && queue->newest_inst_elem == cur_inst_elem)
    {
        queue->newest_inst_elem = cur_inst_elem->prev_inst_elem;
    }
    cur_inst_elem->next_inst_elem = NULL;
    cur_inst_elem->prev_inst_elem = NULL;

	if (push_back_inst_elem_pool){
		if (is_full_queue(&inst_elem_pool)){
			push_back_inst_elem(&inst_elem_pool, cur_inst_elem);
		}
		else {
			free(cur_inst_elem);
		}
	}
	queue->size--;
}


InstElem* pop_front_inst_elem(InstQueue *queue){
    InstElem *cur_inst_elem = queue->oldest_inst_elem;
    if (cur_inst_elem){
		remove_from_list(queue, cur_inst_elem, true);
        return cur_inst_elem;
    }
    else{
        return NULL;
    }
}

void push_front_inst(InstQueue *queue, Inst *inst){
	 InstElem *elem = get_inst_elem_from_pool();
	 elem->inst = inst;
	
	if (queue->capacity == 0 || queue->size < queue->capacity){
  		elem->next_inst_elem = queue->oldest_inst_elem;
  		elem->prev_inst_elem = NULL;
  
		if (queue->size > 0){
			queue->oldest_inst_elem->prev_inst_elem = elem;
		}
		else {
      		queue->oldest_inst_elem = elem;
		}
		queue->oldest_inst_elem = elem;
  		queue->size++;
// 		return elem;
	}
	else {
//		return NULL;
	}

}

Inst* pop_front_inst(InstQueue *queue){
    InstElem *cur_inst_elem = queue->oldest_inst_elem;
    if (cur_inst_elem){
		Inst *inst = cur_inst_elem->inst;
        remove_from_list(queue, cur_inst_elem, true);
		return inst;
    }
    else{
        return NULL;
    }
}


bool is_full_queue(InstQueue *queue){
	if (queue->capacity == 0){
		return false;
	}
    if (queue->size >= queue->capacity){
        return true;
    }
    else{
        return false;
    }
}
bool is_empty_queue(InstQueue *queue){
    if (queue->size == 0){
        return true;
    }
    else{
        return false;
    }
}

int get_current_queue_size(InstQueue *queue){
    return queue->size;
}

