#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

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
#define IMG_WIDTH 16
#define IMG_HEIGHT 2

/*
	todo:
		cleanup code and make things more consistent (make things that need to be pointers pointers, otherwise just copy it)
		switch on material
			image textures
		switch on type of intersection
		bvh (aabbs)
		triangles
		photon mapping maybe
		try to make it fast enough for real time ray tracing?!?!!?

*/

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
void vec3_mult2(struct vec3* a, struct vec3* b, struct vec3* c) {
	c->x = a->x * b->x;
	c->y = a->y * b->y;
	c->z = a->z * b->z;
}
void vec3_copy(struct vec3* a, struct vec3* b) {
	b->x = a->x;
	b->y = a->y;
	b->z = a->z;
}
void vec3_negate(struct vec3* a, struct vec3* b) {
	b->x = -a->x;
	b->y = -a->y;
	b->z = -a->z;
}
void vec3_random_sphere(struct vec3* a) {
	a->x = RANDRANGE(-1,1);
	a->y = RANDRANGE(-1,1);
	a->z = RANDRANGE(-1,1);
	do {
		a->x = RANDRANGE(-1,1);
		a->y = RANDRANGE(-1,1);
		a->z = RANDRANGE(-1,1);
	} while (vec3_dot(a,a) > 1);
}
void vec3_random_sphere2(double u1, double u2, struct vec3* out) {
	double r = sqrt(1-u1*u1);
	double theta = 2 * PI * u2;
	out->x = r * cos(theta);
	out->y = r * sin(theta);
	out->z = sqrt(MAX(0.0, u1));
}

// halton 2/3
// technically messes up the thing but its fine because i had [-0.5,0.5] but this is [0,1)
void halton(int n, double* w_fac, double* h_fac) {
	// represent n in base b but inverted
	double halt_num_1d = 0;
	double base_exp_1d = 0.5;
	int n2 = n;
	while (n) {
		halt_num_1d += base_exp_1d * (n % 2);
		base_exp_1d/=2.0;
		n>>=1;
	}

	double halt_num_2d = 0;
	double base_exp_2d = 1.0/3;
	while(n2) {
		halt_num_2d += base_exp_2d * (n2%3);
		base_exp_2d/=3;
		n2/=3;
	}
	*w_fac = halt_num_1d;
	*h_fac = halt_num_2d;
	
}

struct Lambertian {
	double emit, alb;
};
struct Glossy {
	double roughness;
};
struct Dielectric {
	double ior;
};
struct Shader {
	// shader with lambertian and whatever
	// 0 = lambertian
	// 1 = glossy
	// 2 = refractive
	int type;
	void* shader_data;
};
struct Sphere {
	struct vec3 pos, emit, alb, vp;
	int type;
	double r;
	double ior;
};
struct Triangle {
	struct vec3 a,b,c;
};
struct Shape {
	void* shape_ptr;
	int type;
	struct Shader* material;
};

void sphere_normal(struct Sphere* hit_sphere, struct vec3* point, struct vec3* normal) {
	// normal = point - center probably
	vec3_sub(point, &hit_sphere->pos, normal);
}

#define PVEC(v) printf("%f %f %f\n", v.x, v.y, v.z);

