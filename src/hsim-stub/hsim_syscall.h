#ifndef __HSIM_SYSCALL_H
#define __HSIM_SYSCALL_H

#ifdef __cplusplus
extern "C"
{
#endif

#define SYS_REG_BASE 0xE0000000
#define NUM_SYS_REG  8

typedef enum {
	FREQ_12M500K = 0,
	FREQ_25M = 1,
	FREQ_50M = 2,
	FREQ_100M = 3,
	FREQ_400M = 4
} FreqLevel;


typedef enum {	
	syscall_nop,																					/* No operation */
	syscall_begin, syscall_exit, syscall_set_notice_period, syscall_get_id, syscall_sleep, syscall_wakeup, syscall_vcache_sync, 	/* HSIM_fundament */
	syscall_cache_flush, syscall_cache_writeback, syscall_cache_invalidate, 							/* Cache Operations */
	syscall_cache_flush_all, syscall_cache_writeback_all, syscall_cache_invalidate_all,
	syscall_get_cycle, syscall_get_num_insts,
	syscall_get_il1_access, syscall_get_dl1_access, syscall_get_ul2_access,
	syscall_get_il1_miss, syscall_get_dl1_miss, syscall_get_ul2_miss, 			/* Performance Counter Related */
	syscall_get_ul2_access_tile, syscall_get_ul2_miss_tile,
	syscall_host_to_guest_addr, syscall_guest_to_host_addr,
	syscall_lm_read, syscall_lm_write,																/* Local memory access */
	syscall_fetch_mem,
	syscall_connect, syscall_disconnect, syscall_is_connected, syscall_load_template,
	syscall_test_and_set,
	syscall_set_dvfs_level, syscall_get_dvfs_level,
	syscall_barrier_init, syscall_barrier
} hsim_syscall;

#ifdef __cplusplus
}
#endif

#endif
