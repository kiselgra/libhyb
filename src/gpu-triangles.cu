#include <librta/basic_types.h>
#include <librta/cuda-vec.h>
#include <librta/cuda-kernels.h>

#include <iostream>

using namespace rta;
using namespace std;

namespace rta {
	namespace cgls {
		
		namespace k {
			static __global__ void update_triangle_data(cuda::simple_triangle *tri, int N, int offset, 
												 float3 *v, float3 *n, float2 *t, uint3 *I, int triangles,
												 int wg_size, int work_groups, int iter, int3 *wg_data) {

				uint local_thread_id = threadIdx.x;
				int work_group_id = blockIdx.x;
				
				int3 wgd = wg_data[work_group_id];
				#define batch_offset   (wgd.x)
				#define batch_size     (wgd.y)
				#define batch_material (wgd.z)

				for (int i = 0; i < iter; ++i) {
					if (wg_size * i + local_thread_id >= batch_size)
						break;

					int tri_id = batch_offset    					// batch tri-offset depends on drawelement sizes, as not all batches are filled.
					           + wg_size * i						// iteration offset in batch
							   + local_thread_id;					// current triangle in sub-batch
					cuda::simple_triangle out;
					uint3 indices = I[tri_id];
					*((float3*)&out.a)  = v[indices.x];
					*((float3*)&out.b)  = v[indices.y];
					*((float3*)&out.c)  = v[indices.z];
					*((float3*)&out.na) = n[indices.x];
					*((float3*)&out.nb) = n[indices.y];
					*((float3*)&out.nc) = n[indices.z];
					*((float2*)&out.ta) = t[indices.x];
					*((float2*)&out.tb) = t[indices.y];
					*((float2*)&out.tc) = t[indices.z];
					out.material_index = batch_material;
					tri[tri_id] = out;
				}
				
			}
		}
		
		void update_triangle_data(basic_flat_triangle_list<cuda::simple_triangle> &ftl, int offset, 
								  float3 *v, float3 *n, float2 *t, uint3 *I, int triangles,
								  int wg_size, int iter, int work_groups, int3 *wg_data) {

			dim3 threads(wg_size);
			dim3 blocks = rta::cuda::block_configuration_2d(wg_size*work_groups, 1, threads);

			cout << "Conversion." << endl;
			cout << "  Triangles:   " << ftl.triangles << endl;
			cout << "  Offset:      " << offset << endl;
			cout << "  Triangles2:  " << triangles << endl;
			cout << "  wg_size:     " << wg_size << endl;
			cout << "  iter:        " << iter << endl;
			cout << "  work_groups: " << work_groups << endl;
			k::update_triangle_data<<<blocks,threads>>>(ftl.triangle, ftl.triangles, offset,
														v, n, t, I, triangles,
														wg_size, iter, work_groups/100, wg_data);
		}

	}
}
