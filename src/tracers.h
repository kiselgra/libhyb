#ifndef __TRACERS_H__ 
#define __TRACERS_H__ 

#include <iostream>
#include <algorithm>
#include <libcgl/cgl.h>
#include <librta/material.h>
#include <png++/png.hpp>

#include "rta-cgls-connection.h"

/*  having everything in the header is not really good practice.
 *  i hope this helps to better see what belongs together.
 *  don't do this at home.
 */

namespace example {
	using namespace rta;
	using namespace std;

	class use_case {
	public:
		virtual void compute() = 0;
	};

	class simple_material : public use_case {
	protected:
		rt_set<simple_aabb, simple_triangle> set;
		int w, h;
		vec3f *hitpoints;
		vec3f *material;
		cam_ray_generator_shirley *crgs;
		rta::primary_intersection_collector<rta::simple_aabb, rta::simple_triangle> *cpu_bouncer;
	public:
		simple_material(rt_set<simple_aabb, simple_triangle> &org_set, int w, int h) : w(w), h(h), material(0), crgs(0), cpu_bouncer(0) {
			material = new vec3f[w*h];
			set = org_set;
			set.rt = org_set.rt->copy();
			set.bouncer = cpu_bouncer = new rta::primary_intersection_collector<rta::simple_aabb, rta::simple_triangle>(w, h);
			set.rgen = crgs = new rta::cam_ray_generator_shirley(w, h);
			set.rt->ray_bouncer(set.bouncer);
			set.rt->force_cpu_bouncer(cpu_bouncer); // why oh why
			set.rt->ray_generator(set.rgen);
		}

		virtual void primary_visibility() {
			cout << "tracing..." << endl;
			vec3f pos, dir, up;
			matrix4x4f *lookat_matrix = lookat_matrix_of_cam(current_camera());
			extract_pos_vec3f_of_matrix(&pos, lookat_matrix);
			extract_dir_vec3f_of_matrix(&dir, lookat_matrix);
			extract_up_vec3f_of_matrix(&up, lookat_matrix);
			crgs->setup(&pos, &dir, &up, 2*camera_fovy(current_camera()));
			set.rt->trace();

			vec3f bc;
			for (int y = 0; y < h; ++y)
				for (int x = 0; x < w; ++x) {
					const triangle_intersection<simple_triangle> &ti = cpu_bouncer->intersection(x,y);
					if (ti.valid()) {
						simple_triangle &tri = set.as->triangle_ptr()[ti.ref];
						const vec3_t &va = vertex_a(tri);
						const vec3_t &vb = vertex_b(tri);
						const vec3_t &vc = vertex_c(tri);
						barycentric_interpolation(hitpoints+y*w+x, &bc, &va, &vb, &vc);
					}
					else
						make_vec3f(hitpoints+y*w+x, FLT_MAX, FLT_MAX, FLT_MAX);
				}
		}

		virtual void evaluate_material() {
			cout << "evaluating material..." << endl;
			for (int y = 0; y < h; ++y) {
				int y_out = h - y - 1;
				for (int x = 0; x < w; ++x) {
					const triangle_intersection<simple_triangle> &ti = cpu_bouncer->intersection(x,y);
					if (ti.valid()) {
						simple_triangle &tri = set.as->triangle_ptr()[ti.ref];
						material_t *mat = rta::material(tri.material_index);
						vec3f col = (*mat)();
						if (mat->diffuse_texture) {
							vec3_t bc; 
							ti.barycentric_coord(&bc);
							const vec2_t &ta = texcoord_a(tri);
							const vec2_t &tb = texcoord_b(tri);
							const vec2_t &tc = texcoord_c(tri);
							vec2_t T;
							barycentric_interpolation(&T, &bc, &ta, &tb, &tc);
							col = (*mat)(T);
						}
						material[y*w+x] = col;
					}
					else
						make_vec3f(material+y*w+x, 0, 0, 0);
				}
			}
		}

		virtual void compute() {
			primary_visibility();
			evaluate_material();
			save(material);
		}

		virtual void save(vec3f *out) {
			cout << "saving output" << endl;
			png::image<png::rgb_pixel> image(w, h);
			for (int y = 0; y < h; ++y) {
				int y_out = h - y - 1;
				for (int x = 0; x < w; ++x) {
					vec3f *pixel = out+y*w+x;
					image.set_pixel(x, y_out, png::rgb_pixel(255*pixel->x, 255*pixel->y, 255*pixel->z)); 
				}
			}
			image.write("out.png");
		}
	};


	class simple_lighting : public simple_material {
	protected:
		list<light_ref> lights;
		vec3f *lighting_buffer;

	public:
		simple_lighting(rt_set<simple_aabb, simple_triangle> &org_set, int w, int h) : simple_material(org_set, w, h), lighting_buffer(0) {
			lighting_buffer = new vec3f[w*h];
		}

		void find_lights() {
			extern scene_ref the_scene;
			lights.clear();
			for (light_list *ll = scene_lights(the_scene); ll; ll = ll->next)
				lights.push_back(ll->ref);
		}

		void clear_lighting_buffer() {
			vec3f null {0,0,0};
			fill(lighting_buffer, lighting_buffer+w*h, null);
		}

		virtual void add_lighting(light_ref ref, vec3f *lighting_buffer) {
		}

		virtual void compute() {
			primary_visibility();
			evaluate_material();
			find_lights();
			clear_lighting_buffer();
		}
	};

}

#endif

