#ifndef __RTA_CGLS_CONNECTION_H__ 
#define __RTA_CGLS_CONNECTION_H__ 

#include <librta/librta.h>
#include <librta/plugins.h>

#include <libcgls/cgls.h>
#include <string>
#include <stdexcept>

namespace rta {
	
	struct cannot_load_rta_plugin : public std::runtime_error {
	public:
		explicit cannot_load_rta_plugin(const std::string &name) : std::runtime_error("cannot load plugin: " + name) {}
	};

	class connection {
	public:
		connection(const std::string &plugin_name) throw (cannot_load_rta_plugin);
	};


	namespace cgls {

		/*! use like
		 *  <code>
		 *  	static rta::flat_triangle_list *ftl = rta::cgls::connection::convert_scene_to_ftl(the_scene);
		 *  	rta::rt_set<rta::simple_aabb, rta::simple_triangle> set = rta::plugin_create_rt_set(*ftl, rays_w, rays_h);
		 *	</code>
		 *
		 *	note that the cgls objloader has to be setup to keep the vertex data on the cpu. 
		 *	to do so change the default meaning prior to scene loading as follows:
		 *	<code>
		 *		scm_c_eval_string("(define (load-objfile-and-create-objects-with-single-vbo filename objname callback fallback-mat merge-factor)\
		 *			                  (load-objfile-and-create-objects-with-single-vbo-general filename objname callback fallback-mat #t merge-factor))");
		 *	</code>
		 */
		struct connection : public rta::connection {
		protected:
			struct conversion_state {
				scene_ref scene;
				int triangles, offset;
				rta::flat_triangle_list *ftl;
				conversion_state() : triangles(0), offset(0), ftl(0) { scene.id = -1; }
			};
			typedef void (*handler_t)(unsigned int vertices, unsigned int indices, 
			                          unsigned int start, unsigned int len, 
			                          drawelement_ref de, mesh_ref mesh, conversion_state &state);
			static void count_ftl_entries(unsigned int vertices, unsigned int indices, unsigned int start, 
										  unsigned int len, drawelement_ref de, mesh_ref mesh, conversion_state &state);
			static void insert_tris(unsigned int vertices, unsigned int indices, unsigned int start, 
									unsigned int len, drawelement_ref de, mesh_ref mesh, conversion_state &state);
			static void apply_to_scene(handler_t call, conversion_state &state);

		public:
			connection(const std::string &plugin_name) 
				throw (cannot_load_rta_plugin)
					: rta::connection(plugin_name) {
			}

			static rta::flat_triangle_list* convert_scene_to_ftl(scene_ref scene);
		};

	}
}


#endif

