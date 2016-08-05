#include "packet_buffer.h"
#include <string.h>

void pb_init (PacketBuffer * pb) {
    pb->start = 0;
    pb->end = 0;
}
int pb_is_full (PacketBuffer * pb) {
    return (pb->end + 1) % PKT_BUFFER_SIZE == pb->start;
}
int pb_is_empty (PacketBuffer * pb) {
    return pb->end == pb->start;
}
int pb_write (PacketBuffer * pb, Packet * elem) {
    memcpy(&(pb->elems[pb->end]), elem, sizeof(Packet));
    pb->end = (pb->end + 1) % PKT_BUFFER_SIZE;
	return sizeof(Packet);
}
int pb_read (PacketBuffer * pb, Packet * elem) {
    memcpy(elem, &(pb->elems[pb->start]),  sizeof(Packet));
    pb->start = (pb->start + 1) % PKT_BUFFER_SIZE;
	return sizeof(Packet);
}
