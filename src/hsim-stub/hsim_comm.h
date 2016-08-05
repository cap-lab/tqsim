#ifndef __HSIM_COMM_H
#define __HSIM_COMM_H

#include "hsim_common.h"
#include "hsim_packet.h"

typedef enum {
	none_comm_mode,
	pipe_comm_mode,
	socket_comm_mode,
	shmem_comm_mode
} hsim_comm_mode;


/* socket */

int hsim_socket_open(int nPort);
int hsim_socket_wait_connection(int fd);
int hsim_socket_listen_and_accept(unsigned int port);
int hsim_socket_connect(const char *host, unsigned int port);
void *establish_shm_segment(char *shmem_name, int size);
void comm_send(Packet * pPacket);
void comm_sync(uint64_t cycle);
void comm_close(uint64_t cycle);
int comm_recv(Packet * pPacket);

void comm_send_data(uint8_t* buf, int size);
void comm_recv_data(uint8_t* buf, int size);

void comm_initialize(hsim_comm_mode _comm_mode);
void hsim_shmem_initialize(void);
void hsim_shmem_close(void);
void hsim_socket_initialize(void);
void hsim_pipe_initialize(void);
void comm_destroy(uint64_t cycle);

void *get_sysreg(void);

#endif
