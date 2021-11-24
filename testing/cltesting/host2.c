#include <stdio.h>
#include <stdlib.h>
#define __CL_ENABLE_EXCEPTIONS
#include <OpenCL/opencl.h>

#define NUM_VALUES (1000000)
#define MAX_SOURCE 10000 * sizeof(char)
const int num_blocks = 1;
struct joe_mama {
	cl_float3 abc;
};
int main() {


	// INIT VALUES FOR CODE:
	int i;
	int *a = (int*) malloc(sizeof(int) * NUM_VALUES);

	for (int i=0;i<NUM_VALUES;i++) {
		a[i] = -1;
	}


	FILE *kernel_source;
	char *source_str;
	size_t source_size;


	kernel_source = fopen("blocking.cl","r");
	if (!kernel_source) {
		fprintf(stderr, "Failed to load kernel\n");
		exit(1);
	}

	// istg if the source size is too small im gonna cry bc i'll never find the issue
	source_str = (char*) malloc(MAX_SOURCE);
	source_size = fread(source_str, 1, MAX_SOURCE, kernel_source);
	fclose(kernel_source);
	fprintf(stdout, "%d %s\n",(int)source_size, source_str);

	cl_platform_id platform_id = NULL;
	cl_device_id device_id = NULL;
	cl_uint ret_num_devices;
	cl_uint ret_num_platforms;
	cl_int ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
	ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, &ret_num_devices);

	cl_context context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);

	cl_command_queue command_queue =  clCreateCommandQueue(context, device_id, 0, &ret);

	cl_mem a_device = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
							NUM_VALUES * sizeof(int), NULL, &ret);


	ret = clEnqueueWriteBuffer(command_queue, a_device, CL_TRUE, 0, 
							NUM_VALUES * sizeof(int), a, 0, NULL, NULL);

	cl_program program = clCreateProgramWithSource(context, 1,
	 (const char**) &source_str, (const size_t *) &source_size, &ret);

	ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);

	if (ret==CL_BUILD_PROGRAM_FAILURE) {
		fprintf(stdout,"FAILED\n");
		exit(1);
	}

	cl_kernel kernel = clCreateKernel(program, "block", &ret);
	
	int block_sz = NUM_VALUES/num_blocks;
	int *b = (int*) malloc(sizeof(int) * NUM_VALUES);

	for (int i=0;i<num_blocks;i++) {
		printf("hi %d\n", i);

		ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), &a_device);
		ret = clSetKernelArg(kernel, 1, sizeof(int), &i);
		ret = clSetKernelArg(kernel, 2, sizeof(int), &block_sz);

		size_t global_work_size = block_sz;
		size_t local_work_size = 4;
		ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, 
									&global_work_size, &local_work_size, 0, 
									NULL, NULL);

		ret = clEnqueueReadBuffer(command_queue, a_device, CL_TRUE, i*block_sz*sizeof(int), block_sz * sizeof(int)
									, b + (i*block_sz), 0, NULL, NULL);
		// clEnqueueReadBuffer(cmd_queue, of_buf, CL_TRUE, 0, chunk_size, b + i*block_sz, 0, NULL, NULL);
	}
	// for (int i=0;i<NUM_VALUES;i++) 
		// fprintf(stdout, "%d: %d\n", i, b[i]);

}