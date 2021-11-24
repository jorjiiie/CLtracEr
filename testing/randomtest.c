#include <stdio.h>
#include <stdint.h>
#include <limits.h>
typedef uint64_t ulong;
typedef uint32_t uint;
typedef struct { uint64_t state;  uint64_t inc; } pcg32_random_t;

uint32_t pcg32_random_r(pcg32_random_t* rng)
{
    uint64_t oldstate = rng->state;
    // Advance internal state
    rng->state = oldstate * 6364136223846793005ULL + (rng->inc|1);
    // Calculate output function (XSH RR), uses old state for max ILP
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

float aa(ulong *seed0, ulong *seed1) {
    // pcg32
    ulong oldstate = *seed0;
    *seed0 = oldstate * 6364136223846793005ULL + (*seed1|1);

    uint xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint rot = oldstate >> 59u;
    return ((xorshifted >> rot) | (xorshifted << ((-rot) & 31))) * 1.0 / (UINT_MAX); 
}
int main() {
	ulong seed0 = 69420;
	ulong seed1 = 210;

	pcg32_random_t joe = {17289312, 127381};
	int good = 0;
	for (int i=0;i<200000000;i++) {
		double x = aa(&seed0, &seed1);
		double y = aa(&seed0, &seed1);
		if (x*x + y*y <= 1) {
			good++;
		}
	}
	printf("%lf\n",good*4*1.0/200000000);
}