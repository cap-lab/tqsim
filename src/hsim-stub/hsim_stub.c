#include <sched.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>
#include "cachesim.h"
#include "hsim_comm.h"
#include "hsim_stub.h"
#include "hsim_mmap.h"
#include "hsim_syscall.h"
#include "cache.h"

static int core_id;
static uint64_t tqsim_cpu_cycle = 0;
static uint64_t tqsim_last_sync_cycles = 0;
static int comm_notice_period = 500;
uint32_t *hsim_register;

extern uint64_t num_insts;

char comm_id[64];

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

void hsim_start(void)
{
	printf("hsim_start\n");
	tqsim_cpu_cycle = 0;

	comm_initialize(shmem_comm_mode);
	memmap_initialize();
	hsim_syscall_init();
	comm_read_env();
}

void hsim_end(uint64_t cycle)
{
	printf("hsim_end\n");
	printf("simulated_insts: %" PRIu64 " insts\n",  num_insts);
	tqsim_cpu_cycle = cycle;
	comm_destroy(tqsim_cpu_cycle - tqsim_last_sync_cycles);
	memmap_destroy();
	tqsim_last_sync_cycles = tqsim_cpu_cycle;
}

//TODO: replace the logic with more advanced logic along with libraries
void comm_read_env(void)
{
	char fileName[64];
	FILE *fp;
	sprintf(fileName, ".env_%s.dat", comm_id);
	printf("Comm file name = %s\n", fileName);
	printf("Comm read env...\n");
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

			if (strcmp(key, "NoticePeriod") == 0) {
				printf("-- set notice period : %s\n", value);
				comm_notice_period = atoi(value);
			} else if (strcmp(key, "Memorymap") == 0) {
				int index = 0;
				char *lpszData[3];
				unsigned int arData[3];

				lpszData[index++] = value;
				for (i = 0; i < 128; i++) {
					if (value[i] == ':') {
						value[i] = '\0';
						lpszData[index++] = &value[i + 1];
					} else if (value[i] == '\0')
						break;
				}
				for (i = 0; i < 3; i++)
					arData[i] = atoi(lpszData[i]);
				printf
					("-- add memory region (0x%08x:0x%08x,  type:%d)\n",
					 arData[0], arData[0] + arData[1] - 1,  arData[2]);

				mem_region *pRegion =(mem_region *) malloc(sizeof(mem_region));

				pRegion->base = arData[0];
				pRegion->size = arData[1];
				pRegion->type = (MemoryType) arData[2];
				memmap_add_region(pRegion);

			} else if (strcmp(key, "ProcessorId") == 0) {
				core_id = atoi(value);
				printf("-- set core iD : %d\n", core_id);
			}

			else
				printf("Unknown configuration : %s\n", key);
		}
		fclose(fp);
	}
	// mapping to core
	{
		int alloc_proc = 0;
		int num_total_proc =  sysconf(_SC_NPROCESSORS_ONLN);
		alloc_proc = (core_id+1)% num_total_proc;
		set_affinity(alloc_proc);
	}
}
uint64_t mem_access(uint64_t cycle, Address address, int rw, uint64_t * data, int size, AccessType atype)
{
//	if (address < 0x30000000)
//		return;

	int size_in_byte =  size / 8;
	if (size_in_byte > 8) size_in_byte = 8;		//FIXME. instruction cache miss
	Packet my_packet;
	my_packet.cycle = cycle;

	my_packet.address = address;
	my_packet.size = size_in_byte;
	my_packet.type = rw?packet_write:packet_read;
	my_packet.flags = (uint32_t)atype;

	if (rw){
		memcpy(my_packet.data, data, size_in_byte);
	}
	comm_send(&my_packet);
	if (my_packet.flags & BlockingAccess){
		if (!rw){
			comm_recv(&my_packet);
			memcpy(data, my_packet.data, size_in_byte);
		}
	}
	return 0;
}
extern unsigned long guest_base;

