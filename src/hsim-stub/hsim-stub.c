#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <sched.h>
#include <sys/time.h>
#include <time.h>
//#include "vcache.h"
#include "hsim-stub.h"
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>

#include "ringbuffer.h"
//#include "perfcounter.h"
#include "cache.h"

#define SYSTEM_REGISTER_BASE	0xE0000000
#define SYSTEM_REGISTER_SIZE	0x00001000	// 4K

#define PERF_REGISTER_BASE 0xF0000000
#define PERF_REGISTER_SIZE	0x00001000	// 4K

#define MEM_BASE 0x30000000

Cache_t *vcache;


static char comm_id[64];

static uint64_t totalTime = 0;
//static uint64_t blockingTime = 0;

#define BILLION 1000000000L

struct timespec total_start, total_end;

long startTime;
long commTime;


Cycle esim_cpu_cycle = 0;

static int base = 0;
int esim_interrupt_period = -1;

Cycle esim_synced_cycles = 0;
Cycle esim_current_cycles = 0;


static int msg_group_enabled = 0;
static int single_processor = 0;

static int msg_group_size = 100;

static int comm_count = 0;
static int comm_message = 0;

int comm_mode = SHMEM_COMM_MODE;

//communication 
int comm_socket = 0;
int comm_pipe_snd = 0;
int comm_pipe_rcv = 0;

CircularBuffer *sendBuffer;
CircularBuffer *recvBuffer;
//PerfCounter *perfBuffer;



mem_region *mem_region_head = NULL;

//shkang
int tile_id;
int core_id;

int comm_interrupt_sync_period = 20;
int comm_notice_period = 500;

long long comm_sum_wait_write = 0;
long long comm_sum_wait_read = 0;
long long comm_num_sync = 0;
struct timeval tvStart, tvEnd;

struct channel_ctrl_t {
    unsigned int id;
    unsigned int size;
    unsigned int address;
    unsigned int cmd;
} channel_ctrl;

void *establish_shm_segment(char *shmem_name);

void set_affinity(int alloc_id)
{

    cpu_set_t set;
    int ret;
    __CPU_ZERO_S(sizeof(cpu_set_t), &set);
    __CPU_SET_S(alloc_id, sizeof(cpu_set_t), &set);
    ret = sched_setaffinity(0, sizeof(cpu_set_t), &set);
    if (ret == 0)
		printf(", Sched on CPU#%d\n", alloc_id);
	else 
		printf(", Failed to sche on CPU%d\n", alloc_id);
}

void HsimStart(void)
{
    printf("HsimStart\n");
    esim_initialize();
}

void HsimEnd(Cycle cycle)
{

    printf("HsimEnd\n");

    esim_current_cycles += (cycle - esim_cpu_cycle);
    esim_cpu_cycle = cycle;
    esim_destroy(esim_current_cycles - esim_synced_cycles);
    esim_synced_cycles = esim_current_cycles;
}


void esim_initialize(void)
{
    esim_cpu_cycle = 0;

	clock_gettime(CLOCK_MONOTONIC, &total_start);

    comm_initialize();
    mem_initialize();

    if (comm_mode != NONE_COMM_MODE) {
	comm_read_env();
    }
	
	vcache = (Cache_t*) malloc(sizeof(Cache_t));
	cache_initialize(vcache, "vcache",1024 , 64, 128, LRU, 1, 10);

}

void esim_destroy(Cycle cycle)
{
    clock_gettime(CLOCK_MONOTONIC, &total_end);	/* mark the end time */
    totalTime =
	BILLION * (total_end.tv_sec - total_start.tv_sec) +
	total_end.tv_nsec - total_start.tv_nsec;
//    printf("%" PRIu64 " / %" PRIu64 "\n", blockingTime, totalTime);
//    printf("total simulated cycle = %" PRIu64 "\n", esim_cpu_cycle);

    comm_destroy(cycle);
    mem_destroy();
    if (msg_group_enabled && comm_count > 0) {
	float rate = (float) comm_message / (float) comm_count;

	printf("message grouping rate : %.2f\r\n", rate);
    }
	cache_close(vcache);

//    getchar();
}

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
	    return FALSE;
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

	sleep(1);
	printf(".");
	fflush(stdout);

	if (timeout > 3) {
	    return FALSE;
	}
	close(sId);
    }

    comm_socket = sId;

    return TRUE;
}


