#ifndef __HSIM_MEMTYPE_H
#define __HSIM_MEMTYPE_H

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
    MEM_VIRTUAL,
	MEM_INTERNAL,
    MEM_LOCAL,
    MEM_SCRATCHPAD,
    MEM_SHARED_C,
	MEM_SHARED_NC,
    MEM_UNKNOWN,
	MEM_UNDEFINED
} MemoryType;


#ifdef __cplusplus
}
#endif

#endif
