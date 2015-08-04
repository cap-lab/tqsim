#ifndef esim_type_h
#define esim_type_h

#include <ctype.h>
#include <inttypes.h>

#define MIN_SYNC_CYCLE		1000000

typedef uint64_t Cycle;
typedef uint32_t Address;	//assume 32bit machine

typedef struct packet {
	uint32_t size;
	uint32_t type;
	uint64_t cycle;
	uint32_t address;
	uint8_t data[64];
	union {
		uint64_t latency;
		uint64_t flag;
	} param;
	uint32_t needFeedback;
	uint32_t interrupt;
	uint32_t reserved;
} packet;

#ifndef TRUE
#define FALSE 	0
#define TRUE	1
#endif

#endif
