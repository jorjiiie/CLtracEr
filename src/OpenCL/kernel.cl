// OpenCL kernel

__constant float EPSILON = 0.00003f; 
__constant float PI = 3.14159265359f;
__constant int SAMPLES = 1<<10;
__constant float NEAR_CLIP = 0.00003;
__constant float FAR_CLIP = 10000000;
__constant float IPI = 0.31830988618f;
#define CLAMP(a,mn,mx) (((a) > (mx)) ? (mx) : ((a) < (mn) ? (mn) : (a)))
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define ABS(a) ((a) < 0 ? (-a) : (a))
// for triangles and stuff maybe throw it in through a big struct that has the arrays or smth
// memory hog though
// opencl does kinda suck

typedef struct {
    // sphere data
    float3 pos;
    float r;
} Sphere;
typedef struct {
    // 0 = lamb, 1 = glossy, 2 = refractive
    float3 alb, emit;
    int type;
    float ior;
} Shader;

inline void halton(int n, float* w_fac, float* h_fac) {
    float halt_num_1d = 0;
    float base_exp_1d = 0.5;
    int n2 = n;
    while (n) {
        halt_num_1d += base_exp_1d * (n % 2);
        base_exp_1d/=2.0;
        n>>=1;
    }

    float halt_num_2d = 0;
    float base_exp_2d = 1.0/3;
    while(n2) {
        halt_num_2d += base_exp_2d * (n2%3);
        base_exp_2d/=3;
        n2/=3;
    }
    *w_fac = halt_num_1d;
    *h_fac = halt_num_2d;
}
inline float random(ulong *seed0, ulong *seed1) {
    // pcg32
    ulong oldstate = *seed0;
    *seed0 = oldstate * 6364136223846793005ULL + (*seed1|1);

    uint xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint rot = oldstate >> 59u;
    return ((xorshifted >> rot) | (xorshifted << ((-rot) & 31))) * 1.0f / (4294967295u); 
}

float3 sphere_random(float u1, float u2) {
    // this is technically wrong but looks pretty close + converges faster
    float r = sqrt((float) (1-(u1*u1)));
    float theta = 2 * PI * u2;
    return (float3) (r * cos(theta), r * sin(theta), sqrt(MAX(0.0,u1)));
}
int sphere_intersect(__constant Sphere *sphere, float3 pos, float3 direc, float *t) {
    float3 ray = pos - sphere->pos;

    float front = - dot(direc, ray);
    float inside = front * front - (dot(ray,ray) - sphere->r * sphere->r);

    if (inside < 0.0) 
        return 0;
    

    float back = sqrt(inside);
    float t1 = front - back;
    float t2 = front + back;

    // t2 > t1 always
    //printf("%f %f %f %f\n", t2, t1, front, back);
    if (t2 <= NEAR_CLIP) 
        return 0;

    if (t1 <= NEAR_CLIP)
        *t = t2;
    else 
        *t = t1;
    return 69;

}
float sphere_intersect2(Sphere *sphere, float3 pos, float3 direc) {
    float3 ray = pos-(sphere->pos);
    float front = - dot(direc, ray);
    float inside = front * front - (dot(ray,ray) - (sphere->r * sphere->r));
    if (inside < 0) 
        return -1.0;
    
    float back = sqrt(inside);
    float t1 = front - back;
    float t2 = front + back;

    // t2 > t1 always
    if (t2 <= NEAR_CLIP) 
        return -1.0;

    if (t1 <= NEAR_CLIP)
        return t2;
    else 
        return t1;

}
float3 sphere_normal(Sphere sphere, float3 point) {
    return (float3) (point - sphere.pos);
}
float3 get_cam_ray(int width_id, 
                int height_id, 
                float3 bottom_left,
                float3 d_up, 
                float3 d_width) {

    return (float3) (bottom_left + height_id * d_up + width_id * d_width);
}

