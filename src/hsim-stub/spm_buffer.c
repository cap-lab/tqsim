#include "spm_buffer.h"
#include <string.h>

void sp_init (SPMBuffer * sb)
{
  sb->size = 0;
}

void sp_free (SPMBuffer * sb)
{
}

int sp_is_full (SPMBuffer * sb)
{
	return (sb->size > 0);
}

int sp_is_empty (SPMBuffer * sb)
{
	return (sb->size == 0);
}

void sp_write (SPMBuffer * sb, uint8_t *data, uint32_t size)
{
  memcpy(sb->payload, data, size);
  sb->size = size;
}

void sp_read (SPMBuffer * sb, uint8_t *data, uint32_t size)
{
  memcpy(data, sb->payload, size);
  sb->size = 0;
}


