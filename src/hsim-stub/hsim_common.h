#ifndef __hsim_common_h
#define __hsim_common_h

#include <stdint.h>
/*
#ifndef TRUE
	#define TRUE 1
#endif
#ifndef FALSE
	#define FALSE 0
#endif
*/
typedef uint32_t Address;
typedef enum {NonBlockingAccess, BlockingAccess, InstAccess} AccessType;


#endif
