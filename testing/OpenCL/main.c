#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <GLFW/glfw3.h>

#define CL_TARGET_OPENCL_VERSION 120
#define __CL_ENABLE_EXCEPTIONS

#ifdef __APPLE__
	#include <OpenCL/OpenCL.h>
#else
	#include <CL/cl.h>
#endif

#include "vec3.c"

#define PI 3.1415926535
#define TAN_DEG(X) tan((X) * 1.0 * PI / 180)

#define NEAR_CLIP 0.01
#define FAR_CLIP 100000000
#define RAND() (rand() * 1.0 / RAND_MAX)
#define RANDRANGE(a,b) ((a) + ((b - a) * (rand() * 1.0 / RAND_MAX)))
#define CLAMP(a,mn,mx) (((a) > (mx)) ? (mx) : ((a) < (mn) ? (mn) : (a)))
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define ABS(a) ((a) < 0 ? (-a) : (a))
#define MAX_SOURCE 10000 * sizeof(char)
#define NUM_VALUES (1<<0)

const int IMG_WIDTH = 600;
const int IMG_HEIGHT = 400;

const int SAMPLES = 1<<11;
const int NUM_BLOCKS = 1<<2;
const int BLOCK_SZ = SAMPLES/NUM_BLOCKS;

// padded to 32 bytes
typedef struct {
    cl_float3 pos;
    cl_float r;
    cl_float d1;
    cl_float d2;
    cl_float d3;
} Sphere;
// padded to 64 bytes
typedef struct {
    cl_float3 alb, emit;
    cl_int type;
    cl_float ior;
    cl_float d2;
    cl_float d3;
    // apparently adding this corrupts memory so i'm not doing it :sunglasses:
    // cl_float3 d1;
} Shader;
typedef struct {
    struct vec3 pos, target, up, d_width, d_height, direction, bottom_left;
    int width,height;   
    double fov;
} Camera;

const int num_spheres = 9;
Sphere spheres[9];
Shader shaders[9];
cl_float3 pos, direction, d_up, d_width, bottom_left, target; 
Camera cam;