void comm_send(packet * pPacket)
{
	//fprintf(stderr, "%x\n", pPacket->type);
    int fd = 0;
    switch (comm_mode) {
    case PIPE_COMM_MODE:
	fd = comm_pipe_snd;
	break;

    case SOCKET_COMM_MODE:
	fd = comm_socket;
	break;

    case SHMEM_COMM_MODE:
	break;

    default:
	return;
    }


	if (comm_mode == PIPE_COMM_MODE || comm_mode == SOCKET_COMM_MODE) {

		int nLength = sizeof(packet);
		char *pPos = (char *) pPacket;

		while (nLength > 0) {
			int nSend = write(fd, pPos, nLength);
			pPos += nSend;
			nLength -= nSend;
		}
	} else if (comm_mode == SHMEM_COMM_MODE) {
		//	if (pPacket->address != 0)
		//		printf ("try to send a message...%X\n", (unsigned)pPacket->address);
		//getchar();	
//		struct timespec tim, tim2;
//		tim.tv_sec = 0;
//		tim.tv_nsec = 1;
	
		int latency = 1;
		while (cbIsFull(sendBuffer)) {
			usleep(latency);
			//nanosleep(&tim, &tim2); 
//			latency *= 2; 
//			if (latency > 100) 	latency = 1;
//			tim.tv_nsec = latency;
		};
		cbWrite(sendBuffer, pPacket);
	}
}



void comm_send_data(const char *pSrc, int nSize)
{

	int fd = 0;
	switch (comm_mode) {
		case PIPE_COMM_MODE:
			fd = comm_pipe_snd;
			break;

		case SOCKET_COMM_MODE:
			fd = comm_socket;
			break;

		case SHMEM_COMM_MODE:
			break;

		default:
			return;
	}

	if (comm_mode == PIPE_COMM_MODE || comm_mode == SOCKET_COMM_MODE) {
		int nLength = nSize;
		const char *pPos = pSrc;

		while (nLength > 0) {
			int nSend = write(fd, pPos, nLength);

			if (nSend > 0) {
				pPos += nSend;
				nLength -= nSend;
			}
		}

	}
//	 else if (comm_mode == SHMEM_COMM_MODE) {
//		memcpy((void*)perfBuffer, pSrc, nSize);
//	}
}



void comm_sync(Cycle cycle)
{
    packet myPacket;

    myPacket.type = PKT_ELAPSED | PKT_UNREAL;
    myPacket.address = 0;
	myPacket.cycle = cycle;
	//myPacket.needFeedback = 0;
	comm_send(&myPacket);
}



void comm_close(Cycle cycle)
{
    packet myPacket;

    myPacket.type = PKT_EXIT | PKT_UNREAL;
    myPacket.cycle = cycle;
    comm_send(&myPacket);

    switch (comm_mode) {
    case PIPE_COMM_MODE:
	close(comm_pipe_snd);
	close(comm_pipe_rcv);
	break;

    case SOCKET_COMM_MODE:
	close(comm_socket);
	break;

    case SHMEM_COMM_MODE:
	shmem_close();
	break;

    default:
	break;
    }


}



