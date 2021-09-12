#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#define PI 3.1415926535
#define TAN_DEG(X) tan((X) * 1.0 * PI / 180)

#define NEAR_CLIP 0.001
#define FAR_CLIP 100000
#define RAND() rand() * 1.0 / RAND_MAX
#define RANDRANGE(a,b) (a) + ((b - a) * (rand() * 1.0 / RAND_MAX))
#define CLAMP(a,mn,mx) (((a) > (mx)) ? (mx) : ((a) < (mn) ? (mn) : (a)))
// #define CLAMP(value, low, high) (((value)<(low))?(low):(((value)>(high))?(high):(value)))

struct vec3 {
	double x,y,z;
};
void vec3_set(struct vec3* a, struct vec3* b) {
	a->x = b->x;
	a->y = b->y;
	a->z = b->z;
}
void vec3_normalize(struct vec3* vec, struct vec3* out) {
	double fac = 1.0/sqrt(vec->x*vec->x+vec->y*vec->y+vec->z*vec->z);
	out->x = vec->x * fac;
	out->y = vec->y * fac;
	out->z = vec->z * fac;
}
void vec3_pow(struct vec3* vec, double p, struct vec3* out) {
	out->x = pow(vec->x, p);
	out->y = pow(vec->y, p);
	out->z = pow(vec->z, p);
}
double vec3_dot(struct vec3* a, struct vec3* b) {
	return (a->x * b->x + a->y * b->y + a->z * b->z);
}
void vec3_add(struct vec3* a, struct vec3* b, struct vec3* c) {
	c->x = a->x + b->x;
	c->y = a->y + b->y;
	c->z = a->z + b->z;
}
void vec3_sub(struct vec3* a, struct vec3* b, struct vec3* c) {
	c->x = a->x - b->x;
	c->y = a->y - b->y;
	c->z = a->z - b->z;
}
void vec3_cross(struct vec3* a, struct vec3* b, struct vec3* out) {
	out->x = a->y * b->z - a->z * b->y;
	out->y = a->z * b->x - a->x * b->z;
	out->z = a->x * b->y - a->y * b->x;
}
void vec3_multiply(struct vec3* a, double fac, struct vec3* out) {
	out->x = a->x * fac;
	out->y = a->y * fac;
	out->z = a->z * fac;
}
void vec3_copy(struct vec3* a, struct vec3* b) {
	b->x = a->x;
	b->y = a->y;
	b->z = a->z;
}
struct Sphere {
	struct vec3 pos, emit, alb;
	double r;
};
#define PVEC(v) printf("%f %f %f\n", v.x, v.y, v.z);
int sphere_intersect(struct Sphere* sphere, struct vec3* position, struct vec3* direction, double* time_out) {
	struct vec3 v;
	vec3_set(&v, position);
	vec3_sub(&v, &sphere->pos, &v);

	double front = - vec3_dot(&v, direction);
	double inside = front * front - (vec3_dot(&v, &v) - sphere->r * sphere->r);

	if (inside < 0) 
		return 0;

	double back = sqrt(inside);

	double t1 = front + back;
	double t2 = front - back;

	double mx = (t1 > t2) ? t1 : t2;

	if (mx <= NEAR_CLIP) 
		return 0;

	double mn = (t1 > t2) ? t2 : t1;

	if (mn <= NEAR_CLIP) 
		(*time_out) = mx;
	else 
		(*time_out) = mn;

	return 69;
}
struct hit {
	struct vec3 point;
	int front;
};
struct cam {
	struct vec3 pos, target, up, d_width, d_height, direction, bottom_left;
	int width,height;	
	double fov;
};
#define ADD_TWO(a,b,c) 	vec3_add(a,b,a); \
						vec3_add(a,c,a);

