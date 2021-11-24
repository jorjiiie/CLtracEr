#include <math.h>

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
#define ADD_TWO(a,b,c) 	vec3_add(a,b,a); \
						vec3_add(a,c,a);
#define PVEC(v) printf("%f %f %f\n", v.x, v.y, v.z);
						
// might want to do this w cl_float3
void gamma_correct(struct vec3* px, double degree) {
	//hi my name is shash, <3 shash
	vec3_pow(px, degree, px);
}