int comm_recv(packet * pPacket)
{
    //printf("r\t%d\n",  (int)esim_cpu_cycle);
    int fd = 0;
    switch (comm_mode) {
    case PIPE_COMM_MODE:
	fd = comm_pipe_rcv;
	break;

    case SOCKET_COMM_MODE:
	fd = comm_socket;
	break;

    case SHMEM_COMM_MODE:
	break;

    default:
	return FALSE;
    }

    if (comm_mode == PIPE_COMM_MODE || comm_mode == SOCKET_COMM_MODE) {


	int nLength = sizeof(packet);
	char *pBuf = (char *) pPacket;

	//      int waitC = 0;

	while (nLength > 0) {
	    int nRecv = read(fd, pBuf, nLength);

	    if (nRecv > 0) {
		pBuf += nRecv;
		nLength -= nRecv;
	    }
	}
	return TRUE;
    } else if (comm_mode == SHMEM_COMM_MODE) {
//		struct timespec tim, tim2;

//		tim.tv_sec = 0;
//		tim.tv_nsec = 1;

		int latency = 1;
		while (cbIsEmpty(recvBuffer)){
			usleep(latency);
			//nanosleep(&tim, &tim2); 
//			latency *= 2; 
//			if (latency > 100) 	latency = 1;

//			tim.tv_nsec = latency;
		};


	cbRead(recvBuffer, pPacket);
	return TRUE;
    }
    return FALSE;
}


int comm_recv_data(uint8_t * pDst, int nSize)
{

    int fd = 0;
    switch (comm_mode) {
    case PIPE_COMM_MODE:
	fd = comm_pipe_rcv;
	break;

    case SOCKET_COMM_MODE:
	fd = comm_socket;
	break;

    case SHMEM_COMM_MODE:
	break;

    default:
	return FALSE;
    }
	if (comm_mode == PIPE_COMM_MODE || comm_mode == SOCKET_COMM_MODE) {

    int nLength = nSize;
    uint8_t *pBuf = pDst;

    while (nLength > 0) {
	int nRecv = read(fd, pBuf, nLength);

	if (nRecv > 0) {
	    pBuf += nRecv;
	    nLength -= nRecv;
	}
    }
    return TRUE;
	}
//	else if (comm_mode == SHMEM_COMM_MODE){
//		memcpy(pDst, perfBuffer, nSize);
		//return TRUE;
//	}
	return FALSE;
}

extern void update_interrupt(void *top, int n_int);

int comm_check_interrupt(Cycle cycle)
{
    packet my_packet;

    my_packet.cycle = cycle;
    my_packet.type = PKT_INTERRUPT | PKT_UNREAL;
    comm_send(&my_packet);
    comm_recv(&my_packet);
    return my_packet.interrupt;
}


void mem_initialize(void)
{
    mem_region_head = NULL;
}



void mem_destroy(void)
{
    mem_region *pRegion = mem_region_head;

    while (pRegion != NULL) {
	mem_region *pDel = pRegion;

	pRegion = pRegion->next;
	free(pDel);
    }
}



	//mem_region is connected as a linked list

void mem_add_region(mem_region * pRegion)
{
    pRegion->prev = NULL;
    pRegion->next = NULL;

    if (mem_region_head == NULL) {
	// if it is head
	mem_region_head = pRegion;
    } else {
	// connect it to tail
	mem_region *prevRegion = mem_region_head;

	while (prevRegion->next != NULL)
	    prevRegion = prevRegion->next;
	prevRegion->next = pRegion;
	pRegion->prev = prevRegion;
    }
}


//temporary
int mem_is_local(Address addr)
{
    if (addr < 0x30000000)
	return 1;
    return 0;
}

int mem_is_shared(Address addr)
{

	if (addr >= SYSTEM_REGISTER_BASE && addr < (SYSTEM_REGISTER_BASE + SYSTEM_REGISTER_SIZE)) {
		return MEM_TYPE_INTERNAL;
	}

	if (addr >=PERF_REGISTER_BASE && addr < (PERF_REGISTER_BASE + PERF_REGISTER_SIZE)) {
		return MEM_TYPE_PERF;
	}

	mem_region *pRegion = mem_find_mem_region(addr);

	if (pRegion != NULL)
		return pRegion->type;
	else
		return 0;
}



