#ifndef __PBRT_H__ 
#define __PBRT_H__ 

/*

This file contains functions we took from pbrt.

Copyright (c) 1998-2014, Matt Pharr and Greg Humphreys.All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.Redistributions in binary form
must reproduce the above copyright notice, this list of conditions and the
following disclaimer in the documentation and/or other materials provided with
the distribution.THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.

*/

namespace rta {
	namespace pbrt {
		// pbrt
		inline void ConcentricSampleDisk(float u1, float u2, float *dx, float *dy) {
			float r, theta;
			// Map uniform random numbers to $[-1,1]^2$
			//     float sx = 2 * u1 - 1;
			//     float sy = 2 * u2 - 1;
			float sx = u1;
			float sy = u2;

			// Map square to $(r,\theta)$

			// Handle degeneracy at the origin
			if (sx == 0.0 && sy == 0.0) {
				*dx = 0.0;
				*dy = 0.0;
				return;
			}
			if (sx >= -sy) {
				if (sx > sy) {
					// Handle first region of disk
					r = sx;
					if (sy > 0.0) theta = sy/r;
					else          theta = 8.0f + sy/r;
				}
				else {
					// Handle second region of disk
					r = sy;
					theta = 2.0f - sx/r;
				}
			}
			else {
				if (sx <= sy) {
					// Handle third region of disk
					r = -sx;
					theta = 4.0f - sy/r;
				}
				else {
					// Handle fourth region of disk
					r = -sy;
					theta = 6.0f + sx/r;
				}
			}
			theta *= M_PI / 4.f;
			*dx = r * cosf(theta);
			*dy = r * sinf(theta);
		}

		// pbrt
		inline vec3_t CosineSampleHemisphere(float u1, float u2) {
			vec3_t ret;
			ConcentricSampleDisk(u1, u2, &ret.x, &ret.y);
			ret.z = sqrtf(std::max(0.f, 1.f - ret.x*ret.x - ret.y*ret.y));
			return ret;
		}

	}
}

#endif

