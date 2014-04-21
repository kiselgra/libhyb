#include "shadowrays.h"

using namespace std;

namespace rta {
	pointlight_shadow_ray_generator::pointlight_shadow_ray_generator(int w, int h, const vec3_t &lp) 
	: ray_generator(w, h), light_pos(lp) {
	}
}

/* vim: set foldmethod=marker: */

