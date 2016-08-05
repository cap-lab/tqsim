#ifndef __HSIM_PACKET_H
#define __HSIM_PACKET_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

typedef enum {
	packet_read,		/* blocking */
	packet_write,		/* blocking */
	packet_iread,		/* non-blocking */
	packet_iwrite,		/* non-blocking */
	packet_elapsed,
	packet_syscall
} PacketType;

typedef struct _Packet{
	PacketType type;	
	uint32_t size;
	uint64_t cycle;
	uint32_t address;
	uint8_t data[8];
	uint32_t flags;
} Packet;

#ifdef __cplusplus
}
#endif

#endif
