#ifndef __LIBHYB_TRACERS_H__ 
#define __LIBHYB_TRACERS_H__ 

#include <iostream>
#include <algorithm>
#include <libcgl/cgl.h>
#include <librta/material.h>
#include <librta/cuda.h>
#include <png++/png.hpp>

#include "rta-cgls-connection.h"
#include "trav-util.h"
#include "shadowrays.h"

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

	/*! A very simple example that traces primary rays and evaluates the material at the hitpoints.
	 *  No shading is applied.
	 */
	template<typename box_t, typename tri_t> class simple_material : public use_case {
	protected:
		rt_set set;
		typedef typename tri_t::vec3_t vec3_t;
		typedef typename rta::vector_traits<vec3_t>::vec2_t vec2_t;
		int w, h;
		image<vec3f, 1> hitpoints, normals;
		vec3f *material;
		cam_ray_generator_shirley *crgs;
		rta::primary_intersection_collector<box_t, tri_t> *primary;

		rta::primary_intersection_collector<box_t, tri_t>* make_primary_intersection_collector(rta::simple_triangle t, int w, int h) {
			return new rta::primary_intersection_collector<box_t, tri_t>(w, h);
		}
		rta::primary_intersection_collector<box_t, tri_t>* make_primary_intersection_collector(rta::cuda::simple_triangle t, int w, int h) {
			return new rta::cuda::primary_intersection_downloader<box_t, tri_t, primary_intersection_collector<box_t, tri_t>>(w, h);
		}
		rta::cam_ray_generator_shirley* make_crgs(rta::simple_triangle t, int w, int h) {
			return new rta::cam_ray_generator_shirley(w, h);
		}
		rta::cam_ray_generator_shirley* make_crgs(rta::cuda::simple_triangle t, int w, int h) {
			return new rta::cuda::raygen_with_buffer<rta::cam_ray_generator_shirley>(w, h);
		}

	public:
		simple_material(rt_set &org_set, int w, int h) 
		: w(w), h(h), material(0), crgs(0), primary(0), hitpoints(w,h), normals(w,h) {
			material = new vec3f[w*h];
			set = org_set;
			set.rt = org_set.rt->copy();
			set.bouncer = primary = make_primary_intersection_collector(tri_t(), w, h);
			set.rgen = crgs = make_crgs(tri_t(), w, h);
			set.basic_rt<box_t, tri_t>()->ray_bouncer(set.bouncer);
			set.basic_rt<box_t, tri_t>()->ray_generator(set.rgen);
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

			tri_t *triangles = set.basic_as<box_t, tri_t>()->canonical_triangle_ptr();
			vec3_t bc, tmp;
			for (int y = 0; y < h; ++y)
				for (int x = 0; x < w; ++x) {
					const triangle_intersection<tri_t> &ti = primary->intersection(x,y);
					if (ti.valid()) {
						tri_t &tri = triangles[ti.ref];
						ti.barycentric_coord(&bc);
						const vec3_t &va = vertex_a(tri);
						const vec3_t &vb = vertex_b(tri);
						const vec3_t &vc = vertex_c(tri);
						barycentric_interpolation(&tmp, &bc, &va, &vb, &vc);
						hitpoints.pixel(x,y) = { tmp.x, tmp.y, tmp.z };
						const vec3_t &na = normal_a(tri);
						const vec3_t &nb = normal_b(tri);
						const vec3_t &nc = normal_c(tri);
						barycentric_interpolation(&tmp, &bc, &na, &nb, &nc);
						normals.pixel(x,y) = { tmp.x, tmp.y, tmp.z };
					}
					else {
						make_vec3f(&hitpoints.pixel(x,y), FLT_MAX, FLT_MAX, FLT_MAX);
						make_vec3f(&normals.pixel(x,y), 0, 0, 0);
					}
				}
			set.basic_as<box_t, tri_t>()->free_canonical_triangles(triangles);
		}

		virtual void evaluate_material() {
			cout << "evaluating material..." << endl;
			tri_t *triangles = set.basic_as<box_t, tri_t>()->canonical_triangle_ptr();
			for (int y = 0; y < h; ++y) {
				int y_out = h - y - 1;
				for (int x = 0; x < w; ++x) {
					const triangle_intersection<tri_t> &ti = primary->intersection(x,y);
					if (ti.valid()) {
						tri_t &tri = triangles[ti.ref];
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
							vec2f t = { T.x, T.y };
							col = (*mat)(t);
						}
						material[y*w+x] = col;
					}
					else
						make_vec3f(material+y*w+x, 0, 0, 0);
				}
			}
			set.basic_as<box_t, tri_t>()->free_canonical_triangles(triangles);
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

	
	/*! An extention of the simple_material example that adds lighting by spotlights and hemispherical lights.
	 *  Does not compute shadows.
	 */
	template<typename box_t, typename tri_t> class simple_lighting : public simple_material<box_t, tri_t> {
	protected:
		list<light_ref> lights;
		vec3f *lighting_buffer;
		scene_ref the_scene;
		typedef simple_material<box_t, tri_t> parent;
		using parent::w;
		using parent::h;
		using parent::hitpoints;
		using parent::normals;
		using parent::material;

	public:
		simple_lighting(rt_set &org_set, int w, int h, scene_ref the_scene) 
		: simple_material<box_t, tri_t>(org_set, w, h), lighting_buffer(0), the_scene(the_scene) {
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
			vec3f *hitpoint = &hitpoints.pixel(x,y);
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
				vec3f *normal = &normals.pixel(x,y);
				return spot_contrib(ref, x, y, &pos, normal, &dir, spot_cos_cutoff, cutoff);
		}

		virtual void add_lighting(light_ref ref, vec3f *lighting_buffer) {
			int type = light_type(ref);
			if (type == hemi_light_t) {
				vec3f *dir = (vec3f*)light_aux(ref);
				vec3f tmp;
				for (int y = 0; y < h; ++y)
					for (int x = 0; x < w; ++x) {
						vec3f *n = &normals.pixel(x,y);
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
						vec3f *normal = &normals.pixel(x,y);
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
			this->primary_visibility();
			this->evaluate_material();
			find_lights();
			clear_lighting_buffer(lighting_buffer);
			add_lighting(lighting_buffer);
			shade(lighting_buffer, lighting_buffer);
			this->save(lighting_buffer);
		}
	};


	/*! A shadowing pass to compute visibility of a set of hitpoints from a point light source.
	 *  Does not handle sender/receiver geometry, this should be done when shading.
	 */
	template<typename box_t, typename tri_t> class shadow_computation_for_pointlight {
	public:
		rt_set shadow_set;
		binary_shadow_collector<box_t, tri_t> *shadow_bouncer;
		pointlight_shadow_ray_generator *plsrg;
		light_ref ref;

		binary_shadow_collector<box_t, tri_t>* make_shadow_collector(rta::simple_triangle, int w, int h) {
			return new rta::binary_shadow_collector<box_t, tri_t>(w, h);
		}
		binary_shadow_collector<box_t, tri_t>* make_shadow_collector(rta::cuda::simple_triangle, int w, int h) {
			return new rta::cuda::hackish_binary_shadow_collector<box_t, tri_t>(w, h);
		}
		pointlight_shadow_ray_generator* make_plrg(rta::simple_triangle, int w, int h) {
			return new rta::pointlight_shadow_ray_generator(w, h);
		}
		pointlight_shadow_ray_generator* make_plrg(rta::cuda::simple_triangle, int w, int h) {
			return new rta::cuda::raygen_with_buffer<rta::pointlight_shadow_ray_generator>(w, h);
		}

		shadow_computation_for_pointlight(rt_set &org_set, int w, int h, light_ref light) {
			shadow_set = org_set;
			shadow_set.rt = org_set.rt->copy();
			shadow_set.bouncer = shadow_bouncer = make_shadow_collector(tri_t(), w, h);
			shadow_set.rgen = plsrg = make_plrg(tri_t(), w, h);
			shadow_set.basic_rt<box_t, tri_t>()->ray_bouncer(shadow_set.bouncer);
			shadow_set.basic_rt<box_t, tri_t>()->ray_generator(shadow_set.rgen);
			ref = light;
		}
		void trace(const image<vec3f,1> *hitpoints) {
			cout << "computing shadows for light " << light_name(ref) << "... " << endl;
			matrix4x4f *trafo = light_trafo(ref);
			vec3f pos;
			extract_pos_vec3f_of_matrix(&pos, trafo);
			plsrg->setup(hitpoints, pos);
			shadow_set.rt->trace();
		}
	};


	/*! An extension of the simple_lighting example incorporating hard shadows for spotlights.
	 *  Hemi lights do not cast shadows.
	 */
	template<typename box_t, typename tri_t> class simple_lighting_with_shadows : public simple_lighting<box_t, tri_t> {
	protected:
		typedef simple_lighting<box_t, tri_t> parent;
		using parent::w;
		using parent::h;
		using parent::hitpoints;
		using parent::normals;
		using parent::material;
		using parent::lights;

		vector<light_ref> point_lights;
		vector<vec3f*> lighting_buffer; // we use the same name to avoid accidental use of simple_lighting::lighting_buffer.
		rt_set base_shadow_set;
		vector<shadow_computation_for_pointlight<box_t, tri_t> > shadow_passes;
	public:
		simple_lighting_with_shadows(rt_set &org_set, int w, int h, scene_ref the_scene) 
		: simple_lighting<box_t, tri_t>(org_set, w, h, the_scene) {
			base_shadow_set = org_set;
			base_shadow_set.rt = org_set.rt->copy();
			for (light_list *ll = scene_lights(the_scene); ll; ll = ll->next) {
				int type = light_type(ll->ref);
				if (type == spot_light_t) {
					point_lights.push_back(ll->ref);
					lighting_buffer.push_back(new vec3f[w*h]);
					shadow_passes.push_back(shadow_computation_for_pointlight<box_t, tri_t> (base_shadow_set, w, h, ll->ref));
				}
				else
					lights.push_back(ll->ref);
			}
		}
		
		void fill_lighting_buffers() {
			// accumulate light of unsupported light types in the general lighting buffer.
			for (auto light_iter = lights.begin(); light_iter != lights.end(); ++light_iter) {
				this->add_lighting(*light_iter, parent::lighting_buffer);
			}
			for (int i = 0; i < shadow_passes.size(); ++i) {
				shadow_passes[i].trace(&hitpoints);
				this->add_lighting(point_lights[i], lighting_buffer[i]);
				for (int y = 0; y < h; ++y)
					for (int x = 0; x < w; ++x) {
						vec3f *normal = &normals.pixel(x,y);
						if (normal->x != 0 || normal->y != 0 || normal->z != 0) {
							mul_vec3f_by_scalar(lighting_buffer[i]+y*w+x, lighting_buffer[i]+y*w+x, shadow_passes[i].shadow_bouncer->factor(x, y));
							add_components_vec3f(parent::lighting_buffer+y*w+x, parent::lighting_buffer+y*w+x, lighting_buffer[i]+y*w+x);
						}
					}
			}
		}

		virtual void compute() {
			this->primary_visibility();
			this->evaluate_material();
			this->clear_lighting_buffer(parent::lighting_buffer);
			for (auto buf : lighting_buffer)
				this->clear_lighting_buffer(buf);
			fill_lighting_buffers();
			this->shade(parent::lighting_buffer, parent::lighting_buffer);
			this->save(parent::lighting_buffer);
		}
	};
}

#endif