void init_scene() {
    // init spheres and shaders
    Sphere spheres1[9] = {
        // right wall
        (Sphere) {.pos = (cl_float3) {0,1005,0},
                         .r = 1000},
        // left wall
        (Sphere) {.pos = (cl_float3) {0,-1005,0},
                         .r = 1000},
        // top wall
        (Sphere) {.pos = (cl_float3) {0,0,1005},
                         .r = 1000},
        //bottom wall
        (Sphere) {.pos = (cl_float3) {0,0,-1005},
                         .r = 1000},
        // back wall
        (Sphere) {.pos = (cl_float3) {1010,0,0},
                         .r = 1000},
        // light
        (Sphere) {.pos = (cl_float3) {5,0,4},
                         .r = 1},

        (Sphere) {.pos = (cl_float3) {5,-3,-1.5},
                         .r = 2},

        (Sphere) {.pos = (cl_float3) {7,3,-1},
                         .r = 1.5},

        (Sphere) {.pos = (cl_float3) {7,-1.5,2},
                         .r = 1.3}
    };

    Shader shaders1[9] = {
        (Shader) {.alb = (cl_float3) {0.1f, 0.1f, 0.7f},
                  .emit = (cl_float3) {0.0f, 0.0f, 0.0f},
                  .type = 0},
        (Shader) {.alb = (cl_float3) {0.7f, 0.1f, 0.1f},
                  .emit = (cl_float3) {0.0f, 0.0f, 0.0f},
                  .type = 0},
        (Shader) {.alb = (cl_float3) {0.7f, 0.7f, 0.7f},
                  .emit = (cl_float3) {0.0f, 0.0f, 0.0f},
                  .type = 0},
        (Shader) {.alb = (cl_float3) {0.7f, 0.7f, 0.7f},
                  .emit = (cl_float3) {0.0f, 0.0f, 0.0f},
                  .type = 0},
        (Shader) {.alb = (cl_float3) {0.7f, 0.7f, 0.7f},
                  .emit = (cl_float3) {0.0f, 0.0f, 0.0f},
                  .type = 0},
        (Shader) {.alb = (cl_float3) {0.7f, 0.7f, 0.7f},
                  .emit = (cl_float3) {35.0f, 35.0f, 35.0f},
                  .type = 0},
        (Shader) {.alb = (cl_float3) {0.7f, 0.7f, 0.7f},
                  .emit = (cl_float3) {0.0f, 0.0f, 0.0f},
                  .type = 0},
        (Shader) {.alb = (cl_float3) {0.2f, 0.8f, 0.2f},
                  .emit = (cl_float3) {0.0f, 0.0f, 0.0f},
                  .type = 2,
                  .ior = 1.5},
        (Shader) {.alb = (cl_float3) {0.7f, 0.7f, 0.7f},
                  .emit = (cl_float3) {0.0f, 0.0f, 0.0f},
                  .type = 1},
    };
    for (int i=0;i<9;i++) {
        spheres[i]=spheres1[i];
        shaders[i]=shaders1[i];
    }
    // init cam
    // get everything yadayada
    cam.width = IMG_WIDTH;
    cam.height = IMG_HEIGHT;
    cam.fov = 90;
    cam.direction = (struct vec3) {1.0f, 0.0f, 0.0f};
    cam.pos = (struct vec3) {0.0f, 0.0f, 0.0f};
    cam.target = (struct vec3) {1.0f, 0.0f, 0.0f};
    cam.up = (struct vec3) {0.0f, 0.0f, 1.0f};
    // even makes it even (duh)
    if (cam.width&1) cam.width++;
    if (cam.height&1) cam.height++;

    vec3_sub(&cam.target,&cam.pos,&cam.direction);

    double dist = sqrt(vec3_dot(&cam.direction, &cam.direction));

    vec3_cross(&cam.direction, &cam.up, &cam.d_width);
    vec3_normalize(&cam.d_width, &cam.d_width);

    double scale = TAN_DEG(cam.fov/2.0) * 2.0 * dist / (cam.width-1);

    vec3_multiply(&cam.d_width, -scale, &cam.d_width);
    vec3_multiply(&cam.up, -scale, &cam.d_height);

    struct vec3 w_offset, h_offset;
    vec3_set(&cam.bottom_left, &cam.target);

    vec3_multiply(&cam.d_width, -(cam.width/2), &w_offset);
    vec3_multiply(&cam.d_height, -(cam.height/2-1), &h_offset);

    ADD_TWO(&cam.bottom_left, &w_offset, &h_offset);

    pos = (cl_float3) {cam.pos.x, cam.pos.y, cam.pos.z};
    direction = (cl_float3) {cam.direction.x, cam.direction.y, cam.direction.z};
    d_up = (cl_float3) {cam.d_height.x, cam.d_height.y, cam.d_height.z};
    d_width = (cl_float3) {cam.d_width.x, cam.d_width.y, cam.d_width.z};
    bottom_left = (cl_float3) {cam.bottom_left.x, cam.bottom_left.y, cam.bottom_left.z};
    target = (cl_float3) {cam.target.x, cam.target.y, cam.target.z};
}
int main() {

    GLFWwindow* window;
    if (!glfwInit()) {
        printf("bad\n");
        return 1;
    }
    
    init_scene();

    FILE *kernel_source;
    char *source_str;
    size_t source_size;

    // init OpenCL
    kernel_source = fopen("kernel.cl","r");
    if (!kernel_source) {
        fprintf(stderr, "Failed to load kernel\n");
        exit(1);
    }
    source_str = (char*) malloc(MAX_SOURCE);
    source_size = fread(source_str, 1, MAX_SOURCE, kernel_source);
    fclose(kernel_source);
    // fprintf(stdout, "%d %s\n %ld",(int)source_size, source_str, source_size);

    cl_platform_id platform_id = NULL;
    cl_device_id device_id = NULL;
    cl_uint ret_num_devices;
    cl_uint ret_num_platforms;
    cl_int ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
    ret = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, &ret_num_devices);
    printf("%d\n",ret);

    cl_context context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &ret);
    printf("%d\n",ret);

    cl_command_queue command_queue =  clCreateCommandQueue(context, device_id, 0, &ret);
    printf("%d\n",ret);
 


    // build program
    cl_program program = clCreateProgramWithSource(context, 1,
     (const char**) &source_str, (const size_t *) &source_size, &ret);

    ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    printf("%d\n",ret);
    printf("%d\n",ret);
    if (ret == CL_BUILD_PROGRAM_FAILURE) {
        size_t len = 0;

        ret = clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &len);
        char *buffer = (char*) calloc(len, sizeof(char));
        ret = clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, len, buffer, NULL);

        fprintf(stdout, "%s\n", buffer);
        exit(1);
    }
    cl_mem sphere_device = clCreateBuffer(context, CL_MEM_READ_ONLY,
                            num_spheres * sizeof(Sphere), NULL, &ret);
    printf("%d\n",ret);
    cl_mem shader_device = clCreateBuffer(context, CL_MEM_READ_ONLY,
                            num_spheres * sizeof(Shader), NULL, &ret);
    printf("%d\n",ret);
    cl_mem output_img = clCreateBuffer(context, CL_MEM_READ_ONLY,
                            IMG_WIDTH * IMG_HEIGHT * sizeof(cl_float3), NULL, &ret);
    printf("%d\n",ret);
    ret = clEnqueueWriteBuffer(command_queue, sphere_device, CL_TRUE, 0,
                            num_spheres * sizeof(Sphere), &spheres, 0, NULL, NULL);
     printf("%d\n",ret);
    ret = clEnqueueWriteBuffer(command_queue, shader_device, CL_TRUE, 0,
                            num_spheres * sizeof(Shader), &shaders, 0, NULL, NULL);
     printf("%d\n",ret);

    cl_kernel kernel = clCreateKernel(program, "render", &ret);
     printf("%d\n",ret);

    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), &sphere_device);
    printf("%d\n",ret);
    ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), &shader_device);
        printf("%d\n",ret);

    ret = clSetKernelArg(kernel, 2, sizeof(int), &num_spheres);
        printf("%d\n",ret);

    ret = clSetKernelArg(kernel, 3, sizeof(int), &IMG_WIDTH);
        printf("%d\n",ret);

    ret = clSetKernelArg(kernel, 4, sizeof(int), &IMG_HEIGHT);
        printf("%d\n",ret);

    ret = clSetKernelArg(kernel, 5, sizeof(float), &cam.fov);
        printf("%d\n",ret);

    ret = clSetKernelArg(kernel, 6, sizeof(cl_float3), &pos);
        printf("%d\n",ret);

    ret = clSetKernelArg(kernel, 7, sizeof(cl_float3), &target);
        printf("%d\n",ret);

    ret = clSetKernelArg(kernel, 8, sizeof(cl_float3), &d_up);
        printf("%d\n",ret);

    ret = clSetKernelArg(kernel, 9, sizeof(cl_float3), &d_width);
        printf("%d\n",ret);

    ret = clSetKernelArg(kernel, 10, sizeof(cl_float3), &bottom_left);
        printf("%d\n",ret);

    ret = clSetKernelArg(kernel, 11, sizeof(cl_mem), &output_img);
        printf("%d\n",ret);

    ret = clSetKernelArg(kernel, 12, sizeof(int), &SAMPLES);
        printf("%d\n",ret);


    // just do k * 2^n for img width and height LOL 
    // like base of 32 or something 
    // 8 is fine too
    // so 4:3 is 32*4n : 32:3n
    // LMFAO
    size_t global_work_size = IMG_WIDTH * IMG_HEIGHT;
    size_t local_work_size = 16;

    ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, 
                                    &global_work_size, &local_work_size, 0, 
                                    NULL, NULL);

        printf("%d\n",ret);


    cl_float3 *img = (cl_float3*) malloc(IMG_HEIGHT*IMG_WIDTH*sizeof(cl_float3));

    ret = clEnqueueReadBuffer(command_queue, output_img, CL_TRUE, 0, IMG_WIDTH * IMG_HEIGHT * sizeof(cl_float3),
                                img, 0, NULL, NULL);
        printf("%d\n",ret);

    // fprintf(stdout, "hi %f %f %f %d %d\n",img[0].x, img[0].y, img[0].z, IMG_WIDTH, IMG_HEIGHT);

    printf("hi\n");
    FILE *out = fopen("img.ppm", "w");
    fprintf(out, "P3 %d %d 255\n",cam.width, cam.height);
    for (int height_id = 0; height_id < cam.height; height_id++) 
        for (int width_id = 0; width_id < cam.width; width_id++) {
            fprintf(out, "%d %d %d\n", (int) (CLAMP(img[width_id + height_id * cam.width].x,0,1) * 255),   
                            (int) (CLAMP(img[width_id + height_id * cam.width].y,0,1) * 255),   
                            (int) (CLAMP(img[width_id + height_id * cam.width].z,0,1) * 255));
                        
        }
    /*


    cl_mem a_device = clCreateBuffer(context, CL_MEM_READ_ONLY, 
                            NUM_VALUES * sizeof(int), NULL, &ret);

    cl_mem b_device = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
                            NUM_VALUES * sizeof(float), NULL, &ret);


    ret = clEnqueueWriteBuffer(command_queue, a_device, CL_TRUE, 0, 
                            NUM_VALUES * sizeof(int), a, 0, NULL, NULL);

    cl_kernel kernel = clCreateKernel(program, "test", &ret);

    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), &a_device);
    ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), &b_device);
    size_t global_work_size = NUM_VALUES;
    size_t local_work_size = 64;
    ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, 
                                    &global_work_size, &local_work_size, 0, 
                                    NULL, NULL);

    float *b = (float*) malloc(sizeof(float) * NUM_VALUES);
    ret = clEnqueueReadBuffer(command_queue, b_device, CL_TRUE, 0, NUM_VALUES * sizeof(float)
                                , b, 0, NULL, NULL);

    printf("%d %d\n",ret, CL_SUCCESS);
    printf("%lf\n",b[0]);
    */



}