mem_region *mem_find_mem_region(Address addr)
{
    mem_region *pRegion = mem_region_head;

    while (pRegion != NULL) {
	if (addr >= pRegion->base && addr < pRegion->base + pRegion->size)
	    break;
	pRegion = pRegion->next;
    }
    // pull up recent region to top
    if (pRegion != NULL && pRegion != mem_region_head) {
	// remove selected item
	pRegion->prev->next = pRegion->next;
	if (pRegion->next != NULL)
	    pRegion->next->prev = pRegion->prev;
	// insert selected to top
	pRegion->next = mem_region_head;
	mem_region_head->prev = pRegion;
	mem_region_head = pRegion;
    }
    return pRegion;
}

void comm_read_env(void)
{
    char fileName[64];
    FILE *fp;

    printf("comm_read_env...\n");
    sprintf(fileName, ".env_%s.dat", comm_id);
    //for tile
//      sprintf(fileName, ".env_tile_%d.dat", tile_id);
//      sprintf(fileName, ".env_ARM0.dat", tile_id);
    printf("Comm file name = %s\n", fileName);

    fp = fopen(fileName, "r");

    if (fp != NULL) {
	char buf[128];

	while (1) {
	    int i;
	    char *key = 0, *value = 0, *result;

	    result = fgets(buf, 128, fp);
	    if (buf[0] == '#')
		continue;
	    else if (result == NULL)
		break;

	    key = buf;
	    for (i = 0; i < 128; i++) {
		if (buf[i] == '=') {
		    buf[i] = '\0';
		    value = &buf[i + 1];
		} else if (buf[i] == '\0' || buf[i] == '\n') {
		    buf[i] = '\0';
		    break;
		}
	    }
	    if (value == 0)
		continue;

	    if (strcmp(key, "SchedulePeriod") == 0) {
		printf("-- set schedule period : %s\n", value);
		comm_interrupt_sync_period = atoi(value);
	    } else if (strcmp(key, "NoticePeriod") == 0) {
		printf("-- set notice period : %s\n", value);
		comm_notice_period = atoi(value);
	    } else if (strcmp(key, "InterruptPeriod") == 0) {
		printf("-- set interrupt period : %s\n", value);
		esim_interrupt_period = atoi(value);
	    } else if (strcmp(key, "SimulationCacheSize") == 0) {
		int size = atoi(value);

		if (size > 0) {
		    printf("-- use simulation cache : %d byte(s)\n",
			   1 << size);
		}
	    } else if (strcmp(key, "MessageGroupingSize") == 0) {
		int size = atoi(value);

		if (size > 0) {
		    printf("-- use message grouping : %d\n", size);
		    msg_group_enabled = 1;
		    msg_group_size = size;
		}
	    } else if (strcmp(key, "UseMultiCore") == 0) {
		printf("-- use multi core : %d\n", atoi(key));
		single_processor = (strcmp(value, "true") == 0) ? 1 : 0;

	    } else if (strcmp(key, "Memorymap") == 0) {
		int index = 0;
		char *lpszData[4];
		unsigned int arData[4];

		lpszData[index++] = value;
		for (i = 0; i < 128; i++) {
		    if (value[i] == ':') {
			value[i] = '\0';
			lpszData[index++] = &value[i + 1];
		    } else if (value[i] == '\0')
			break;
		}
		for (i = 0; i < 4; i++)
		    arData[i] = atoi(lpszData[i]);
		printf
		    ("-- add memory region (0x%08x:0x%08x, latency:%d, type:%d)\n",
		     arData[0], arData[0] + arData[1] - 1, arData[2],
		     arData[3]);

		mem_region *pRegion =
		    (mem_region *) malloc(sizeof(mem_region));

		pRegion->base = arData[0];
		base = pRegion->base;
		pRegion->size = arData[1];
		pRegion->type = arData[3];
		mem_add_region(pRegion);

	    } else if (strcmp(key, "ProcessorId") == 0) {
		core_id = atoi(value);
		printf("-- set core iD : %d\n", core_id);
	    }

	    /*else if (strcmp(key, "TileId") == 0) {
	       tile_id = atoi(value);
	       printf("-- set tile iD : %d\n", tile_id);
	       }
	     */
	    else
		printf("Unknown configuration : %s\n", key);
	}
	fclose(fp);
    }
    // mapping to core
    {
	int alloc_proc = 0;
	int num_total_proc =  sysconf(_SC_NPROCESSORS_ONLN);

	//SHKANG:PARALLEL?
	single_processor = 0;
	
	if (!single_processor) {
	    alloc_proc = (core_id% (num_total_proc-1))+1;
	} else {
	    alloc_proc = 0;
	}
	set_affinity(alloc_proc);
    }
}

