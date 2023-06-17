#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <GLFW/glfw3.h>

#define CL_TARGET_OPENCL_VERSION 120
#define __CL_ENABLE_EXCEPTIONS

#ifdef __APPLE__
	#include <OpenCL/OpenCL.h>
    #define TIMETEST
#else
	#include <CL/cl.h>
#endif

#ifdef __linux__
    #define TIMETEST
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
#define MAX_SOURCE 100000 * sizeof(char)

const int IMG_WIDTH = 600;
const int IMG_HEIGHT = 400;

// speeds for camera movement per second
const double v_default = 15;
const double v_theta = 0.2;
const double v_phi = 0.2;
const double mouse_sens = 0.002;
// velocities!
// theta, phi, forwards, side, up
int v_t, v_p, v_f, v_s, v_u;

// current angles ()
double up_phi = PI/2, up_theta = 0;
double phi = 0, theta = 0;
double last_xpos, last_ypos;

#ifdef TIMETEST
    #include <sys/time.h>

    #define frame_count 20
    long long ft[frame_count];

    int cur = 0;
    long long total_time = 0;
#endif

//change f2f
int delta = 1;

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
    // padded to 48 bytes
    // cl_float3 d1;
} Shader;
// i should add this to kernel code but i can't be bothered
typedef struct {
    struct vec3 pos, target, up, d_width, d_height, direction, bottom_left, side;
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
                         .r = 1},

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
        (Shader) {.alb = (cl_float3) {1.0f, 1.0f, 1.0f},
                  .emit = (cl_float3) {0.0f, 0.0f, 0.0f},
                  .type = 2,
                  .ior = 1.5},
        (Shader) {.alb = (cl_float3) {0.7f, 0.7f, 0.7f},
                  .emit = (cl_float3) {0.0f, 0.0f, 0.0f},
                  .type = 1}
    };
    for (int i=0;i<9;i++) {
        spheres[i]=spheres1[i];
        shaders[i]=shaders1[i];
    }
}
void init_cam() {
    // init cam
    // get everything yadayada

    // even makes it even (duh)
    if (cam.width&1) cam.width++;
    if (cam.height&1) cam.height++;


    cam.direction.x = cos(phi) * cos(theta);
    cam.direction.y = cos(phi) * sin(theta);
    cam.direction.z = sin(phi);

    cam.up.x = cos(up_phi) * cos(up_theta);
    cam.up.y = cos(up_phi) * sin(up_theta);
    cam.up.z = sin(up_phi);

    vec3_normalize(&cam.up, &cam.up);

    vec3_add(&cam.direction,&cam.pos,&cam.target);

    double dist = sqrt(vec3_dot(&cam.direction, &cam.direction));

    vec3_cross(&cam.direction, &cam.up, &cam.d_width);
    vec3_normalize(&cam.d_width, &cam.d_width);
    vec3_set(&cam.side, &cam.d_width);

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

void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  switch(key) {
    // velocities
    // switch to an if statement to handle the up/down stuffs
    case GLFW_KEY_W:
        v_f = ((action == GLFW_PRESS || action == GLFW_REPEAT) ? 1 : 0);
        break;
    case GLFW_KEY_A:
        v_s = ((action == GLFW_PRESS || action == GLFW_REPEAT) ? 1 : 0);
        break;
    case GLFW_KEY_S:
        v_f = ((action == GLFW_PRESS || action == GLFW_REPEAT) ? -1 : 0);
        break;
    case GLFW_KEY_D:
        v_s = ((action == GLFW_PRESS || action == GLFW_REPEAT) ? -1 : 0);
        break;
    case GLFW_KEY_SPACE:
        v_u = ((action == GLFW_PRESS || action == GLFW_REPEAT) ? 1 : 0);
        break;
    case GLFW_KEY_LEFT_SHIFT:
        v_u = (action == GLFW_PRESS ? -1 : 0);
        break;
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;


    default:
        break;
  }
}
void glfwCursorCallback(GLFWwindow* window, double xpos, double ypos) {
    theta += (last_xpos - xpos) * -mouse_sens;
    up_theta += (last_xpos - xpos) * -mouse_sens;
    


    // check if above hemisphere plus small delta
    double dy = last_ypos - ypos;
    if (dy < 0 && phi > -(PI/2 - 0.04)) {
        phi += (last_ypos - ypos) * mouse_sens;
        up_phi += (last_ypos - ypos) * mouse_sens;
    }
    else if (dy > 0 && (phi < PI/2 - 0.04)) {
        phi += (last_ypos - ypos) * mouse_sens;
        up_phi += (last_ypos - ypos) * mouse_sens;
    }
    delta = 1;

    last_xpos = xpos;
    last_ypos = ypos;
}

