#include <stdio.h>
#include <stdlib.h>
#include <OpenCL/opencl.h>

#define NUM_VALUES (10000000)
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

struct joe_mama {
	cl_float3 abc;
};
int main() {


	// INIT VALUES FOR CODE:
	int i;
	int *a = (int*) malloc(sizeof(int) * NUM_VALUES);

	for (int i=0;i<NUM_VALUES;i++) {
		a[i] = i;
	}


	FILE *kernel_source;
	char *source_str;
	size_t source_size;


	kernel_source = fopen("square_kernel.cl","r");
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

	cl_mem a_device = clCreateBuffer(context, CL_MEM_READ_ONLY, 
							NUM_VALUES * sizeof(int), NULL, &ret);

	cl_mem b_device = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
							NUM_VALUES * sizeof(int), NULL, &ret);

	ret = clEnqueueWriteBuffer(command_queue, a_device, CL_TRUE, 0, 
							NUM_VALUES * sizeof(int), a, 0, NULL, NULL);

	cl_program program = clCreateProgramWithSource(context, 1,
	 (const char**) &source_str, (const size_t *) &source_size, &ret);

	ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);

	if (ret==CL_BUILD_PROGRAM_FAILURE) {
		fprintf(stdout,"FAILED\n");
		exit(0);
	}
	cl_kernel kernel = clCreateKernel(program, "square", &ret);

	ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), &a_device);
	ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), &b_device);

	size_t global_work_size = NUM_VALUES;
	size_t local_work_size = 64;
	ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, 
									&global_work_size, &local_work_size, 0, 
									NULL, NULL);

	int *b = (int*) malloc(sizeof(int) * NUM_VALUES);
	ret = clEnqueueReadBuffer(command_queue, b_device, CL_TRUE, 0, NUM_VALUES * sizeof(int)
								, b, 0, NULL, NULL);

	for (int i=0;i<NUM_VALUES;i++) {
		fprintf(stdout,"%d * %d = %d\n", a[i],i,b[i]);
	}


}