void comm_initialize(void)
{
    printf("comm_initialize...\n");

    switch (comm_mode) {
    case PIPE_COMM_MODE:
	pipe_initialize();
	break;

    case SOCKET_COMM_MODE:
	socket_initialize();
	break;

    case SHMEM_COMM_MODE:
	shmem_initialize();
	break;

    default:
	break;

    }
}

void *establish_shm_segment(char *shmem_name)
{
    int fd;
    int size = sizeof(CircularBuffer);
    CircularBuffer *addr;

//    fd = shm_open(shmem_name, O_RDWR | O_CREAT, 0600);
    fd = shm_open(shmem_name, O_RDWR, 0600);

	if (fd < 0)
	fprintf(stderr, "shm_open fails with %s\n", shmem_name);
    if (ftruncate(fd, size) < 0)
         fprintf(stderr, "ftruncate() shared memory segment\n" );
    addr =
	(CircularBuffer *) mmap(NULL, size, PROT_READ | PROT_WRITE,	MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED)
	fprintf(stderr, "mapping shared memory segment\n");

    close(fd);
    return addr;
}


char ib_name[64], bi_name[64], shmem_name[64];

void shmem_initialize(void)
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

	printf("send buffer [%s] | recv buffer [%s]\n", ib_name, bi_name);
	sendBuffer = establish_shm_segment(ib_name);
	recvBuffer = establish_shm_segment(bi_name);
	//perfBuffer = establish_shm_segment(shmem_name);
	//send starting packet;
	//packet p;
	//comm_send(&p);
	{
	    packet pkt;
	    pkt.type = PKT_BEGIN | PKT_UNREAL;
	    pkt.size = sizeof(packet) - sizeof(int);
	    comm_send(&pkt);
	}

	printf("Shared Memory is Established\n");

    } else
	printf("No Shared Memory\n");

}

void shmem_close(void)
{
    munmap(sendBuffer, sizeof(CircularBuffer));
    munmap(recvBuffer, sizeof(CircularBuffer));
//	munmap(perfBuffer, sizeof(PerfCounter));
    shm_unlink(ib_name);
    shm_unlink(bi_name);
	shm_unlink(shmem_name);
}

void socket_initialize(void)
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


void pipe_initialize(void)
{
    FILE *fp;
    int ret = 0;
    char filename_pipe[32];
    printf("tile_id: %d\n", tile_id);
    sprintf(filename_pipe, ".args.pipe_%d.dat", tile_id);
    fp = fopen(filename_pipe, "r");
    if (fp != NULL) {
	printf("pipe initialization\n");
	char pipe_r[16], pipe_w[16], tileId[16], coreId[16];

	ret |= fscanf(fp, "%s", pipe_w);
	ret |= fscanf(fp, "%s", pipe_r);
	ret |= fscanf(fp, "%s", tileId);
	ret |= fscanf(fp, "%s", coreId);

	//printf("pipe_initialization error\n");
	fclose(fp);
	//tile_id = atoi(tileId);
	core_id = atoi(coreId);

	printf(", try to open PIPE Connection %s %s %d...\n", pipe_w,
	       pipe_r, core_id);

	comm_pipe_rcv = open(pipe_r, O_RDONLY);
	comm_pipe_snd = open(pipe_w, O_WRONLY);


	// OPEN NONBLOCKING MODE
	//fcntl(comm_pipe_rcv, F_SETFL, fcntl(comm_pipe_rcv, F_GETFL) | O_NONBLOCK);

	printf(", Open PIPE Connection\n");
	{
	    packet pkt;
	    pkt.type = PKT_BEGIN | PKT_UNREAL;
	    pkt.size = sizeof(packet) - sizeof(int);
	    comm_send(&pkt);

	}
/*
        {
            int alloc_proc = 0;

            if (!single_processor)
                alloc_proc = (core_id + 1) % 8;
            set_affinity(alloc_proc);
        }
 */
    } else
	printf("No PIPE\n");
}