int main(int argc, char** argv) {

    srand(69);
    


    GLFWwindow* window;
    if (!glfwInit()) {
        printf("bad\n");
        return 1;
    }
    window = glfwCreateWindow(IMG_WIDTH,IMG_HEIGHT,"Super cool ray tracer",NULL,NULL);
    if (!window) {
        printf("no window\n");
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
    cl_mem output_img = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                            IMG_WIDTH * IMG_HEIGHT * sizeof(cl_float3), NULL, &ret);
    printf("%d\n",ret);

    cl_mem img_buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                            IMG_WIDTH * IMG_HEIGHT * sizeof(unsigned char) * 3, NULL, &ret);
    printf("%d\n",ret);

    ret = clEnqueueWriteBuffer(command_queue, sphere_device, CL_TRUE, 0,
                            num_spheres * sizeof(Sphere), &spheres, 0, NULL, NULL);
    printf("%d\n",ret);
    ret = clEnqueueWriteBuffer(command_queue, shader_device, CL_TRUE, 0,
                            num_spheres * sizeof(Shader), &shaders, 0, NULL, NULL);
    printf("%d\n",ret);

    cl_kernel kernel = clCreateKernel(program, "progressive_refine", &ret);
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

    ret = clSetKernelArg(kernel, 11, sizeof(cl_mem), &output_img);
    printf("%d\n",ret);

    ret = clSetKernelArg(kernel, 12, sizeof(cl_mem), &img_buffer);
    printf("%d\n", ret);

    cl_event event;
    unsigned char* final_img = (unsigned char*) malloc(IMG_HEIGHT*IMG_WIDTH*sizeof(unsigned char) * 3);

    
    glfwMakeContextCurrent(window);
    int width, height;
    if (argc <= 1)
        glfwSetWindowSize(window,IMG_WIDTH/2,IMG_HEIGHT/2);
    else
        glfwSetWindowSize(window, IMG_WIDTH, IMG_HEIGHT);
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    printf("%d %d %d %d\n",width,height, IMG_WIDTH, IMG_HEIGHT);


    time_t last_time = clock();
    time_t start = clock();
    cam.width = IMG_WIDTH;
    cam.height = IMG_HEIGHT;
    cam.fov = 90;
    cam.direction = (struct vec3) {1.0f, 0.0f, 0.0f};
    cam.pos = (struct vec3) {0.0f, 0.0f, 0.0f};
    // no target, target is just direction + pos
    cam.up = (struct vec3) {0.0f, 0.0f, 1.0f};

    // cursor + keyboard callbacks
    glfwSetKeyCallback(window, glfwKeyCallback);
    glfwSetCursorPosCallback(window, glfwCursorCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwGetCursorPos(window, &last_xpos, &last_ypos);

    init_cam();
    last_time = clock();
    int current_sample = 0;
    while (!glfwWindowShouldClose(window)) {

        // this measures frametime not elapsed time between events
        #ifdef TIMETEST
            struct timeval frame_begin_tv;
            gettimeofday(&frame_begin_tv, NULL);
        #endif


        glfwPollEvents();

        if (delta) {
            init_cam();
            current_sample = 0;
        }
        delta = 0;


        ret = clSetKernelArg(kernel, 5, sizeof(float), &cam.fov);

        ret = clSetKernelArg(kernel, 6, sizeof(cl_float3), &pos);

        ret = clSetKernelArg(kernel, 7, sizeof(cl_float3), &target);

        ret = clSetKernelArg(kernel, 8, sizeof(cl_float3), &d_up);

        ret = clSetKernelArg(kernel, 9, sizeof(cl_float3), &d_width);

        ret = clSetKernelArg(kernel, 10, sizeof(cl_float3), &bottom_left);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ret = clSetKernelArg(kernel, 13, sizeof(int), &current_sample);

        int seed1 = rand();
        int seed2 = rand();


        ret = clSetKernelArg(kernel, 14, sizeof(int), &seed1);
        ret = clSetKernelArg(kernel, 15, sizeof(int), &seed2);

        size_t global_work_size = IMG_WIDTH * IMG_HEIGHT;
        size_t local_work_size = 16;

        ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, 
                                &global_work_size, &local_work_size, 0, 
                                NULL, &event);

        clWaitForEvents(1,&event);

        ret = clEnqueueReadBuffer(command_queue, img_buffer, CL_TRUE, 0, IMG_WIDTH * IMG_HEIGHT * sizeof(unsigned char) * 3,
                                final_img, 0, NULL, NULL);

        



        // just a heads up this is kinda bad as it doesn't factor in gpu time and only cpu time
        // so if the scene has varying complexity its gonna be bad (inconsistent)
        double seconds =  (double) (clock() - last_time) / CLOCKS_PER_SEC;

        // v2d addition * the target/side/up vector
        struct vec3 tmp;

        // this should be independent of z, so just take out the z and norm it
        if (v_f) {
            cam.direction.z = 0;
            vec3_normalize(&cam.direction, &cam.direction);
            vec3_multiply(&cam.direction,v_f*seconds*v_default,&tmp);
            tmp.z = 0;
            vec3_add(&cam.pos,&tmp,&cam.pos);
            delta = 1;
        }
        if (v_s) {
            cam.side.z = 0;
            vec3_normalize(&cam.side, &cam.side);
            vec3_multiply(&cam.side,v_s*seconds*v_default,&tmp);
            tmp.z = 0;
            vec3_add(&cam.pos,&tmp,&cam.pos);
            delta = 1;
        }
        if (v_u) {
            cam.pos.z += (v_default * v_u * seconds);
            delta = 1;
        }
        last_time = clock();

        current_sample++;

        #ifdef TIMETEST
            struct timeval frame_end_tv;
            gettimeofday(&frame_end_tv, NULL);

            long usec_elapsed = (frame_end_tv.tv_sec - frame_begin_tv.tv_sec) * 1000000 + frame_end_tv.tv_usec - frame_begin_tv.tv_usec;

            total_time -= ft[cur];

            ft[cur++] = usec_elapsed;
            total_time += usec_elapsed;
            printf("Average frame time for past %d frames is %lf ms or %d fps %lld %ld %d %lf ms (cpu time)       \r", frame_count, total_time/1000.0/frame_count, (int)(1000000*frame_count/total_time), total_time, usec_elapsed, current_sample, seconds*1000);
            fflush(stdout);
            cur %= frame_count;
        #endif
        glDrawPixels(IMG_WIDTH,IMG_HEIGHT,GL_RGB,GL_UNSIGNED_BYTE,final_img);
        glfwSwapBuffers(window);
    }


    ret = clEnqueueReadBuffer(command_queue, img_buffer, CL_TRUE, 0, IMG_WIDTH * IMG_HEIGHT * sizeof(unsigned char) * 3,
                                final_img, 0, NULL, NULL);
    printf("%d\n",ret);

    printf("hi\n");
    FILE *out = fopen("img.ppm", "w");
    fprintf(out, "P3 %d %d 255\n",cam.width, cam.height);
    for (int height_id = 0; height_id < cam.height; height_id++) 
        for (int width_id = 0; width_id < cam.width; width_id++) {
            fprintf(out, "%d %d %d\n",  final_img[height_id * IMG_WIDTH * 3 + width_id * 3 + 0],
                                        final_img[height_id * IMG_WIDTH * 3 + width_id * 3 + 1],
                                        final_img[height_id * IMG_WIDTH * 3 + width_id * 3 + 2]);
                        
        }

}
