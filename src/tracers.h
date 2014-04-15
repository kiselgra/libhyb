#ifndef __TRACERS_H__ 
#define __TRACERS_H__ 

#include <libcgl/cgl.h>
#include <png++/png.hpp>

#include "rta-cgls-connection.h"

/*  having everything in the header is not really good practice.
 *  i hope this helps to better see what belongs together.
 *  don't do this at home.
 */

namespace example {
	using namespace rta;

	class use_case {
	public:
		virtual void compute() = 0;
	};

	class simple_lighting : public use_case {
		rt_set<simple_aabb, simple_triangle> set;
		int w, h;
		cam_ray_generator_shirley *crgs;
		rta::primary_intersection_collector<rta::simple_aabb, rta::simple_triangle> *cpu_bouncer;
	public:
		simple_lighting(rt_set<simple_aabb, simple_triangle> &org_set, int w, int h) : w(w), h(h), crgs(0), cpu_bouncer(0) {
			set = org_set;
			set.rt = org_set.rt->copy();
			set.bouncer = cpu_bouncer = new rta::primary_intersection_collector<rta::simple_aabb, rta::simple_triangle>(w, h);
			set.rgen = crgs = new rta::cam_ray_generator_shirley(w, h);
			set.rt->ray_bouncer(set.bouncer);
			set.rt->force_cpu_bouncer(cpu_bouncer); // why oh why
			set.rt->ray_generator(set.rgen);
		}

		void compute() {
			vec3f pos, dir, up;
			matrix4x4f *lookat_matrix = lookat_matrix_of_cam(current_camera());
			extract_pos_vec3f_of_matrix(&pos, lookat_matrix);
			extract_dir_vec3f_of_matrix(&dir, lookat_matrix);
			extract_up_vec3f_of_matrix(&up, lookat_matrix);
			crgs->setup(&pos, &dir, &up, 2*camera_fovy(current_camera()));
			set.rt->trace();

			save();
		}

		void save() {
			png::image<png::rgb_pixel> image(w, h);
			for (int y = 0; y < h; ++y)
				for (int x = 0; x < w; ++x) {
					const triangle_intersection<simple_triangle> &ti = cpu_bouncer->intersection(x,y);
					if (ti.valid())
						image.set_pixel(x, y, png::rgb_pixel(255,255,255));
					else
						image.set_pixel(x, y, png::rgb_pixel(0,0,0));
				}
			image.write("bin.png");
		}
	};

}

#endif

