#include "sat_counter.h"

void init_counter(SatCounter *c, uint8_t initial_val, unsigned bits){
	c->initialVal = initial_val;
	c->maxVal = (1<<bits) -1;
	c->counter = initial_val;
}


void reset_counter(SatCounter *c)
{
	c->counter = c->initialVal;
	
}
void increment_counter(SatCounter *c){
	        if (c->counter < c->maxVal) {
            ++(c->counter);
        }

}

void decrement_counter(SatCounter *c){
        if (c->counter > 0) {
            --(c->counter);
        }
}



uint8_t read_counter(SatCounter *c) {
	return c->counter;
}



