#ifndef __CPU_PRED_SAT_COUNTER_HH__
#define __CPU_PRED_SAT_COUNTER_HH__

#include <inttypes.h>

typedef struct _SatCounter
{
    uint8_t initialVal;
    uint8_t maxVal;
    uint8_t counter;
} SatCounter;

void init_counter(SatCounter *c, uint8_t initial_val, unsigned bits);
void reset_counter(SatCounter *c);
void increment_counter(SatCounter *c);
void decrement_counter(SatCounter *c);
uint8_t read_counter(SatCounter *c);


#endif // __CPU_PRED_SAT_COUNTER_HH__
