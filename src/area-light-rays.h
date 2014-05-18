#ifndef __AREA_LIGHT_RAYS_H__ 
#define __AREA_LIGHT_RAYS_H__ 

#include <librta/librta.h>

#include "trav-util.h"

namespace rta {

	// area light aligned in x/z plane, atm.
	class rectangular_light_ray_generator : public rta::ray_generator {
		unsigned int samples;
		float w, h;
		vec3_t center;

		std::random_device rd;
		std::mt19937 gen;
		std::uniform_real_distribution<> distribution;

		inline float uniform_random_11() {
			return float((double)(rand() % 32767) / 32767.0) * 2 - 1;
			return distribution(gen);
		}


	public:
		rectangular_light_ray_generator(unsigned int samples, const vec3_t &center, float w, float h);
		virtual void generate_rays();
		virtual std::string identification();
		virtual void dont_forget_to_initialize_max_t() {}
	};

}


#endif

