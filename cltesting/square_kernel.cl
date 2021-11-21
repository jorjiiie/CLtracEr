__kernel void square(
	__constant int* input,
	__global int* output) 
{
	int i = get_global_id(0);
	//printf("hi %f %f %f\n", joe.x, joe.y, joe.z);
	//printf("aj\n");n

	output[i] = input[i] * input[i];
}