#include "rta-cgls-connection.h"
#include <librta/material.h>

using namespace std;

namespace rta {

	connection::connection(const std::string &name) throw (cannot_load_rta_plugin) {
		if (load_plugin_functions(name))
			cout << "rt plugin loaded just fine." << endl;
		else {
			cout << "could not load rt plugin." << endl;
			throw cannot_load_rta_plugin(name);
		}

		char *argv[] = { (char*)"rt" };
		plugin_parse_cmdline(1, argv);
		plugin_initialize();
	}

	namespace cgls {

		void connection::count_ftl_entries(unsigned int vertices, unsigned int indices, unsigned int start, 
		                                   unsigned int len, drawelement_ref de, mesh_ref mesh, conversion_state &state) {
			if (drawelement_hidden(de)) return;
// 			cout << "counting " << drawelement_name(de) << "\t(" << len/3 << ")" << endl;
			state.triangles += len/3;
		}

		/*! in this part we do make quite some assumptions about the vertex buffer layout. */
		void connection::insert_tris(unsigned int vertices, unsigned int indices, unsigned int start, 
		                             unsigned int len, drawelement_ref de, mesh_ref mesh, conversion_state &state) {
			if (drawelement_hidden(de)) return;
// 			cout << "inserting " << drawelement_name(de) << "\t(" << len/3 << ")" << endl;
			int buffers = vertex_buffers_in_mesh(mesh);
			int *index_buffer = (int*)cpu_index_buffer_of_mesh(mesh);
			float **vertex_buffers = (float**)cpu_vertex_buffers_of_mesh(mesh);

			matrix4x4f *trafo = drawelement_trafo(de);

			for (int i = 0; i < len/3; ++i) {
				vec3f *v = &(state.ftl->triangle[state.offset+i].a);
				vec3f *n = &(state.ftl->triangle[state.offset+i].na);
				vec2f *t = &(state.ftl->triangle[state.offset+i].ta);
				for (int j = 0; j < 3; ++j) {
					v[j].x = vertex_buffers[0][3*index_buffer[start + 3*i + j]+0];
					v[j].y = vertex_buffers[0][3*index_buffer[start + 3*i + j]+1];
					v[j].z = vertex_buffers[0][3*index_buffer[start + 3*i + j]+2];
					n[j].x = vertex_buffers[1][3*index_buffer[start + 3*i + j]+0];
					n[j].y = vertex_buffers[1][3*index_buffer[start + 3*i + j]+1];
					n[j].z = vertex_buffers[1][3*index_buffer[start + 3*i + j]+2];
					if (buffers > 2) {
						t[j].x = vertex_buffers[2][2*index_buffer[start + 3*i + j]+0];
						t[j].y = vertex_buffers[2][2*index_buffer[start + 3*i + j]+1];
					}
					if (trafo) {
						vec4f v4 = { v[j].x, v[j].y, v[j].z, 1 }, res;
						multiply_matrix4x4f_vec4f(&res, trafo, &v4);
						v[j].x = res.x, v[j].y = res.y, v[j].z = res.z;
					}
				}
				state.ftl->triangle[state.offset+i].material_index = state.material_map[drawelement_material(de).id];
			}
			state.offset += len/3;
		}

		void connection::convert_materials(unsigned int vertices, unsigned int indices, unsigned int start, 
		                                   unsigned int len, drawelement_ref de, mesh_ref mesh, conversion_state &state) {
			if (drawelement_hidden(de)) return;
			material_ref cgls_mat = drawelement_material(de);
			if (state.material_map.find(cgls_mat.id) != state.material_map.end())
				return;
			vec4f *v4 = material_diffuse_color(cgls_mat);
			vec3f v3 {v4->x, v4->y, v4->z};

			auto texture_called = [&](const char *name)->texture_ref 
									{
										for (struct texture_node *run = material_textures(cgls_mat); run; run = run->next)
											if (strcmp(run->name, name) == 0)
												return run->ref;
										return {-1};
									};

			texture_ref diffuse = texture_called("diffuse_tex");
			string texname = "";
			if (valid_texture_ref(diffuse)) {
				texname = texture_source_filename(diffuse);
			}

			rta::material_t *mat = new rta::material_t(material_name(cgls_mat), v3, texname);
			int id = rta::register_material(mat);
			state.material_map[cgls_mat.id] = id;
			cout << "made material " << id << " (" << material_name(cgls_mat) << " " << v3.x << " " << v3.y << " " << v3.z << " " << texname << ")"<< endl;
		}

		void connection::apply_to_scene(handler_t call, conversion_state &state) {
			drawelement_node *list = scene_drawelements(state.scene);

			for (drawelement_node *run = list; run; run = run->next) {
				bool using_index_range = drawelement_using_index_range(run->ref);
				unsigned int range_start = drawelement_index_range_start(run->ref);
				unsigned int range_len = drawelement_index_range_len(run->ref);
				mesh_ref mesh = drawelement_mesh(run->ref);
				unsigned int vertices = vertex_buffer_length_of_mesh(mesh);
				unsigned int indices = index_buffer_length_of_mesh(mesh);

				if (indices == 0 || !using_index_range) {
					cerr << "cannot handle drawelement " << drawelement_name(run->ref) << "! ! !" << endl;
					continue;
				}

				call(vertices, indices, range_start, range_len, run->ref, mesh, state);
			}
		}
			
		rta::flat_triangle_list* connection::convert_scene_to_ftl(scene_ref scene) {
			conversion_state state;
			state.scene = scene;

			apply_to_scene(count_ftl_entries, state);
			apply_to_scene(convert_materials, state);
			state.ftl = new flat_triangle_list(state.triangles);
			apply_to_scene(insert_tris, state);

			return state.ftl;
		}


	}
}


/* vim: set foldmethod=marker: */

