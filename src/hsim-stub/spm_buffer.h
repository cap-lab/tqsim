/* Circular buffer example, keeps one slot open */
#ifndef __SPM_BUFFER_H
#define __SPM_BUFFER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#define SP_BUFFER_SIZE 0x1000

typedef struct
{
  uint32_t size;
  uint8_t payload[SP_BUFFER_SIZE];
} SPMBuffer;

void sp_init (SPMBuffer * sp);
void sp_free (SPMBuffer * sp);
int sp_is_full (SPMBuffer * sp);
int sp_is_empty (SPMBuffer * sp);
void sp_write (SPMBuffer * sp, uint8_t *data, uint32_t size);
void sp_read (SPMBuffer * sp, uint8_t *data, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif 
