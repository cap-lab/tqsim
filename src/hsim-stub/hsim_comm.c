#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "hsim_comm.h"
#include "hsim_packet.h"
#include "hsim_syscall.h"
#include "spm_buffer.h"
#include "packet_buffer.h"

hsim_comm_mode comm_mode;

PacketBuffer *sendBuffer;
PacketBuffer *recvBuffer;
SPMBuffer *sharedBuffer;					//for massive communication
uint32_t *sys_registers;
extern uint64_t num_insts;

//communication 
int comm_socket = 0;
int comm_pipe_snd = 0;
int comm_pipe_rcv = 0;

extern char comm_id[64];
char ib_name[64], bi_name[64], shmem_name[64];
char sysreg_name[64];

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

int hsim_socket_open(int nPort)
{
	// Open a TCP socket
	int sId;

	if ((sId = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("cannot open stream socket.\n");
		return 0;
	}

	/*
	 * bind local address 
	 */
	struct sockaddr_in servaddr;
	int temp;

	bzero((char *) &servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(nPort);
	temp = bind(sId, (struct sockaddr *) &servaddr, sizeof(servaddr));

	if (temp < 0) {
		printf("bind error. port number %d is not available\n", nPort);
		return 0;
	}

	if (listen(sId, 5) < 0) {
		printf("listen error\n");
		return 0;
	}

	return sId;
}

int hsim_socket_wait_connection(int fd)
{
	struct sockaddr_in far_addr;

	int len = sizeof(far_addr);
	int socketId =
		(int) accept(fd, (struct sockaddr *) &far_addr,
				(socklen_t *) & len);

	if (socketId <= 0) {
		printf("accept error\n");
		return 0;
	} else {
		int flag = 1;
		int ret =
			setsockopt(socketId, IPPROTO_TCP, TCP_NODELAY, (char *) &flag,
					sizeof(flag));
		if (ret == -1)
			printf("Couldn't setsockopt(TCP_NODELAY)\n");
	}

	return socketId;
}



int hsim_socket_listen_and_accept(unsigned int port)
{
	int fd = hsim_socket_open(port);

	if (fd <= 0) {
		printf("couldn't open port : %d\r\n", port);
		return 0;
	}

	comm_socket = hsim_socket_wait_connection(fd);
	if (comm_socket > 0)
		return 1;
	else
		return 0;
}


int hsim_socket_connect(const char *host, unsigned int port)
{
	int sId, timeout;
	struct sockaddr_in addr;

	for (timeout = 0;; timeout++) {
		if ((sId = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			//ERROR("cannot open socket for")
			return -1;
		}

		memset((char *) &addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = inet_addr(host);
		addr.sin_port = htons(port);

		if (!connect(sId, (struct sockaddr *) &addr, sizeof(addr))) {
			int flag = 1;
			int ret =
				setsockopt(sId, IPPROTO_TCP, TCP_NODELAY, (char *) &flag,
						sizeof(flag));
			if (ret == -1)
				printf("Couldn't setsockopt(TCP_NODELAY)\n");

			break;
		}

		//	sleep(1);

		if (timeout > 3) {
			return -1;
		}
		close(sId);
	}

	comm_socket = sId;

	return 0;
}

void comm_send_data(uint8_t* buf, int size){
	int latency = 1;
	while (sp_is_full(sharedBuffer)) {
		usleep(latency);
	};
	sp_write(sharedBuffer, buf, size);

}

void comm_recv_data(uint8_t *buf, int size){
	int latency = 1;
	while (sp_is_empty(sharedBuffer)){
		usleep(latency);
	};
	sp_read(sharedBuffer, buf, size);
}

void comm_send(Packet * pPacket)
{
	int fd = 0;
	switch (comm_mode) {
		case pipe_comm_mode:
			fd = comm_pipe_snd;
			break;

		case socket_comm_mode:
			fd = comm_socket;
			break;

		default:
			fd = 0;
		//	return;
	}

	if (comm_mode == pipe_comm_mode || comm_mode == socket_comm_mode) {

		int nLength = sizeof(Packet);
		char *pPos = (char *) pPacket;

		while (nLength > 0) {
			int nSend = write(fd, pPos, nLength);
			pPos += nSend;
			nLength -= nSend;
		}
	} else if (comm_mode == shmem_comm_mode) {
		int latency = 1;
		while (pb_is_full(sendBuffer)) {
			usleep(latency);
		};
		pb_write(sendBuffer, pPacket);
	}
}

void comm_sync(uint64_t cycle)
{
	Packet myPacket;
	myPacket.type = packet_elapsed;
	myPacket.address = 0;
	myPacket.cycle = cycle;
	comm_send(&myPacket);
}




void comm_close(uint64_t cycle)
{
	Packet myPacket;

	myPacket.type = packet_syscall;
	myPacket.cycle = cycle;
	
	uint64_t *dst = (uint64_t*)sys_registers;
	dst[0] = num_insts;
	sys_registers[NUM_SYS_REG-1] = syscall_exit;
//	memcpy(myPacket.data, reg, sizeof(uint32_t)*NUM_SYS_REG);
	comm_send(&myPacket);
	

	switch (comm_mode) {
		case pipe_comm_mode:
			close(comm_pipe_snd);
			close(comm_pipe_rcv);
			break;

		case socket_comm_mode:
			close(comm_socket);
			break;

		case shmem_comm_mode:
			hsim_shmem_close();
			break;

		default:
			break;
	}


}



int comm_recv(Packet * pPacket)
{
	int fd = 0;
	switch (comm_mode) {
		case pipe_comm_mode:
			fd = comm_pipe_rcv;
			break;

		case socket_comm_mode:
			fd = comm_socket;
			break;

		case shmem_comm_mode:
			break;

		default:
			return -1;
	}

	if (comm_mode == pipe_comm_mode || comm_mode == socket_comm_mode) {


		int nLength = sizeof(Packet);
		char *pBuf = (char *) pPacket;


		while (nLength > 0) {
			int nRecv = read(fd, pBuf, nLength);

			if (nRecv > 0) {
				pBuf += nRecv;
				nLength -= nRecv;
			}
		}
		return TRUE;
	} else if (comm_mode == shmem_comm_mode) {

		int latency = 1;
		while (pb_is_empty(recvBuffer)){
			usleep(latency);
		};


		pb_read(recvBuffer, pPacket);
		return TRUE;
	}
	return FALSE;
}



void comm_initialize(hsim_comm_mode _comm_mode)
{

	printf("comm_initialize...\n");
	comm_mode = _comm_mode;

	switch (comm_mode) {
		case pipe_comm_mode:
			hsim_pipe_initialize();
			break;

		case socket_comm_mode:
			hsim_socket_initialize();
			break;

		case shmem_comm_mode:
			hsim_shmem_initialize();
			break;

		default:
			break;

	}
}

void *establish_shm_segment(char *shmem_name, int size)
{
	int fd;
	void *addr;
	fd = shm_open(shmem_name, O_RDWR, 0600);
	if (fd < 0)
		fprintf(stderr, "shm_open fails with %s\n", shmem_name);
	if (ftruncate(fd, size) < 0)
		fprintf(stderr, "ftruncate() shared memory segment\n" );
	addr = (void *) mmap(NULL, size, PROT_READ | PROT_WRITE,	MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED)
		fprintf(stderr, "mapping shared memory segment\n");

	close(fd);
	return addr;
}



void hsim_shmem_initialize(void)
{
	int ret = 0;
	const char *strFilename = ".args.shmem.dat";
	FILE *fp;
	fp = fopen(strFilename, "r");

	if (fp != NULL) {

		ret |= fscanf(fp, "%s", comm_id);
		ret |= fscanf(fp, "%s", ib_name);
		ret |= fscanf(fp, "%s", bi_name);
		ret |= fscanf(fp, "%s", shmem_name);
		ret |= fscanf(fp, "%s", sysreg_name);

		
//		printf("comm id [%s] \n", comm_id);
//		printf("send buffer [%s] | recv buffer [%s]\n", ib_name, bi_name);
		sendBuffer = establish_shm_segment(ib_name, sizeof(PacketBuffer));
		recvBuffer = establish_shm_segment(bi_name, sizeof(PacketBuffer));
		sharedBuffer = establish_shm_segment(shmem_name, sizeof(SPMBuffer));
		sys_registers = establish_shm_segment(sysreg_name, sizeof(uint32_t)*8);


		{
			Packet pkt;
			pkt.type = packet_syscall;
			pkt.size = sizeof(Packet) - sizeof(int);
//			uint32_t regs[NUM_SYS_REG];
			sys_registers[NUM_SYS_REG-1] = syscall_begin;
//			memcpy(pkt.data, regs, sizeof(uint32_t)*NUM_SYS_REG);
			comm_send(&pkt);
		}

		printf("Shared Memory is Established\n");

	} else
		printf("No Shared Memory\n");

}

void hsim_shmem_close(void)
{
	munmap(sendBuffer, sizeof(PacketBuffer));
	munmap(recvBuffer, sizeof(PacketBuffer));
	munmap(sharedBuffer, sizeof(SPMBuffer));
	munmap(sys_registers, sizeof(uint32_t)*8);
	shm_unlink(ib_name);
	shm_unlink(bi_name);
	shm_unlink(shmem_name);
	shm_unlink(sysreg_name);
}

void *get_sysreg(void){
	return sys_registers;
}

void hsim_socket_initialize(void)
{
	int ret = 0;
	printf("socket_initialize...\n");
	char strHost[64], buf[64], strXMLname[64];
	int port = 0;
	const char *strFilename = ".args.sock.dat";
	FILE *fp;

	fp = fopen(strFilename, "r");

	if (fp != NULL) {

		ret |= fscanf(fp, "%s", strHost);
		ret |= fscanf(fp, "%s", buf);
		ret |= fscanf(fp, "%s", strXMLname);
		ret |= fscanf(fp, "%s", comm_id);

		port = atoi(buf);
		fclose(fp);
		printf("Reading a socket configuration file...\n");
		if (strcmp(strHost, "0") == 0) {
			printf("\r\nwaiting kernel(%d)...", port);
			fflush(stdout);
			if (hsim_socket_listen_and_accept(port))
				printf("Connected\r\n");
			else
				printf("Connection Fail\r\n");
		} else {
			printf("\r\nconnecting kernel(%s:%d)...", strHost, port);
			fflush(stdout);

			if (hsim_socket_connect(strHost, port))
				printf("Connected\r\n");
			else
				printf("Connection Fail\r\n");
		}
	} else
		printf("No Socket\n");
}


void hsim_pipe_initialize(void)
{
	FILE *fp;
	int ret = 0;
	char filename_pipe[32];
	sprintf(filename_pipe, ".args.pipe.dat");
	fp = fopen(filename_pipe, "r");
	if (fp != NULL) {
		printf("pipe initialization\n");
		char pipe_r[16], pipe_w[16], tileId[16], coreId[16];

		ret |= fscanf(fp, "%s", pipe_w);
		ret |= fscanf(fp, "%s", pipe_r);
		ret |= fscanf(fp, "%s", tileId);
		ret |= fscanf(fp, "%s", coreId);

		fclose(fp);

		printf(", try to open PIPE Connection %s %s...\n", pipe_w, pipe_r);

		comm_pipe_rcv = open(pipe_r, O_RDONLY);
		comm_pipe_snd = open(pipe_w, O_WRONLY);


		// OPEN NONBLOCKING MODE
		//fcntl(comm_pipe_rcv, F_SETFL, fcntl(comm_pipe_rcv, F_GETFL) | O_NONBLOCK);

		printf(", Open PIPE Connection\n");
		{
			Packet pkt;
			pkt.type = packet_syscall;
			uint32_t regs[NUM_SYS_REG];
			regs[NUM_SYS_REG-1] = syscall_begin;
			memcpy(pkt.data, regs, sizeof(uint32_t)*NUM_SYS_REG);
			pkt.size = sizeof(Packet) - sizeof(int);
			comm_send(&pkt);
			comm_recv(&pkt);

		}
	} else
	{
		printf("No PIPE\n");
	}
}


void comm_destroy(uint64_t cycle)
{
	comm_close(cycle);
}


