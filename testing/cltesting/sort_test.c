#include <stdio.h>
#include <stdlib.h>
#include <OpenCL/opencl.h>

#define NUM_VALUES (1<<7)
#define MAX_SOURCE 10000 * sizeof(char)
int validate(int* input, int* output) {
	for (int i=0;i<NUM_VALUES;i++) {
		if (output[i] != input[i] * input[i]) {
			fprintf(stdout, "bad at %d: expected %d but got %d\n", i, output[i], input[i] * input[i]);
			fflush(stdout);
			return 0;
		}
	}	
	return 1;
}

int main() {
	int i;
	char name[128];

	int *a = (int*) malloc(sizeof(int) * NUM_VALUES);

	FILE *kernel_source;
	char *source_str;
	size_t source_size;

	for (int i=0;i<NUM_VALUES;i++) {
		a[i] = NUM_VALUES-i;
	}
	kernel_source = fopen("bubble_sort_kernel.cl","r");
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

	cl_mem array_device = clCreateBuffer(context, CL_MEM_READ_ONLY, 
							NUM_VALUES * sizeof(int), NULL, &ret);

	cl_mem clOffset = clCreateBuffer(context, CL_MEM_READ_ONLY,
							sizeof(int), NULL, &ret);
	cl_mem clFlag = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
							sizeof(int), NULL, &ret);

	ret = clEnqueueWriteBuffer(command_queue, a_device, CL_TRUE, 0, 
							NUM_VALUES * sizeof(int), a, 0, NULL, NULL);

	cl_program program = clCreateProgramWithSource(context, 1,
	 (const char**) &source_str, (const size_t *) &source_size, &ret);

	ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);

	cl_kernel kernel = clCreateKernel(program, "bubble_sort", &ret);
	cl_int offset=0;
	for (;;) {
		
		cl_int flag = 0;
		ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), &a_device);
		ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), &clOffset);
		ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), &clFlag);

		size_t global_work_size = NUM_VALUES;
		size_t local_work_size = 64;
		ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, 
										&global_work_size, &local_work_size, 0, 
									NULL, NULL);
		cl_int flg;
		ret = clEnqueueReadBuffer(command_queue, clFlag, CL_TRUE, 0,
								 sizeof(cl_int), &flg, 0, 
						 		NULL, NULL);
		if (flg) {
			offset^=1; 
		}
	}

}