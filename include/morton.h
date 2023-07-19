// -*- C++ -*-
/*
 * Copyright 2014 Nicola Gigante
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OCMESH_MORTON_H
#define OCMESH_MORTON_H

#include "glm.h"

#if __GNUC__  < 5 && !__clang__

#include <algorithm>

namespace std
{
	template <std::size_t Len, class... Types>
	struct aligned_union
	{
		static constexpr std::size_t alignment_value = std::max({alignof(Types)...});

		struct type
		{
			alignas(alignment_value) char _s[std::max({Len, sizeof(Types)...})];
		};
	};
}
#endif

#include <utility>
#include <array>

#include <cassert>
#include <cstdint>
#include <cmath>

namespace ocmesh {
namespace details {
   
    /*
     * Morton encoding calculation
     *
     * Voxels in the linear octree are stored in a specific order, that is
     * equivalent to a pre-order traversal of the tree, which spatially
     * corresponds to a space-filling path that is often called Z-order or,
     * recursively, Morton order.
     *
     * The morton code of a 3D coordinate vector is obtained by interleaving
     * the bits of the coordinates, e.g. if the coordinates are:
     *
     * x = xxxx, y = yyyy, z = zzzz
     *
     * the Morton code is zyxzyxzyx
     *
     * Note that into a 64bit word, one can encode a 3D vector of 21 bits for
     * each component. In the voxel class in voxel.h, coordinate components
     * have a smaller width, for other reasons, but the code in this section
     * is general and these functions are independent of the choices
     * made in voxel.h, so they pack and unpack all the 21 bits of each 
     * coordinate.
     *
     * Note also that the relative order of each coordinate in the interleave is
     * arbitrary, and corresponds to the spatial order that we want to follow.
     *
     * The order matters, as it has effects on the optimal order of traversal of
     * the octree.
     *
     * This order is hardcoded once and for all in the 
     * following enum declaration:
     */
    enum class coordinate_t : uint8_t {
        x = 0,
        y = 1,
        z = 2
    };
    
    
    /*
     * This function applies the interleaving.
     *
     * To understand the magic bit hackery here, try to apply the operations
     * by hand and the mechanism should become clear.
     */
    inline
    uint64_t morton(uint32_t value, coordinate_t coord)
    {
        uint64_t x = value;
        
        x = (x | (x << 32)) & 0xFFFF00000000FFFF;
        x = (x | (x << 16)) & 0x00FF0000FF0000FF;
        x = (x | (x <<  8)) & 0xF00F00F00F00F00F;
        x = (x | (x <<  4)) & 0x30C30C30C30C30C3;
        x = (x | (x <<  2)) & 0x9249249249249249;
        
        x = x << uint8_t(coord);
        
        return x;
    }
    
    /*
     * Unpacks one component of a morton encoded 3D vector.
     */
    inline
    uint32_t unmorton(uint64_t x, coordinate_t coord)
    {
        x = x >> uint8_t(coord); // Shift to select the component
        
        x =  x              & 0x9249249249249249;
        x = (x | (x >>  2)) & 0x30C30C30C30C30C3;
        x = (x | (x >>  4)) & 0xF00F00F00F00F00F;
        x = (x | (x >>  8)) & 0x00FF0000FF0000FF;
        x = (x | (x >> 16)) & 0xFFFF00000000FFFF;
        x = (x | (x >> 32)) & 0x00000000FFFFFFFF;
        
        return uint32_t(x); // The result surely fits into 32 bits.
    }
    
    
    /*
     * This function interleaves all the three components and packs them 
     * together.
     */
    inline
    uint64_t morton(glm::u32vec3 coordinates)
    {
        return morton(coordinates.x, coordinate_t::x) |
               morton(coordinates.y, coordinate_t::y) |
               morton(coordinates.z, coordinate_t::z);
    }
    
    /*
     * Unpacks a morton encoded 3D coordinate vector
     */
    inline
    glm::u32vec3 unmorton(uint64_t m)
    {
        uint32_t x = unmorton(m, coordinate_t::x),
                 y = unmorton(m, coordinate_t::y),
                 z = unmorton(m, coordinate_t::z);
        
        return { x, y, z };
    }

} // namespace details

using details::morton;
using details::unmorton;
    
} // namespace ocmesh


#endif