uint64_t hsim_internal_access(uint64_t cycle, Address address, int rw, uint64_t * data,  int size, AccessType atype)
{
	Packet my_packet;
	int size_in_byte =  size / 8;
	my_packet.cycle = cycle;
	my_packet.size = size_in_byte;
	my_packet.flags= BlockingAccess;

	if (rw){
		memcpy(	&(hsim_register[(address - SYS_REG_BASE)/4]), data,  size_in_byte);
	}

	if ((address - SYS_REG_BASE)/4 == NUM_SYS_REG-1){
		switch (hsim_register[NUM_SYS_REG-1]) {
			case syscall_set_notice_period:
				comm_notice_period = hsim_register[0];
				break;
			case syscall_sleep:
				my_packet.type = packet_syscall;
				printf("SLEEP...\r\n");
				comm_send(&my_packet);
				while (1) {
					comm_recv(&my_packet);
					if (hsim_register[NUM_SYS_REG-1] == syscall_wakeup)
						break;
				}
				printf("...WAKEUP\r\n");
				break;
			case syscall_get_il1_miss:
				{
					uint64_t *dst = (uint64_t*)hsim_register;
					*dst = cachesim_il1_num_miss();
				}
				break;
			case syscall_get_il1_access:
				{
					uint64_t *dst = (uint64_t*)hsim_register;
					*dst = cachesim_il1_num_access();
				}
				break;
			case syscall_get_num_insts:
				{
					uint64_t *dst = (uint64_t*)hsim_register;
					*dst = num_insts; 
				}
				break;

			case syscall_host_to_guest_addr: 
			case syscall_guest_to_host_addr:
			case syscall_get_ul2_access:
			case syscall_get_dl1_access:
			case syscall_get_dl1_miss:
			case syscall_get_ul2_miss:
			case syscall_get_ul2_access_tile:
			case syscall_get_ul2_miss_tile:
			case syscall_get_cycle:
			case syscall_lm_read:
			case syscall_lm_write:
			case syscall_cache_flush_all:
			case syscall_cache_writeback_all:
			case syscall_cache_invalidate_all:
			case syscall_cache_flush:
			case syscall_cache_writeback:
			case syscall_cache_invalidate:
			case syscall_connect:
			case syscall_disconnect:
			case syscall_load_template:
			case syscall_test_and_set:
			case syscall_barrier_init:
			case syscall_barrier:
			case syscall_is_connected:
			case syscall_debug:
			case syscall_hw_lock:
			case syscall_hw_unlock:
				my_packet.type = packet_syscall;
				my_packet.flags = BlockingAccess;
				comm_send(&my_packet);
				comm_recv(&my_packet);
				break;
	
			
			case syscall_get_id:
				hsim_register[0] = core_id;
				break;
			case syscall_fetch_mem:
				{					
					my_packet.type = packet_syscall;
					uint32_t dst_address = hsim_register[0];
					int fetch_size = hsim_register[2];
					comm_send(&my_packet);
					comm_recv(&my_packet);
					uint8_t *buf;
					buf = (uint8_t*)malloc(fetch_size);
					comm_recv_data((uint8_t*)buf, fetch_size);
					memcpy((void*)(dst_address+guest_base), buf, fetch_size);
					free(buf);
				}
				break;



			default:
				break;
		}
	}
	if (!rw){
		memcpy(data, &(hsim_register[(address - SYS_REG_BASE)/4]),  my_packet.size);
	}
	return 0;
}

uint64_t hsim_access(uint64_t cur_cycle, Address address, int rw, uint64_t * data,  int size, AccessType atype)
{
	tqsim_cpu_cycle = cur_cycle;
	uint64_t delta_cycle; 
	if (tqsim_cpu_cycle > tqsim_last_sync_cycles){
		 delta_cycle = tqsim_cpu_cycle - tqsim_last_sync_cycles;
	}
	else {
		delta_cycle = 1;
	}

	MemoryType memory_type = memmap_get_type(address);
	if (memory_type == MEM_SHARED_C || memory_type == MEM_SHARED_NC || memory_type == MEM_VIRTUAL || memory_type == MEM_SCRATCHPAD) {
		mem_access(delta_cycle, address, rw, data, size, atype);
	}
	else if (memory_type == MEM_INTERNAL) {
		hsim_internal_access(delta_cycle, address, rw, data, size, atype);
	}
	else {
		printf("unknown mem type %d\n", memory_type);
	}
	tqsim_last_sync_cycles = tqsim_cpu_cycle;
	return 0;
}

void hsim_notice(uint64_t cur_cycle)
{
	tqsim_cpu_cycle = cur_cycle;
	uint64_t delta_cycle = tqsim_cpu_cycle - tqsim_last_sync_cycles;
	if (tqsim_cpu_cycle > tqsim_last_sync_cycles && delta_cycle > comm_notice_period) {
		comm_sync(delta_cycle);
		tqsim_last_sync_cycles = tqsim_cpu_cycle;
	}
	
}


void hsim_syscall_init(void){
	hsim_register = (uint32_t*)get_sysreg();
}


