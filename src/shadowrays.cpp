#include "shadowrays.h"

using namespace std;

namespace rta {
	pointlight_shadow_ray_generator::pointlight_shadow_ray_generator(int w, int h) 
	: ray_generator(w, h), light_pos(0,0,0) {
	}
		
	void pointlight_shadow_ray_generator::setup(const image<vec3_t, 1> *hitpoints, const vec3f &lp) {
		visible = hitpoints;
		light_pos = lp;
	}

	void pointlight_shadow_ray_generator::generate_rays() {
		for (int y = 0; y < res_y(); ++y)
			for (int x = 0; x < res_x(); ++x) {
				*origin(x, y) = visible->pixel(x,y);
				*direction(x, y) = light_pos - *origin(x, y);
				max_t(x, y) = length_of_vec3f(direction(x, y));
				normalize_vec3f(direction(x, y));
				*origin(x, y) = *origin(x, y) + *direction(x, y)*0.01;
			}
	}
}

/* vim: set foldmethod=marker: */

