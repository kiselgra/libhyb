#ifndef __LIBHYB_SHADOWRAYS_H__ 
#define __LIBHYB_SHADOWRAYS_H__ 

#include "rta-cgls-connection.h"

namespace rta {

	/*! \brief Ray generator for point lights (that includes spotlights).
	 *
	 *  Rays are cast from the receiving points to the light source.
	 *  Another option (which should be faster -> test) would be to trace from the light source to the receiver points.
	 */
	class pointlight_shadow_ray_generator : public ray_generator {
		vec3_t light_pos;
	public:
		pointlight_shadow_ray_generator(int w, int h, const vec3_t &lp);
	};
}

#endif