void comm_destroy(Cycle cycle)
{
    comm_close(cycle);


//    printf("[Info] Total Sync Wait : %lld\r\n", comm_sum_wait_write);
//    if (comm_num_sync > 0)
//	printf("[Info] Average Wait    : %lld\r\n", comm_sum_wait_write / comm_num_sync);
}

Cycle mem_access(Cycle cycle, Address address, int rw, uint64_t * data, int size, int unreal)
{
	int useVcache = false;
	int isMiss;

	int sizeInByte =  1 << (size - 3);

	//FIXME
	if (0x30000000 <= address && address < 0x50000000){
		useVcache = true;
	}

	useVcache  = 0;
	if (useVcache)
		isMiss = cache_access_data(cycle, vcache, rw?Write:Read, address, (uint8_t*)data, sizeInByte);
    
	packet my_packet;
    my_packet.cycle = cycle;
    my_packet.reserved = core_id;
	my_packet.param.flag = 0;

	if (useVcache && isMiss) {	//read full block
		    my_packet.address = address & vcache->addr_mask;
		    my_packet.size = vcache->config.bsize;
		    my_packet.type = PKT_READ;
			my_packet.needFeedback = 1;
//			printf("Vcache miss\n");
	}
	else {
	    my_packet.address = address;
	    my_packet.size = sizeInByte;
	    my_packet.type = rw?PKT_WRITE:PKT_READ;
		if (rw){
			memcpy(my_packet.data, data, sizeInByte);
		}
		if (useVcache && !isMiss){		//vcache hit case
//			printf("Vcache hit\n");
			my_packet.needFeedback = 0;
		}
		else {							//normal access
//			printf("Normal access\n");
			my_packet.needFeedback = 1;
		}
	}

	//send
	//printf("sending... %d\n", (int)my_packet.needFeedback);
	comm_send(&my_packet);

	if (my_packet.needFeedback){
		comm_recv(&my_packet);

		if (useVcache){
	//		printf("Cache update\n");
			cache_update_data(cycle, vcache, my_packet.address, (uint8_t*)my_packet.data); 
			cache_access_data(cycle, vcache, rw?Write:Read, address, (uint8_t*)data, sizeInByte);
		}
		else {
			if (!rw){
				memcpy(data, my_packet.data, sizeInByte);
			}
		}
	}

	// blocked communication
	esim_synced_cycles = esim_current_cycles;

	//CHECK ME
    return my_packet.param.latency;
}

