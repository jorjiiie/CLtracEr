#include <stdio.h>


// halton sampling in each pixel
typedef struct point {
	double x,y;
} point;

void halton(int n, int d) {
	// represent n in base b but inverted
	double halt_num_1d = 0;
	double base_exp_1d = 0.5;
	int n2 = n;
	while (n) {
		halt_num_1d += base_exp_1d * (n % 2);
		base_exp_1d/=2.0;
		n>>=1;
	}

	double halt_num_2d = 0;
	double base_exp_2d = 1.0/3;
	while(n2) {
		halt_num_2d += base_exp_2d * (n2%3);
		base_exp_2d/=3;
		n2/=3;
	}
	printf("(%lf, %lf)\n",halt_num_1d, halt_num_2d);
}

int main() {
	for (int i=1;i<100;i++)
		halton(i,2);
}