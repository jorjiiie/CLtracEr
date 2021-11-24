#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifdef __APPLE__
	#include <OpenCL/OpenCL.h>
#else
	#include <CL/cl.h>
#endif

struct Sphere {
	cl_float4 position;
	cl_float4 emission;
	cl_float4 albedo;

	cl_float radius;
	cl_int TYPE;
	cl_float dummy2;
	cl_float dummy3;
};



struct Ray {

}

int main() {
	const char* attributeNames[5] = { "Name", "Vendor",
        "Version", "Profile", "Extensions" };
    const cl_platform_info attributeTypes[5] = { CL_PLATFORM_NAME, CL_PLATFORM_VENDOR,
        CL_PLATFORM_VERSION, CL_PLATFORM_PROFILE, CL_PLATFORM_EXTENSIONS };
    const int attributeCount = sizeof(attributeNames) / sizeof(char*);
	
	const char* deviceAttributeNames[6] = { "Name: ", "Device Type", "Device Version:",
		"Driver Version:", "Device OpenCL Version", "Max Compute Units" };
	const cl_device_info deviceAttributeType[6] = { CL_DEVICE_NAME, CL_DEVICE_TYPE, CL_DEVICE_VERSION,
		CL_DRIVER_VERSION, CL_DEVICE_OPENCL_C_VERSION, CL_DEVICE_MAX_COMPUTE_UNITS};
	const int deviceAttributeCount = sizeof(deviceAttributeNames) / sizeof(char*);

	cl_uint num_platforms;
	cl_platform_id* platforms;
	clGetPlatformIDs(0, NULL, &num_platforms);
	platforms = (cl_platform_id*) malloc(sizeof(cl_platform_id) * num_platforms);
	clGetPlatformIDs(num_platforms, platforms, NULL);

	cl_device_id* devices;
	cl_uint device_count;

	clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 0, NULL, &device_count);
	devices = (cl_device_id*) malloc(sizeof(cl_device_id) * device_count);
	clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, device_count, devices, NULL);


	printf("Found %d devices:\n", device_count);

	char* value;
	size_t valueSize;
	cl_uint maxComputeUnits;
	for (int i=0;i<device_count;i++) {
		// print device name
        clGetDeviceInfo(devices[i], CL_DEVICE_NAME, 0, NULL, &valueSize);
        value = (char*) malloc(valueSize);
        clGetDeviceInfo(devices[i], CL_DEVICE_NAME, valueSize, value, NULL);
        printf("%d. Device: %s\n", i+1, value);
        free(value);

        // print hardware device version
        clGetDeviceInfo(devices[i], CL_DEVICE_VERSION, 0, NULL, &valueSize);
        value = (char*) malloc(valueSize);
        clGetDeviceInfo(devices[i], CL_DEVICE_VERSION, valueSize, value, NULL);
        printf(" %d.%d Hardware version: %s\n", i+1, 1, value);
        free(value);

        // print software driver version
        clGetDeviceInfo(devices[i], CL_DRIVER_VERSION, 0, NULL, &valueSize);
        value = (char*) malloc(valueSize);
        clGetDeviceInfo(devices[i], CL_DRIVER_VERSION, valueSize, value, NULL);
        printf(" %d.%d Software version: %s\n", i+1, 2, value);
        free(value);

        // print c version supported by compiler for device
        clGetDeviceInfo(devices[i], CL_DEVICE_OPENCL_C_VERSION, 0, NULL, &valueSize);
        value = (char*) malloc(valueSize);
        clGetDeviceInfo(devices[i], CL_DEVICE_OPENCL_C_VERSION, valueSize, value, NULL);
        printf(" %d.%d OpenCL C version: %s\n", i+1, 3, value);
        free(value);

        // print parallel compute units
        clGetDeviceInfo(devices[i], CL_DEVICE_MAX_COMPUTE_UNITS,
                sizeof(maxComputeUnits), &maxComputeUnits, NULL);
        printf(" %d.%d Parallel compute units: %d\n", i+1, 4, maxComputeUnits);
	}


}