/*(
Cycle mem_access_old(Cycle cycle, Address address, int rw, uint64_t * data, int size, int unreal)
{

    packet my_packet;

    my_packet.cycle = cycle;
    my_packet.address = address;
    my_packet.size = 1 << (size - 3);
    //my_packet.size = size;
    my_packet.reserved = core_id;

    //normal access
	my_packet.param.flag = 0;
	// normal processing
	if (rw) {
	    // WRITE
	    switch (size) {
	    case 3:		// BYTE
	    case 4:		// SHORT
	    case 5:		// WORD
		my_packet.data = *data;
		break;
		
	    case 6:		// 2-WORD
		memcpy(my_packet.largeData, data, 8);
		break;
	    case 7:		// 4-WORD
		memcpy(my_packet.largeData, data, 16);
		break;
	    case 8:		// 8-WORD
		memcpy(my_packet.largeData, data, 32);
		break;
	    case 9:		// 16-WORD
		memcpy(my_packet.largeData, data, 64);
		break;
	
	    }
	    my_packet.type = PKT_WRITE;
	} else {
	    // READ
	    my_packet.type = PKT_READ;
	}

	//      printf("mem access(%d) : 0x%x\n", (int)my_packet.cycle, (unsigned int)my_packet.address);

	// if response is not needed, on PKT_UNREAL
	{
	    long long diff = 0;

	    ///////////////////////////////////////////////////
	    // send and receive synched access (read/write)
	    ///////////////////////////////////////////////////
	    //gettimeofday(&tvStart, NULL);
	    flush_group_message();
	    //int count;

	    //ioctl(comm_socket, FIONREAD, &count);
//                      printf("1!) Channel has %d bytes \n", count);


	    struct timespec temp_start, temp_end;
	    clock_gettime(CLOCK_MONOTONIC, &temp_start);
	    //for tile
	    
		if (unreal && address < 0x80000000)
			my_packet.type = my_packet.type | PKT_UNREAL;

	    comm_send(&my_packet);
	    // if (!rw)    //shkang
		
	    comm_recv(&my_packet);
	    clock_gettime(CLOCK_MONOTONIC, &temp_end);
	    blockingTime +=
		BILLION * (temp_end.tv_sec - temp_start.tv_sec) +
		temp_end.tv_nsec - temp_start.tv_nsec;


	    comm_count++;
	    comm_message++;
	    //update_interrupt(top, my_packet.interrupt);
	    //gettimeofday(&tvEnd, NULL);

	    diff =
		((tvEnd.tv_sec - tvStart.tv_sec) * 1000000 + tvEnd.tv_usec - tvStart.tv_usec);
	    //              if(diff<2000) 
	    {
		comm_num_sync++;
		comm_sum_wait_write += diff;
		//printf("%d\r\n", ((tvEnd.tv_sec-tvStart.tv_sec)*1000000 + tvEnd.tv_usec - tvStart.tv_usec));
	    }
	    if (!rw) {
		switch (size) {
		case 3:	// BYTE
		case 4:	// SHORT
		case 5:	// WORD
		    *data = my_packet.data;
		    break;
			
		case 6:	// 2-WORD
		    memcpy(data, my_packet.largeData, 8);
		    break;
		case 7:	// 4-WORD
		    memcpy(data, my_packet.largeData, 16);
		    break;
		case 8:	// 8-WORD
		    memcpy(data, my_packet.largeData, 32);
		    break;
		case 9:	// 16-WORD
		    memcpy(data, my_packet.largeData, 64);
		    break;
			
		}
	    }
	    // blocked communication
	    esim_synced_cycles = esim_current_cycles;
	    return my_packet.param.latency;
	}
    
}
*/
Cycle hsim_access(Cycle curCycle, Address address, int rw, uint64_t * data,  int size, int unreal)
{

	esim_current_cycles += (curCycle - esim_cpu_cycle);
	esim_cpu_cycle = curCycle;

	int memory_type = mem_is_shared(address);

	if (memory_type == MEM_TYPE_SHARED) {
		mem_access(esim_current_cycles - esim_synced_cycles, address, rw, data, size, unreal);

	} else if (memory_type == MEM_TYPE_INTERNAL) {
	//	printf("hsim_access %x\n", address - SYSTEM_REGISTER_BASE);			
		switch (address - SYSTEM_REGISTER_BASE) {
			case SYSTEM_SET_INTERRUPT_PERIOD:
				if (rw) {
					esim_interrupt_period = *data;
				}
				break;
			case SYSTEM_SET_NOTICE_PERIOD:
				comm_notice_period = *data;
				break;
			case SYSTEM_SET_INTERRUPT_SYNC_PERIOD:
				comm_interrupt_sync_period = *data;
				break;
			default:
				system_access(esim_current_cycles - esim_synced_cycles,	  address, rw, data, size);
				break;
		}
	} else if (memory_type == MEM_TYPE_PERF){
//		uint64_t *list = (uint64_t*)perfBuffer;
//		*data = list[(address - PERF_REGISTER_BASE) / 8];
		
	}
	return 0;
}


