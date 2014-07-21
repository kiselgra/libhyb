#include "rta-cgls-connection.h"
#include <librta/material.h>
#include <librta/cuda.h>
#include <librta/cuda-kernels.h>

#include <cuda_gl_interop.h>

#include <map>

using namespace std;

bool operator<(const mesh_ref &a, const mesh_ref &b) {
	return a.id < b.id;
}

namespace rta {

	connection::connection(const std::string &name, const std::vector<std::string> &args) throw (cannot_load_rta_plugin) {
		if (load_plugin_functions(name))
			cout << "rt plugin loaded just fine." << endl;
		else {
			cout << "could not load rt plugin." << endl;
			throw cannot_load_rta_plugin(name);
		}

		if (args.size() >= 48)
			throw "too many arguments to plugin. extend libhyb!";
		const char *argv[50];
		argv[0] = (char*)"rt";
		for (int i = 0; i < args.size(); ++i)
			argv[i+1] = args[i].c_str();
		for (int i = args.size()+1; i < 50; ++i)
			argv[i] = 0;
		plugin_parse_cmdline(1+args.size(), (char**)argv);
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
			
		rta::basic_flat_triangle_list<rta::simple_triangle>* connection::convert_scene_to_ftl(scene_ref scene) {
			conversion_state state;
			state.scene = scene;

			apply_to_scene(count_ftl_entries, state);
			apply_to_scene(convert_materials, state);
			state.ftl = new basic_flat_triangle_list<rta::simple_triangle>(state.triangles);
			apply_to_scene(insert_tris, state);

			return state.ftl;
		}

			
		connection::cuda_triangle_data* connection::convert_scene_to_cuda_triangle_data(scene_ref scene) {
			cuda_triangle_data *data = new cuda_triangle_data;
			data->work_group_size = 512;
			data->work_group_iterations = 2;
			int batch_size = data->work_group_size * data->work_group_iterations;

			// convert materials first, so we can refer to them when building the cuda triangle base representation
			conversion_state state;
			state.scene = scene;
			apply_to_scene(convert_materials, state);

			// sort drawelements by mesh (i.e. by gl buffers), acumulate triangle count
			for (drawelement_node *run = scene_drawelements(scene); run; run = run->next)
				if (drawelement_using_index_range(run->ref) && drawelement_index_range_len(run->ref) > 0) {
					cuda_triangle_data::drawelement_conversion conv(run->ref);
					conv.material_id = state.material_map[drawelement_material(run->ref).id];
					conv.offset      = drawelement_index_range_start(run->ref)/3;
					conv.length      = drawelement_index_range_len(run->ref)/3;
					conv.work_groups = conv.length / batch_size;
					if (conv.length % batch_size != 0)
						conv.work_groups++;
					cuda_triangle_data::group_data *group = data->drawelement_by_mesh[drawelement_mesh(run->ref)];
					if (group == 0)
						group = data->drawelement_by_mesh[drawelement_mesh(run->ref)] = new cuda_triangle_data::group_data;
					group->elements.push_back(conv);
					group->triangles += conv.length;
					group->work_groups += conv.work_groups;
				}
				else
					cerr << "cannot handle drawelement " << drawelement_name(run->ref) << "! ! !" << endl;

			// compute global group triangle data offset and allocate gpu storage
			int offset = 0;
			for (auto it : data->drawelement_by_mesh) {
				it.second->offset = offset;
				offset += it.second->triangles;
			}
			data->ftl.triangles = offset;
			cudaMalloc((void**)&data->ftl.triangle, sizeof(cuda::simple_triangle) * data->ftl.triangles);

			int II= 9;
			// fill in work group data, i.e. offset to mesh-local data, work group's triangle, and current batch's material id.
			// this is per mesh, as we have to rebind the gl buffers for the copy process
			for (auto it : data->drawelement_by_mesh) {
				cuda_triangle_data::group_data *group = it.second;
				group->id = II++;
				cout << "make group " << group->id << endl;
				int3 *host_data = new int3[group->work_groups];
				checked_cuda(cudaMalloc((void**)&group->work_group_data, sizeof(int3) * group->work_groups));
				int batch = 0;
				int offset = 0;
				for (int i = 0; i < group->elements.size(); ++i) {
					cuda_triangle_data::drawelement_conversion &element = group->elements[i];
					for (int g = 0; g < element.work_groups; ++g) {
						host_data[batch].x = offset;
						if (g == element.work_groups-1)
							host_data[batch].y = element.length % batch_size;
						else
							host_data[batch].y = batch_size;
						host_data[batch].z = element.material_id;
						offset += host_data[batch].y;
						++batch;
					}
				}
				// start memcpy immediately, the update remaining data fields
				checked_cuda(cudaMemcpy(group->work_group_data, host_data, sizeof(int3)*group->work_groups, cudaMemcpyHostToDevice));
				if (vertex_buffers_in_mesh(it.first) == 3)
					group->has_tex_coords = true;
				else
					group->has_tex_coords = false;
				group->vbo_v.register_buffer(mesh_vertex_buffer(it.first, 0));
				group->vbo_n.register_buffer(mesh_vertex_buffer(it.first, 1));
				if (group->has_tex_coords)
					group->vbo_t.register_buffer(mesh_vertex_buffer(it.first, 2));
				group->vbo_i.register_buffer(mesh_indexbuffer(it.first));
				// still, we have to wait for the memcpy before we free
				checked_cuda(cudaDeviceSynchronize());
				delete [] host_data;
			}

			data->update();
			return data;
		}

		void connection::cuda_triangle_data::update() {
			drawelement_ref inv = { -1 };
			update(inv);
		}

		void update_triangle_data(basic_flat_triangle_list<cuda::simple_triangle> &ftl, int offset, 
								  float3 *v, float3 *n, float2 *t, uint3 *I, int triangles,
								  int wg_size, int iter, int work_groups, int3 *wg_data);

		// HIDDEN DES WILL NOT WORK
		void connection::cuda_triangle_data::update(drawelement_ref ref) {
			for (auto it : drawelement_by_mesh) {
				cuda_triangle_data::group_data *group = it.second;
				cout << "trav group " << group->id << endl;
				group->vbo_v.map();
				group->vbo_n.map();
				if (group->has_tex_coords) group->vbo_t.map();
				group->vbo_i.map();
			
				cout << "  v: " << group->vbo_v.size << endl;
				cout << "  n: " << group->vbo_n.size << endl;
				cout << "  t: " << group->vbo_t.size << endl;
				cout << "  I: " << group->vbo_i.size << endl;


				update_triangle_data(ftl, group->offset,
									 (float3*)group->vbo_v.device_ptr, (float3*)group->vbo_n.device_ptr, 
									 group->has_tex_coords ? (float2*)group->vbo_t.device_ptr : 0, 
									 (uint3*)group->vbo_i.device_ptr, group->triangles,
									 work_group_size, work_group_iterations, group->work_groups, group->work_group_data);
				
				group->vbo_v.unmap();
				group->vbo_n.unmap();
				if (group->has_tex_coords) group->vbo_t.unmap();
				group->vbo_i.unmap();
			}
		}
	
	
		basic_flat_triangle_list<simple_triangle> connection::cuda_triangle_data::cpu_ftl() {
			basic_flat_triangle_list<simple_triangle> new_ftl(ftl.triangles);
			checked_cuda(cudaMemcpy(new_ftl.triangle, ftl.triangle, sizeof(simple_triangle)*ftl.triangles, cudaMemcpyDeviceToHost));
			checked_cuda(cudaDeviceSynchronize());
			return new_ftl;
		}

		connection::cuda_mapped_gl_buffer::cuda_mapped_gl_buffer(int gl_buffer) : gl_buffer(gl_buffer), mapped(false) {
			if (gl_buffer)
				register_buffer(gl_buffer);
		}
		connection::cuda_mapped_gl_buffer::~cuda_mapped_gl_buffer() {
			if (mapped)
				unmap();
		}
		void connection::cuda_mapped_gl_buffer::register_buffer(int gl_buffer) {
			this->gl_buffer = gl_buffer;
			struct cudaGraphicsResource *vbo_res;
			checked_cuda(cudaGraphicsGLRegisterBuffer(&vbo_res, gl_buffer, cudaGraphicsRegisterFlagsReadOnly));
			graphic_resource = vbo_res;
		}
		void connection::cuda_mapped_gl_buffer::map() {
			cudaGraphicsResource *vbo_res = (cudaGraphicsResource*)graphic_resource;
			checked_cuda(cudaGraphicsMapResources(1, &vbo_res, 0));
			checked_cuda(cudaGraphicsResourceGetMappedPointer(&device_ptr, &size, vbo_res));
			mapped = true;
		}
		void connection::cuda_mapped_gl_buffer::unmap() {
			cudaGraphicsResource *vbo_res = (cudaGraphicsResource*)graphic_resource;
			checked_cuda(cudaGraphicsUnmapResources(1, &vbo_res, 0));
			mapped = false;
		}

	
	
	}
}


/* vim: set foldmethod=marker: */