int sphere_intersect(struct Sphere* sphere, struct vec3* position, 
						struct vec3* direction, double* time_out) {

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

	//printf("%lf %lf %lf %lf\n", t1,t2, front, back);
	double mx = MAX(t1,t2);

	if (mx <= NEAR_CLIP) 
		return 0;

	double mn = MIN(t1,t2);

	if (mn <= NEAR_CLIP) 
		(*time_out) = mx;
	else 
		(*time_out) = mn;

	return 69;
}

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
	vec3_multiply(&cam->d_height, height_id, &h_offset);
	vec3_multiply(&cam->d_width, width_id, &w_offset);

	ADD_TWO(out, &w_offset, &h_offset);


	vec3_sub(out, &cam->pos, out);



}
long total_bounces = 0;
int pix = 0;
void trace(struct Sphere* spheres, size_t sphere_count, struct vec3 position,
				struct vec3 direction, struct vec3* final_color, struct vec3 background_color) {

	struct vec3 attenuation, col;
	attenuation = (struct vec3) {1,1,1};
	col = (struct vec3) {0,0,0};

	int bounces;
	for (bounces = 0; bounces < 2; bounces++) {

		double t = FAR_CLIP;
		struct Sphere* hit = NULL;
		int hit_id = 0;
		for (int i = 0; i < sphere_count; i++) {
			double tt;
			if (sphere_intersect(&spheres[i], &position, &direction, &tt)) {
				if (tt < t && tt > NEAR_CLIP) {
					t = tt;
					hit = &spheres[i];
					hit_id = i;
				}
			}
		}
		

		if (hit==NULL) {
			// background * attenuation
			vec3_mult2(&attenuation, &background_color, &attenuation);
			vec3_add(&col, &attenuation, &col);
			break;
		}

		if (bounces == 0)
			pix++;


		struct vec3 hit_point;
		vec3_multiply(&direction,t,&hit_point);
		vec3_add(&hit_point,&position,&hit_point);

		struct vec3 tmp_light;

		// add the light to the multiplied attenuation
		vec3_mult2(&hit->emit,&attenuation,&tmp_light);
		vec3_add(&col, &tmp_light, &col);

		// multiply by albedo for the picking up of colors
		vec3_mult2(&hit->alb, &attenuation, &attenuation);


		struct vec3 normal, new_direction;
		sphere_normal(hit, &hit_point, &normal);

		vec3_normalize(&normal, &normal);

		double cost;
		if ((cost = vec3_dot(&normal, &direction)) > 0) {
			vec3_multiply(&normal, -1, &normal);
			cost = -cost;
		}

		if (hit->type == 0) {
			// for lambertian brdf/pdf = 1/pi apparently i dont know why
			// attenuation = attenuation * cos(normal, new_direction)

			vec3_multiply(&attenuation,1/PI,&attenuation);

			vec3_random_sphere2(RAND(), RAND(), &new_direction);
			// vec3_random_sphere(&new_direction);
			vec3_normalize(&new_direction,&new_direction);
	
			vec3_add(&new_direction, &normal, &new_direction);


		} else if (hit->type == 1){
			// reflection
			struct vec3 backwards;
			vec3_multiply(&normal, vec3_dot(&normal, &direction) * 2, &backwards);
			vec3_sub(&direction, &backwards, &new_direction);
			
		} else {
			// transmissive
			double cos_t = -vec3_dot(&normal, &direction);
			double sin_t = sqrt(1.0 - cos_t * cos_t);



			double ior = 1.0/(hit->ior);
			int inside = 0;
			if (cos_t < 0.0) {
				ior = 1.0/ior;
				cos_t = -cos_t;
				inside = 1;
			}

			// schlick from ray tracing in a weekend
			double r0 = (1-ior) / (1+ior);
			r0 = r0*r0;
			double r1 = r0 + (1+r0)*pow((1-cos_t),5);

			if (ior * sin_t <= 1.0 || r1 > RAND()) {
				// refract

				struct vec3 r_perp;
				vec3_multiply(&normal,-cos_t,&r_perp);
				vec3_sub(&r_perp, &normal, &r_perp);

				vec3_multiply(&r_perp,ior,&r_perp);

				struct vec3 r_par;

				double inside = 1.0 - vec3_dot(&r_perp, &r_perp);
				vec3_multiply(&normal, -sqrt(ABS(inside)), &r_par);

				vec3_add(&r_par,&r_perp,&new_direction);

			} else {
				// reflect
				struct vec3 backwards;
				if (inside) {
					vec3_multiply(&normal, vec3_dot(&normal, &direction) * 2, &backwards);
				} else {
					vec3_multiply(&normal, -vec3_dot(&normal, &direction) * 2, &backwards);

				}
				vec3_sub(&direction, &backwards, &new_direction);
			}
		}

		if (bounces > 4) {
			double p = MAX(attenuation.x, MAX(attenuation.y, attenuation.z));
			if (RAND() > p) 
				break;

			vec3_multiply(&attenuation, 1/p, &attenuation);
		}

		vec3_set(&position, &hit_point);
		vec3_set(&direction, &new_direction);
		vec3_normalize(&direction,&direction);
		printf("%d sphere %d %lf direc %lf %lf %lf\n",bounces, hit_id, t,normal.x, normal.y, normal.z);

	}
	total_bounces += bounces;
	vec3_set(final_color,&col);
}
struct vec3* IMAGE;
void gamma_correct(struct vec3* px, double degree) {
	//hi my name is shash, <3 shash
	vec3_pow(px, degree, px);
}

#define IMG_PIX(i,j) img[(i) * cam->width + (j)]

