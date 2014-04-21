#ifndef __LIBHYB_TRACERS_H__ 
#define __LIBHYB_TRACERS_H__ 

#include <iostream>
#include <algorithm>
#include <libcgl/cgl.h>
#include <librta/material.h>
#include <png++/png.hpp>

#include "rta-cgls-connection.h"
#include "trav-util.h"

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
		vec3f *hitpoints, *normals;
		vec3f *material;
		cam_ray_generator_shirley *crgs;
		rta::primary_intersection_collector<rta::simple_aabb, rta::simple_triangle> *cpu_bouncer;
	public:
		simple_material(rt_set<simple_aabb, simple_triangle> &org_set, int w, int h) 
		: w(w), h(h), material(0), crgs(0), cpu_bouncer(0), hitpoints(0), normals(0) {
			material = new vec3f[w*h];
			hitpoints = new vec3f[w*h];
			normals = new vec3f[w*h];
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
						ti.barycentric_coord(&bc);
						const vec3_t &va = vertex_a(tri);
						const vec3_t &vb = vertex_b(tri);
						const vec3_t &vc = vertex_c(tri);
						barycentric_interpolation(hitpoints+y*w+x, &bc, &va, &vb, &vc);
						const vec3_t &na = normal_a(tri);
						const vec3_t &nb = normal_b(tri);
						const vec3_t &nc = normal_c(tri);
						barycentric_interpolation(normals+y*w+x, &bc, &na, &nb, &nc);
					}
					else {
						make_vec3f(hitpoints+y*w+x, FLT_MAX, FLT_MAX, FLT_MAX);
						make_vec3f(normals+y*w+x, 0, 0, 0);
					}
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
					image.set_pixel(w-x-1, y_out, png::rgb_pixel(clamp(255*pixel->x,0,255), clamp(255*pixel->y,0,255), clamp(255*pixel->z,0,255))); 
				}
			}
			image.write("out.png");
		}
	};


	class simple_lighting : public simple_material {
	protected:
		list<light_ref> lights;
		vec3f *lighting_buffer;
		scene_ref the_scene;

	public:
		simple_lighting(rt_set<simple_aabb, simple_triangle> &org_set, int w, int h, scene_ref the_scene) 
		: simple_material(org_set, w, h), lighting_buffer(0), the_scene(the_scene) {
			lighting_buffer = new vec3f[w*h];
		}

		void find_lights() {
			lights.clear();
			for (light_list *ll = scene_lights(the_scene); ll; ll = ll->next)
				lights.push_back(ll->ref);
		}

		void clear_lighting_buffer(vec3f *buffer) {
			vec3f null {0,0,0};
			fill(buffer, buffer+w*h, null);
		}

		virtual void add_lighting(vec3f *lighting_buffer) {
			for (light_ref ref : lights)
				add_lighting(ref, lighting_buffer);
		}

		inline vec3f spot_contrib(light_ref ref, int x, int y, vec3f *pos, vec3f *normal, vec3f *dir, float spot_cos_cutoff, float cutoff) {
			vec3f *hitpoint = hitpoints+y*w+x;
			vec3f l = *pos - *hitpoint;
			float distance = length_of_vec3f(&l);
			normalize_vec3f(&l);
			float ndotl = max(dot_vec3f(normal, &l), 0.0f);
			if (ndotl > 0) {
				vec3f ml = -l;
				float cos_theta = dot_vec3f(dir, &ml);
				if (cos_theta > spot_cos_cutoff) {
					float spot_factor = cos_theta;
					float angle = acos(cos_theta);
					spot_factor *= 1.0 - smoothstep(cutoff * .7, cutoff, angle);
					return *light_color(ref) *spot_factor *ndotl;
				}
			}
			return {0,0,0};
		}

		inline vec3f spot_contrib(light_ref ref, int x, int y) {
				float cutoff = *(float*)light_aux(ref);
				matrix4x4f *trafo = light_trafo(ref);
				vec3f pos, dir;
				extract_pos_vec3f_of_matrix(&pos, trafo);
				extract_dir_vec3f_of_matrix(&dir, trafo);
				normalize_vec3f(&dir);
				float spot_cos_cutoff = cos(cutoff);
				vec3f *normal = normals+y*w+x;
				return spot_contrib(ref, x, y, &pos, normal, &dir, spot_cos_cutoff, cutoff);
		}

		virtual void add_lighting(light_ref ref, vec3f *lighting_buffer) {
			int type = light_type(ref);
			if (type == hemi_light_t) {
				vec3f *dir = (vec3f*)light_aux(ref);
				vec3f tmp;
				for (int y = 0; y < h; ++y)
					for (int x = 0; x < w; ++x) {
						vec3f *n = normals+y*w+x;
						if (n->x != 0 || n->y != 0 || n->z != 0) {
							float factor = 0.5*(1+dot_vec3f(n, dir));
							mul_vec3f_by_scalar(&tmp, light_color(ref), factor);
							add_components_vec3f(lighting_buffer+y*w+x, &tmp, lighting_buffer+y*w+x);
						}
					}
			}
			else if (type == spot_light_t) {
				float cutoff = *(float*)light_aux(ref);
				matrix4x4f *trafo = light_trafo(ref);
				vec3f pos, dir, up;
				extract_pos_vec3f_of_matrix(&pos, trafo);
				extract_dir_vec3f_of_matrix(&dir, trafo);
				extract_up_vec3f_of_matrix(&up, trafo);
				normalize_vec3f(&dir);
				float spot_cos_cutoff = cos(cutoff);
				for (int y = 0; y < h; ++y)
					for (int x = 0; x < w; ++x) {
						vec3f *normal = normals+y*w+x;
						if (normal->x != 0 || normal->y != 0 || normal->z != 0) {
							vec3f lighting = spot_contrib(ref, x, y, &pos, normal, &dir, spot_cos_cutoff, cutoff);
							add_components_vec3f(lighting_buffer+y*w+x, lighting_buffer+y*w+x, &lighting);
						}
					}
			}
			else
				cerr << "UNKOWN LIGHT TYPE " << type << endl;
		}

		virtual void shade(vec3f *out, vec3f *lighting_buffer) {
			for (int y = 0; y < h; ++y)
				for (int x = 0; x < w; ++x)
					mul_components_vec3f(out+y*w+x, material+y*w+x, lighting_buffer+y*w+x);
		}

		virtual void compute() {
			primary_visibility();
			evaluate_material();
			find_lights();
			clear_lighting_buffer(lighting_buffer);
			add_lighting(lighting_buffer);
			shade(lighting_buffer, lighting_buffer);
			save(lighting_buffer);
		}
	};

	class simple_lighting_with_shadows : public simple_lighting {
		list<vec3f*> lighting_buffer; // we use the same name to avoid accidental use of simple_lighting::lighting_buffer.
	public:
		simple_lighting_with_shadows(rt_set<simple_aabb, simple_triangle> &org_set, int w, int h, scene_ref the_scene) 
		: simple_lighting(org_set, w, h, the_scene) {
		}
		
		void adjust_lighting_buffers() {
			while (lights.size() < lighting_buffer.size()) {
				delete [] lighting_buffer.back();
				lighting_buffer.pop_back();
			}
			while (lights.size() > lighting_buffer.size())
				lighting_buffer.push_back(new vec3f[w*h]);
		}

		void fill_lighting_buffers() {
			auto buffer_iter = lighting_buffer.begin();
			for (auto light_iter = lights.begin(); light_iter != lights.end(); ++light_iter, ++buffer_iter) {
				add_lighting(*light_iter, *buffer_iter);
			}
		}

		virtual void compute() {
			primary_visibility();
			evaluate_material();
			find_lights();
			adjust_lighting_buffers();
			for (auto buf : lighting_buffer)
				clear_lighting_buffer(buf);
			fill_lighting_buffers();
		}
	};
}

#endif

