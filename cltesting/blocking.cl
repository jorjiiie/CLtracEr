__kernel void block(
	__global int* input,
	const int block_id, const int block_sz)  
{
	int i = get_global_id(0);
	//printf("hi %f %f %f\n", joe.x, joe.y, joe.z);
	//printf("%d = %d\n",i+block_id * block_sz,block_id);
	int k;
	for (int i=0;i<6942000;i++) {
		k = i + i;
	}
	int j = k;
	input[i + block_id * block_sz] = k;
	input[i + block_id * block_sz] = block_id;


}