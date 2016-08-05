#ifndef __PACKET_BUFFER_H
#define __PACKET_BUFFER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "hsim_packet.h"
#define PKT_BUFFER_SIZE 0x1000

typedef struct {
    volatile int start;			/* index of oldest element              */
    volatile int end;			/* index at which to write new element  */
    Packet elems[PKT_BUFFER_SIZE+1];		/* vector of elements                   */
} PacketBuffer;

void pb_init (PacketBuffer * pb);
int pb_is_full (PacketBuffer * pb);
int pb_is_empty (PacketBuffer * pb);
int pb_write (PacketBuffer * pb, Packet * elem);
int pb_read (PacketBuffer * pb, Packet * elem);

#ifdef __cplusplus
}
#endif

#endif