void cam_init(struct cam* cam) {
	// get everything yadayada

	// even makes it even (duh)
	if (cam->width&1) cam->width++;
	if (cam->height&1) cam->height++;

	vec3_sub(&cam->target,&cam->pos,&cam->direction);

	double dist = sqrt(vec3_dot(&cam->direction, &cam->direction));

	vec3_cross(&cam->direction, &cam->up, &cam->d_width);
	vec3_normalize(&cam->d_width, &cam->d_width);

	double scale = TAN_DEG(cam->fov/2.0) * 2.0 * dist / (cam->width-1);

	vec3_multiply(&cam->d_width, -scale, &cam->d_width);
	vec3_multiply(&cam->up, -scale, &cam->d_height);

	struct vec3 w_offset, h_offset;
	vec3_set(&cam->bottom_left, &cam->target);
	vec3_multiply(&cam->d_width, -(cam->width/2), &w_offset);
	vec3_multiply(&cam->d_height, -(cam->height/2-1), &h_offset);

	ADD_TWO(&cam->bottom_left, &w_offset, &h_offset);
}
void cam_getRay(struct cam* cam, int width_id, int height_id, struct vec3* out) {
	struct vec3 tmp, w_offset, h_offset;
	vec3_set(out, &cam->bottom_left);
	vec3_multiply(&cam->d_height, height_id, &w_offset);
	vec3_multiply(&cam->d_width, width_id, &h_offset);

	ADD_TWO(out, &w_offset, &h_offset);

	vec3_sub(out, &cam->pos, out);
	vec3_normalize(out, out);
}

struct vec3* IMAGE;
void gamma_correct(struct vec3* px, double degree) {
	// to the degreeth power
	vec3_pow(px, degree, px);
}

void render(struct cam* cam, int samples, struct Sphere* spheres, int sphere_count, struct vec3** image) {
	// develop the camera stuff?
	
	cam_init(cam);

	struct vec3* img;
	img = (struct vec3*) malloc(sizeof(struct vec3) * cam->width * cam->height);
	*image = img;

	for (int height_id = 0; height_id < cam->height; height_id++) {
		for (int width_id = 0; width_id < cam->width; width_id++) {
			for (int i = 0; i < samples; i++) {
				struct vec3 vec, w_rand, h_rand;
				// random jitter
				vec3_multiply(&cam->d_width, RAND(), &w_rand);
				vec3_multiply(&cam->d_height, RAND(), &h_rand);

				cam_getRay(cam, width_id, height_id, &vec);

				ADD_TWO(&vec, &w_rand, &h_rand);
				vec3_normalize(&vec,&vec);
				double t;
				struct vec3 whyte = (struct vec3) {1,1,1};
				if (sphere_intersect(&spheres[0],&cam->pos,&vec,&t))
					vec3_add(&img[height_id * cam->height + width_id], &whyte, &img[height_id * cam->height + width_id]);
			}
			// adjust samples
			vec3_multiply(&img[height_id * cam->height + width_id], 1.0/samples, &img[height_id * cam->height + width_id]);
			gamma_correct(&img[height_id * cam->height + width_id], 0.5);
		}
	}
}

#define PRINT_COLOR(f,v) 	fprintf(f,"%d %d %d\n", \
							(int) (CLAMP(v.x,0,1) * 255),	\
							(int) (CLAMP(v.y,0,1) * 255),	\
							(int) (CLAMP(v.z,0,1) * 255)	\
						)
int main() {

	
	struct vec3 pos, target, up;

	pos = (struct vec3) {0, 0, 0};
	target = (struct vec3) {1, 0, 0};
	up = (struct vec3) {0, 0, 1};
	struct cam cam;
	cam.pos = pos;
	cam.target = target;
	cam.up = up;
	cam.width = 200;
	cam.height = 200;
	cam.fov = 90;

	struct vec3 s_pos;
	s_pos = (struct vec3) {10,0,0};
	struct Sphere spheres[1];
	spheres[0].pos = s_pos;
	spheres[0].r = 3;
	render(&cam, 1000, spheres, 0, &IMAGE);

	FILE *out = fopen("test.ppm", "w");
	fprintf(out, "P3 %d %d 255\n",cam.width, cam.height);
	for (int height_id = 0; height_id < cam.height; height_id++) 
		for (int width_id = 0; width_id < cam.width; width_id++) 
			PRINT_COLOR(out, IMAGE[height_id * cam.height + width_id]);

}