__kernel void square(
	const __global int* input,
	__global int* output) 
{
	int i = get_global_id(0);
	for (int i=0;i<50000000;i++) 
		output[i] = input[i] * input[i];
}