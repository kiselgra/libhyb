#ifndef __TRAV_UTIL_H__ 
#define __TRAV_UTIL_H__ 

#include "pbrt.h"

#include <librta/cuda-vec.h>

namespace rta {
	/*! \brief transforms the vector into some tangent space given by the normal.
	 *  \note taken from librctest (kai)
	 */
	template<typename vec3> heterogenous inline vec3 make_tangential(const vec3 &dir, const vec3 &normal) {
		vec3 tangent = { -y_comp(normal), x_comp(normal), 0 };
		if (x_comp(tangent) == 0 && y_comp(tangent) == 0)
			tangent.x = 1, tangent.y = tangent.z = 0;
		vec3 bitan;
		cross_vec3f(&bitan, &normal, &tangent);
		vec3 new_dir;
		x_comp(new_dir) = x_comp(tangent) * x_comp(dir)  +  x_comp(bitan) * y_comp(dir)  +  x_comp(normal) * z_comp(dir);
		y_comp(new_dir) = y_comp(tangent) * x_comp(dir)  +  y_comp(bitan) * y_comp(dir)  +  y_comp(normal) * z_comp(dir);
		z_comp(new_dir) = z_comp(tangent) * x_comp(dir)  +  z_comp(bitan) * y_comp(dir)  +  z_comp(normal) * z_comp(dir);
		return new_dir;
	}

	//! \note taken from librctest (kai)
	template<typename vec3> heterogenous inline void diffuse_bounce(vec3 &dir, const vec3 &n, float u1, float u2) {
		vec3 normal = n;
		vec3 neg_dir = {-dir.x, -dir.y, -dir.z};
		if (dot_vec3f(&normal, &neg_dir) < 0) {
			normal.x = -normal.x;
			normal.y = -normal.y;
			normal.z = -normal.z;
		}
		dir = pbrt::CosineSampleHemisphere(u1, u2); // z points "up" from the surface
		make_tangential(dir, normal);
	}


	template<typename tri_t> heterogenous inline typename tri_t::vec3_t intersection_position(const rta::triangle_intersection<tri_t> &ti, tri_t *triangles) {
		typename tri_t::vec3_t bc;
		ti.barycentric_coord(&bc);
		tri_t ref = triangles[ti.ref];
		const typename tri_t::vec3_t &va = vertex_a(ref);
		const typename tri_t::vec3_t &vb = vertex_b(ref);
		const typename tri_t::vec3_t &vc = vertex_c(ref);
		typename tri_t::vec3_t p;
		rta::barycentric_interpolation(&p, &bc, &va, &vb, &vc);
		return p;
	}

	template<typename tri_t> heterogenous inline typename tri_t::vec3_t intersection_normal(const rta::triangle_intersection<tri_t> &ti, tri_t *triangles) {
		typename tri_t::vec3_t bc;
		ti.barycentric_coord(&bc);
		tri_t ref = triangles[ti.ref];
		const typename tri_t::vec3_t &va = normal_a(ref);
		const typename tri_t::vec3_t &vb = normal_b(ref);
		const typename tri_t::vec3_t &vc = normal_c(ref);
		typename tri_t::vec3_t p;
		rta::barycentric_interpolation(&p, &bc, &va, &vb, &vc);
		return p;
	}

	heterogenous inline float clamp(float x, float min, float max) {
		if (x > max) return max;
		if (x < min) return min;
		return x;
	}

	//! http://en.wikipedia.org/wiki/Smoothstep
	heterogenous inline float smoothstep(float edge0, float edge1, float x) {
		x = clamp((x - edge0)/(edge1 - edge0), 0.0f, 1.0f); 
		return x*x*(3 - 2*x);
	}

	//! http://en.wikipedia.org/wiki/Smoothstep
	heterogenous inline float smootherstep(float edge0, float edge1, float x) {
		x = clamp((x - edge0)/(edge1 - edge0), 0.0f, 1.0f);
		return x*x*x*(x*(x*6 - 15) + 10);
	}
}



#endif

