#ifndef flatmem_stub_h
# define flatmem_stub_h

# include "esim_type.h"
# include <string.h>
# include <stdint.h>
# include <stdio.h>
# include <unistd.h>

# define PKT_READ			0x0
# define PKT_WRITE			0x1
# define PKT_ELAPSED		0x2
# define PKT_EXIT			0x3
# define PKT_INTERRUPT		0x4
# define PKT_BEGIN			0x5
# define PKT_SLEEP			0x6
# define PKT_WAKEUP			0x7
# define PKT_CACHE_INVALIDATE    0x8
# define PKT_CACHE_WRITEBACK   0x9
# define PKT_CACHE_FLUSH    0xA
# define PKT_CACHE_TOGGLE   0xB
# define PKT_PERF_REFLESH   0xC

# define PKT_GROUP_ELAPSED	0xD
# define PKT_GROUP_SYNC		0xE
# define PKT_GROUP_READ		0xF

# define PKT_UNREAL			0x10
# define PKT_CHANNEL_CMD	0x20

#define MEM_TYPE_SHARED		0x3
#define MEM_TYPE_INTERNAL	0x2
#define MEM_TYPE_PERF	0x4

#define NONE_COMM_MODE		0x0
#define PIPE_COMM_MODE		0x1
#define SOCKET_COMM_MODE	0x2
#define SHMEM_COMM_MODE		0x3

//packet 
#define SYSTEM_SLEEP 0x0
#define SYSTEM_GETTIME 0x4
#define SYSTEM_GETID 0x8
#define SYSTEM_CACHE_INVALIDATE 0x10
#define SYSTEM_CACHE_WRITEBACK 0x14
#define SYSTEM_CACHE_FLUSH 0x18
#define SYSTEM_CACHE_TOGGLE 0x1C
#define SYSTEM_PERF_REFLESH 0x20
#define SYSTEM_VCACHE_SYNC 0x24
#define SYSTEM_SET_INTERRUPT_PERIOD 0x28
#define SYSTEM_SET_NOTICE_PERIOD 0x2C
#define SYSTEM_SET_INTERRUPT_SYNC_PERIOD 0x30



extern Cycle cpu_cycle;
extern Cycle last_sync_cycle;
extern uint32_t cpu_id;
extern int comm_notice_period;
extern int tile_id;
extern int core_id;

//extern unsigned long guest_base;
//extern int have_guest_base;


typedef struct mem_region {
    Address base;
    unsigned long size;
    Cycle latency;
    unsigned char type;
    struct mem_region *next;
    struct mem_region *prev;
} mem_region;

void HsimStart(void);
void HsimEnd(Cycle cycle);
void esim_initialize(void);
void esim_destroy(Cycle cycle);

void set_affinity(int alloc_id);


int hsim_socket_open(int nPort);
int hsim_socket_wait_connection(int fd);
int hsim_socket_listen_and_accept(unsigned int port);
int hsim_socket_connect(const char *host, unsigned int port);
void comm_send(packet * pPacket);
void comm_send_data(const char *pSrc, int nSize);
void flush_group_message(void);
void comm_sync(Cycle cycle);
void comm_close(Cycle cycle);
int comm_recv(packet * pPacket);
int comm_recv_data(uint8_t * pDst, int nSize);
int comm_check_interrupt(Cycle cycle);

void mem_initialize(void);
void mem_destroy(void);
void mem_add_region(mem_region * pRegion);
int mem_is_shared(Address addr);
int mem_is_local(Address addr);
mem_region *mem_find_mem_region(Address addr);

void comm_read_env(void);
void comm_initialize(void);
void shmem_initialize(void);
void shmem_close(void);


void socket_initialize(void);
void pipe_initialize(void);
void comm_destroy(Cycle cycle);


Cycle mem_access(Cycle cycle, Address address, int rw, uint64_t * data,
                 int size, int unreal);
//Cycle mem_access_old(Cycle cycle, Address address, int rw, uint64_t * data,    int size, int unreal);

Cycle hsim_access(Cycle curCycle, Address address, int rw, uint64_t * data,
                  int size, int unreal);
void system_access(Cycle curCycle, Address address, int rw, uint64_t * data,
                   int size);
void hsim_notice(Cycle curCycle);

#endif