float3 trace(__constant Sphere* spheres, 
            __constant Shader* shaders, 
            int num_spheres, 
            float3 pos, 
            float3 direction,
            ulong *seed0, 
            ulong *seed1) {

    float3 color = (float3) (0.0f, 0.0f, 0.0f);
    float3 attenuation = (float3) (1.0f, 1.0f, 1.0f);

    // 10 max bounces, after 4 = RR
    for (int bounces=0; bounces<15; bounces++) {
        float min_t = FAR_CLIP;
        Sphere hit;
        int hit_id = -1;
        for (int i=0; i<num_spheres; i++) {
            float t;
            if (sphere_intersect(&spheres[i],pos,direction, &t)) {

                if (t <= min_t && t >= NEAR_CLIP) {
                    min_t = t;
                    hit = spheres[i];
                    hit_id = i;
                }
                //return attenuation;
            }
        }

        // grey background... could have a picture but that's for nerds
        if (hit_id == -1) {
            color += attenuation * (float3) (0.6f, 0.6f, 0.6f);
            break;
        }
       
        // light + attenuation
        color += (attenuation * shaders[hit_id].emit);
        attenuation *= shaders[hit_id].alb;

        float3 hit_point = pos + (direction * min_t);
        float3 normal = normalize(sphere_normal(hit,hit_point));
        float cos_t = dot(direction, normal);

        // shaders switch
        if (shaders[hit_id].type == 0) {
            // diffuse
            if (cos_t > 0) 
                normal = -normal;

            // get the random ray
            direction = sphere_random(random(seed0,seed1), random(seed0,seed1));

            // i still dont know why this is try but oh well
            attenuation *= IPI;
            direction = normalize(direction) + normal;
        } else if (shaders[hit_id].type == 1) {
            // glossy
            if (cos_t > 0)
                normal = -normal;

            direction = (direction - (normal * dot(normal, direction) * 2));

        } else if (shaders[hit_id].type==2){
            // transmissive
            float ior = shaders[hit_id].ior;
            int inside = 0;
            if (cos_t < 0) {
                // the right way
                ior = 1.0/ior;
            } else {
                inside = 1;
                normal = -normal;
                cos_t = -cos_t;
            }
            float sin_t = sqrt(1.0-cos_t*cos_t);

            float r0 = (1-ior) / (1+ior);
            r0 = r0*r0;
            float r1 = r0 + (1+r0)*pow((1-cos_t),5);
           // printf("cos: %f sin: %f ior:%f\n",cos_t,sin_t, ior);
            if (ior*sin_t <= 1.0 || r1 > random(seed0, seed1)) {
                float sin_t1 = ior * sin_t;
                float cos_t1 = sqrt(1-sin_t1*sin_t1);
                //printf("%f\n",sin_t1);
                float3 r_par = cos_t1 * (normal);

                float3 r_perp = sin_t1 * normalize(direction - (-cos_t * normal));
               // printf("inside new direction = %f %f %f\n ", direction.x, direction.y, direction.z);
                direction = r_par + r_perp;
            } else {
                normal = -normal;
                direction = (direction + (normal * dot(normal, direction) * 2));
            }
    

        }
        direction = normalize(direction);
        // this fixes the bad float stuff I think
        pos = hit_point + (normal * 0.01);
        //printf("%d sphere %d %f at %f %f %f \n", bounces, hit_id, min_t, direction.x, direction.y, direction.z);
        if (bounces > 4) {
            float prob = MAX(attenuation.x, MAX(attenuation.y, attenuation.z));
            if (random(seed0,seed1) > prob) {
                break;
            }
            attenuation = attenuation/prob;
        }
    }

    return color;

}
// second progressive refine thingie
// img is gamma corrected averaged, output is just raw data
__kernel void progressive_refine(__constant Sphere* spheres, 
                                __constant Shader* shaders, 
                                const int num_spheres,
                                const int width, 
                                const int height, 
                                const float fov, 
                                const float3 pos, 
                                const float3 target, 
                                const float3 d_up, 
                                const float3 d_width, 
                                const float3 bottom_left, 
                                __global float3* output, 
                                __global unsigned char* img, 
                                int sample) {

    int pix_num = get_global_id(0);
    int x_coord = pix_num % width;
    int y_coord = pix_num / width;

    // random seeding
    ulong seed0 = x_coord + 69 + sample * 4220;
    ulong seed1 = y_coord + 4220 + sample * 69420;
    float3 accum = (float3) (0.0f, 0.0f, 0.0f);

    random(&seed0, &seed1);
    random(&seed0, &seed1);

    float wfac, hfac;
    halton(sample,&wfac,&hfac);
    float3 cam_ray = normalize(get_cam_ray(x_coord, y_coord, bottom_left,
                                d_up, d_width) - pos + d_width * wfac + d_up * hfac);

    if (sample==0) {
        output[pix_num] = (float3) (0.0f, 0.0f, 0.0f);
    }
    output[pix_num] += trace(spheres, shaders, num_spheres, pos, cam_ray, &seed0, &seed1);
    
    float r,g,b;
    r = sqrt(output[pix_num].x/(sample+1));
    g = sqrt(output[pix_num].y/(sample+1));
    b = sqrt(output[pix_num].z/(sample+1));


    // to flip, it's height - (current_height + 1) * 3 + width * 3
    // inside down bc opengl is awk
    int real_pix_num = ((height - 1 - y_coord) * width + x_coord) * 3;
    img[real_pix_num    ] = (unsigned char) (256*CLAMP(r, 0.0f, 1.0f));
    img[real_pix_num + 1] = (unsigned char) (256*CLAMP(g, 0.0f, 1.0f));
    img[real_pix_num + 2] = (unsigned char) (256*CLAMP(b, 0.0f, 1.0f));

    return;
}

