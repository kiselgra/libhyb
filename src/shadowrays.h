#ifndef __LIBHYB_SHADOWRAYS_H__ 
#define __LIBHYB_SHADOWRAYS_H__ 

#include "rta-cgls-connection.h"

#include <librta/cuda.h>

namespace rta {

	template<typename box_t, typename tri_t> class binary_shadow_collector : public cpu_ray_bouncer<box_t, tri_t> {
	protected:
		int w, h;
		image<float, 1> attenuation;
	public:
		binary_shadow_collector(int w, int h) : cpu_ray_bouncer<box_t, tri_t>(w, h), attenuation(w, h), w(w), h(h) {
		}
		virtual void bounce() {
			for (int y = 0; y < h; ++y)
				for (int x = 0; x < w; ++x)
					if (this->last_intersection.pixel(x,y,0).valid())
						attenuation.pixel(x,y,0) = 0;
					else
						attenuation.pixel(x,y,0) = 1;
		}
		virtual bool trace_further_bounces() {
			return false;
		}
		virtual std::string identification() {
			return "binary shadow collector";
		}
		float factor(int x, int y) { return attenuation.pixel(x, y, 0); }
	};
	
	namespace cuda {
		template<typename box_t, typename tri_t> 
		class hackish_binary_shadow_collector : public primary_intersection_downloader<box_t, tri_t, binary_shadow_collector<box_t, tri_t> > {
			int w, h;
		public:
			hackish_binary_shadow_collector(int w, int h) 
			: primary_intersection_downloader<box_t, tri_t, binary_shadow_collector<box_t, tri_t> >(w, h), w(w), h(h) {
			}
// 			virtual void bounce() {
// 				primary_intersection_downloader<box_t, tri_t>::bounce();
// 				binary_shadow_collector<box_t, tri_t>::bounce();
// 			}
			virtual std::string identification() {
				return "cuda host-based version of binary shadow collector. (the bouncing shoud *actually* be done on the gpu).";
			}
		};
	}

	/*! \brief Ray generator for point lights (that includes spotlights).
	 *
	 *  Rays are cast from the receiving points to the light source.
	 *  Another option (which should be faster -> test) would be to trace from the light source to the receiver points.
	 */
	class pointlight_shadow_ray_generator : public ray_generator {
		vec3_t light_pos;
		const image<vec3_t, 1> *visible;
	public:
		pointlight_shadow_ray_generator(int w, int h);
		void setup(const image<vec3_t, 1> *hitpoints, const vec3_t &lp);
		void generate_rays();
		virtual void dont_forget_to_initialize_max_t() {}
		virtual std::string identification() { return "simple pointlight shadow ray generator based on hitpoint information."; }
	};
}

#endif