void system_access(Cycle curCycle, Address address, int rw, uint64_t * data, int size)
{
	packet my_packet;
	my_packet.needFeedback = 1;
	my_packet.size = 1 << (size - 3);
	//printf("system_access %d\n", (unsigned int)address); 
	switch (address - SYSTEM_REGISTER_BASE) {
		case SYSTEM_SLEEP:			// SLEEP
			my_packet.type = PKT_SLEEP | PKT_UNREAL;
			my_packet.cycle = esim_current_cycles - esim_synced_cycles;
			esim_synced_cycles = esim_current_cycles;

			printf("SLEEP...\r\n");
			comm_send(&my_packet);
			while (1) {
				comm_recv(&my_packet);
				if (my_packet.type == PKT_WAKEUP)
					break;
			}
			esim_synced_cycles = esim_current_cycles;
			printf("...WAKEUP\r\n");
			break;
		case SYSTEM_GETTIME:			// get current time
			if (!rw)
				*data = esim_current_cycles;
			break;
		case SYSTEM_GETID:
			if (!rw)
				*data = core_id;
			break;
		case SYSTEM_VCACHE_SYNC:
//			vcache_sync();
			break;
		case SYSTEM_CACHE_FLUSH:
			my_packet.type = PKT_CACHE_FLUSH | PKT_UNREAL;
			my_packet.cycle = esim_current_cycles - esim_synced_cycles;
			esim_synced_cycles = esim_current_cycles;
			comm_send(&my_packet);
			esim_synced_cycles = esim_current_cycles;
			break;
		case SYSTEM_CACHE_WRITEBACK:
			my_packet.type = PKT_CACHE_WRITEBACK | PKT_UNREAL;
			my_packet.cycle = esim_current_cycles - esim_synced_cycles;
			esim_synced_cycles = esim_current_cycles;
			comm_send(&my_packet);
			esim_synced_cycles = esim_current_cycles;
			break;
		case SYSTEM_CACHE_INVALIDATE:
			my_packet.type = PKT_CACHE_INVALIDATE | PKT_UNREAL;
			my_packet.cycle = esim_current_cycles - esim_synced_cycles;
			esim_synced_cycles = esim_current_cycles;
			comm_send(&my_packet);
			esim_synced_cycles = esim_current_cycles;
			break;
		case SYSTEM_CACHE_TOGGLE:
			my_packet.type = PKT_CACHE_TOGGLE | PKT_UNREAL;
			my_packet.cycle = esim_current_cycles - esim_synced_cycles;
			esim_synced_cycles = esim_current_cycles;
			printf("CACHE TOGGLE request...\r\n");
			comm_send(&my_packet);
			esim_synced_cycles = esim_current_cycles;
			break;
			/*
		case SYSTEM_PERF_REFLESH:
			my_packet.type = PKT_PERF_REFLESH | PKT_UNREAL;
			my_packet.cycle = esim_current_cycles - esim_synced_cycles;
			esim_synced_cycles = esim_current_cycles;
			printf("PERF reflesh! \r\n");
			comm_send(&my_packet);
			//wait
			printf("Sending the request packet...\n");
			comm_recv(&my_packet);
			//TODO: Temporary...
//			memcpy(data, perfBuffer, sizeof(PerfCounter));
//			comm_recv_datf((void*)perfBuffer, sizeof(perfBuffer));
//			printf("-- %" PRIu64 " cycles \n", perfBuffer->cycle[0]);

			break;
			*/
	}
}

void hsim_notice(Cycle curCycle)
{
    esim_current_cycles += (curCycle - esim_cpu_cycle);
    esim_cpu_cycle = curCycle;
//      printf("%d %d\n", (int)esim_current_cycles, (int)esim_cpu_cycle);
    if (esim_current_cycles - esim_synced_cycles > comm_notice_period) {
	comm_sync(esim_current_cycles - esim_synced_cycles);
	esim_synced_cycles = esim_current_cycles;
    }
}


void flush_group_message(void){

}

