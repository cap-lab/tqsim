#ifndef __BUFFER_MANAGER_H
#define __BUFFER_MANAGER_H

#include "inst_queue.h"

void init_trace_buffer(InstQueue *trace_buffer);
void destroy_trace_buffer(InstQueue *trace_buffer);

void init_trace_file(char* filename, const char *mode);
void destroy_trace_file(void);

int fill_trace_buffer_from_file (InstQueue *trace_buffer);

bool push_code(InstQueue *trace_buffer, unsigned int seq, uint32_t pc, unsigned int code, int miss_type);
bool push_bpred(InstQueue *trace_buffer, bool correct);
bool push_memaddr(InstQueue *trace_buffer, uint32_t addr, int miss_type);

#endif