void render(struct cam* cam, int samples, struct Sphere* spheres, 
				size_t sphere_count, struct vec3** image) {

	// develop the camera stuff
	
	cam_init(cam);

	struct vec3* img;
	img = (struct vec3*) malloc(sizeof(struct vec3) * cam->width * cam->height);
	*image = img;

	struct vec3 background = (struct vec3) {0.1, 0.1, 0.1};
	for (int height_id = 0; height_id < cam->height; height_id++) {
		for (int width_id = 0; width_id < cam->width; width_id++) {
			for (int i = 0; i < samples; i++) {
				struct vec3 vec, w_rand, h_rand;

				/*
				// random jitter				
				vec3_multiply(&cam->d_width, RAND(), &w_rand);
				vec3_multiply(&cam->d_height, RAND(), &h_rand);
				*/
				///*
				// halton jitter
				double w_fac, h_fac;
				halton(i,&w_fac,&h_fac);
				vec3_multiply(&cam->d_width, w_fac, &w_rand);
				vec3_multiply(&cam->d_height, h_fac, &h_rand);	
				//*/


				cam_getRay(cam, width_id, height_id, &vec);

				// ADD_TWO(&vec, &w_rand, &h_rand);
				vec3_normalize(&vec,&vec);

				//printf("%d %d: %lf %lf %lf\n", width_id, height_id, vec.x, vec.y, vec.z);
				struct vec3 col = (struct vec3) {0,0,0};
				trace(spheres, sphere_count, cam->pos, vec, &col, background);
				vec3_add(&IMG_PIX(height_id, width_id), &col, &IMG_PIX(height_id, width_id));
			}
			// adjust samples
			vec3_multiply(&IMG_PIX(height_id, width_id), 1.0/samples, &IMG_PIX(height_id, width_id));
			gamma_correct(&IMG_PIX(height_id, width_id), 0.5);
		}

	}
}

void viewport_render(struct cam* cam, int samples, struct Sphere* spheres,
					 	size_t sphere_count, struct vec3** image) {

	cam_init(cam);

	struct vec3* img;
	img = (struct vec3*) malloc(sizeof(struct vec3) * cam->width * cam->height);
	*image = img;

	struct vec3 background = (struct vec3) {0.1, 0.1, 0.1};
	for (int height_id = 0; height_id < cam->height; height_id++) {
		for (int width_id = 0; width_id < cam->width; width_id++) {
			struct vec3 vec, w_rand, h_rand;
			cam_getRay(cam, width_id, height_id, &vec);

			vec3_normalize(&vec,&vec);
			struct vec3 hitpoint;



			struct vec3 col = (struct vec3) {0,0,0};
			// hitting stuffo
			double t = FAR_CLIP;
			struct Sphere* hit = NULL;
			for (int i = 0; i < sphere_count; i++) {
				double tt;
				if (sphere_intersect(&spheres[i], &cam->pos, &vec, &tt)) {
					if (tt < t && tt > NEAR_CLIP) {
						t = tt;
						hit = &spheres[i];
					}
				}
			}

			if (hit==NULL) {
			} else {
				vec3_add(&IMG_PIX(height_id, width_id), &hit->vp, &IMG_PIX(height_id, width_id));
			}
		}
	}
}

#define PRINT_COLOR(f,v) fprintf(f,"%d %d %d\n", \
							(int) (CLAMP(v.x,0,1) * 255),	\
							(int) (CLAMP(v.y,0,1) * 255),	\
							(int) (CLAMP(v.z,0,1) * 255)	\
						)
