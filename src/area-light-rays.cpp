#include "area-light-rays.h"

using namespace std;

namespace rta {

	rectangular_light_ray_generator::rectangular_light_ray_generator(unsigned int samples, const vec3_t &center, float w, float h)
	: ray_generator(samples, 1), samples(samples), w(w), h(h), center(center), rd(), gen(rd()), distribution(-1,1) {
	}

	void rectangular_light_ray_generator::generate_rays() {
		for (int i = 0; i < samples; ++i) {
			float x = uniform_random_11() * w * 0.5;
			float z = uniform_random_11() * w * 0.5;
			origin(i,0)->x = center.x + x;
			origin(i,0)->y = center.y;
			origin(i,0)->z = center.z + z;
			x = uniform_random_11();
			z = uniform_random_11();
			vec3_t dir = pbrt::CosineSampleHemisphere(x, z);
			*direction(i,0) = { dir.x, -dir.z, dir.y };
		}
	}

	std::string rectangular_light_ray_generator::identification() {
		return "rectangular light ray generator (x/z plane)";
	}

}

/* vim: set foldmethod=marker: */