int main() {

	srand(0);
	struct vec3 pos, target, up;

	pos = (struct vec3) {0, 0, 0};
	target = (struct vec3) {1, 0, 0};
	vec3_normalize(&target, &target);
	up = (struct vec3) {0, 0, 1};
	struct cam cam;
	cam.pos = pos;
	cam.target = target;
	cam.up = up;
	cam.width = IMG_WIDTH;
	cam.height = IMG_HEIGHT;
	cam.fov = 90;

	struct vec3 s_pos;
	s_pos = (struct vec3) {10,0,0};

	/*
	struct Sphere spheres[5];

	spheres[0].pos = s_pos;
	spheres[0].r = 3;
	spheres[0].alb = (struct vec3) {.6,.6,.6};
	spheres[0].emit = pos;
	spheres[0].type = 1;
	spheres[0].vp = (struct vec3) {.5, .5, .5};

	spheres[1].pos = (struct vec3) {10,0,5};
	spheres[1].r = 2;
	spheres[1].alb = (struct vec3) {.5, .5, .5};
	spheres[1].emit = pos;
	spheres[1].type = 0;
	spheres[1].vp = (struct vec3) {.1, .5 ,.5};

	spheres[2].pos = (struct vec3) {5,0,10};
	spheres[2].r = 2;
	spheres[2].emit = (struct vec3) {10,10,10};
	spheres[2].alb = (struct vec3) {1,1,1};
	spheres[2].type = 1;
	spheres[2].vp = (struct vec3) {1,1,1};

	spheres[3] = (struct Sphere) {
		.pos = (struct vec3) {10,5,0},
		.r = 2,
		.emit = (struct vec3) {0,0,0},
		.alb = (struct vec3) {.6,.6,.6},
		.type = 0,
		.vp = (struct vec3) {0,0,0}
	};

	spheres[4] = (struct Sphere) {
		.pos = (struct vec3) {10,5,5},
		.r = 3,
		.emit = (struct vec3) {0,0,0},
		.alb = (struct vec3) {1,1,1},
		.type = 1,
		.vp = (struct vec3) {0,0,0}
	};

//*/

	// /*

	struct vec3 zero = (struct vec3) {0,0,0};
	struct vec3 grey = (struct vec3) {0.7,0.7,0.7};
	struct Sphere spheres[9] = {
		// right wall
		(struct Sphere) {.pos = (struct vec3) {0,1005,0},
						 .r = 1000,
						 .emit = zero,
						 .alb = {0.1,0.1,0.7},
						 .type = 0,
						 .vp = zero},
		// left wall
		(struct Sphere) {.pos = (struct vec3) {0,-1005,0},
						 .r = 1000,
						 .emit = zero,
						 .alb = (struct vec3) {0.7,.1,.1},
						 .type = 0,
						 .vp = zero},
		// top wall
		(struct Sphere) {.pos = (struct vec3) {0,0,1005},
						 .r = 1000,
						 .emit = zero,
						 .alb = grey,
						 .type = 0,
						 .vp = zero},
		//bottom wall
		(struct Sphere) {.pos = (struct vec3) {0,0,-1005},
						 .r = 1000,
						 .emit = zero,
						 .alb = grey,
						 .type = 0,
						 .vp = zero},
		// back wall
		(struct Sphere) {.pos = (struct vec3) {1010,0,0},
						 .r = 1000,
						 .emit = zero,
						 .alb = grey,
						 .type = 0,
						 .vp = grey},
		// light
		(struct Sphere) {.pos = (struct vec3) {5,0,4},
						 .r = 1,
						 .emit = (struct vec3) {35,35,35},
						 .alb = grey,
						 .type = 1,
						 .vp = zero},
		(struct Sphere) {.pos = (struct vec3) {5,-3,-1.5},
						 .r = 2,
						 .emit = zero,
						 .alb = grey,
						 .type = 0,
						 .vp = zero},
		(struct Sphere) {.pos = (struct vec3) {7,3,-1},
						 .r = 1.5,
						 .emit = zero,
						 .alb = (struct vec3) {.2, .8, .2},
						 .type = 2,
						 .ior = 1.5,
						 .vp = zero},
		(struct Sphere) {.pos = (struct vec3) {7,-1.5,2},
						 .r = 1.3,
						 .emit = zero,
						 .alb = (struct vec3) {1,1,1},
						 .type = 1,
						 .vp = zero}
	};
//*/
	render(&cam, 1, spheres, 9, &IMAGE);

	exit(0);
    PVEC(cam.pos);
    PVEC(cam.target);
    PVEC(cam.direction);
    PVEC(cam.up);
    PVEC(cam.d_height);
    PVEC(cam.d_width);
    PVEC(cam.bottom_left);
    printf("height: %d width %d\n",cam.height, cam.width);
    exit(0);
	FILE *out = fopen("HALTONJITTER.ppm", "w");
	fprintf(out, "P3 %d %d 255\n",cam.width, cam.height);
	for (int height_id = 0; height_id < cam.height; height_id++) 
		for (int width_id = 0; width_id < cam.width; width_id++) {
			PRINT_COLOR(out, IMAGE[height_id * cam.width + width_id]);
		}
	printf("Total Bounces %ld %d\n", total_bounces, pix);

}