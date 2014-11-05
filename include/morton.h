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

#include <cassert>
#include <cstdint>
#include <cmath>
#include <glm/glm.hpp>

#include "static_table.h"

namespace ocmesh {
namespace details {


template<typename T, REQUIRES(std::is_integral<T>::value)>
constexpr uint8_t clog2(T v, uint8_t log = 0) {
    return v == 1 ? log : clog2(v / 2, log + 1);
}

class voxel
{
public:
    static constexpr size_t precision     = 13;
    static constexpr size_t location_bits = precision * 3;
    static constexpr size_t level_bits    = clog2(precision) + 1;
    static constexpr size_t material_bits = 64 - location_bits - level_bits;
    
    static constexpr size_t max_coordinate = (1 << precision) - 1;
    static constexpr size_t max_level      = precision;
    static constexpr size_t max_material   = (1 << material_bits) - 1;
    
public:
    /*
     * Constructors
     */
    
    // The default constructor creates a void voxel, i.e. a voxels that lives in
    // void space. Void voxels are not valid are removed by the tree or ignored.
    voxel() = default;
    
    // Explicitly constructing a voxel from its encoding
    explicit voxel(uint64_t code) : _code(code) { }
    
    // Piecewise construction with encoded coordinates.
    voxel(uint64_t morton, uint8_t level, uint32_t material)
        : _code(pack(morton, level, material))
    {
        assert(level    < max_level    && "Cubie level out of range");
        assert(material < max_material && "Material index out of range");
    }
    
    // Piecewise construction with unpacked coordinates.
    voxel(glm::u16vec3 coordinates, uint8_t level, uint32_t material)
        : voxel(morton(coordinates), level, material)
    {
        assert(coordinates.x < max_coordinate && "X coordinate out of range");
        assert(coordinates.y < max_coordinate && "Y coordinate out of range");
        assert(coordinates.z < max_coordinate && "Z coordinate out of range");
    }
    
    // Copy and assignment is trivial
    voxel           (voxel const&) = default;
    voxel &operator=(voxel const&) = default;
    
    /*
     *
     */
    uint8_t level() const {
        return uint8_t((_code >> material_bits) & mask(level_bits));
    }
    
    uint32_t material() const {
        return uint32_t( _code & mask(material_bits) );
    }
    
    uint64_t morton() const {
        return _code >> (material_bits + level_bits) & mask(location_bits);
    }
    
    uint64_t code() const { return _code; }
    
    glm::u16vec3 coordinates() const { assert("Unimplemented"); return {}; }
    
    /*
     * Navigation functions
     */
    
    // Statically obtain a root voxel.
    static voxel root() {
        return { { 0, 0, 0 }, max_level, 0 };
    }
    
    // Get the children of the voxel, in Morton order.
    // Note that the children inherit the material from the parent.
    std::array<voxel, 8> children() const
    {
        assert(level() > 0 && "Can't subdivide a zero-level node");
        
        std::array<voxel, 8> results;
        
        // Decrement the level
        uint64_t l = level() - 1;
        // To get all the children is sufficient to increment the octal
        // digit that corresponds to their level. In a well formed location
        // code, the "don't care" digits are zero, so we can simply increment
        // from the current value, without erasing anything first.
        // Note also that the morton code of the first child (Front/Up/Left),
        // is the same of the parent, the only difference being the level
        // field.
        uint64_t inc = 1 << (l * 3);

        uint64_t m = morton();
        for(voxel &v : results) {
            v = voxel(m, l, material());
            m += inc;
        }
        
        return results;
    }
    
private:
    uint64_t _code = 0;
    
    static constexpr uint64_t mask(uint8_t bits) {
        return bits == 0 ? 0 : uint64_t(-1) >> (64 - bits);
    }
    
    static uint64_t pack(uint64_t morton,
                         uint8_t level, uint32_t material)
    {
        return morton << (material_bits + level_bits) |
               level  <<  material_bits               |
               material;
    }
    
    /*
     * Morton encoding calculation
     *
     * The morton code is obtained by interleaving the bits of the
     * x y z coordinates. This can be done with a constant number of
     * magic bitwise operations. The fastest implementation available is
     * based on the pre-computation of a table of bitmasks for 8 bit
     * values, that are composed to interleave our 16bits words.
     *
     * Thanks to the C++11 constexpr keyword, we can tell the compiler
     * to precompute the table and statically put the results directly
     * as data into the executable.
     *
     * The split() function is what computes the single elements
     * of the table. The code in the morton() function then uses the
     * facilities from static_table.h to fill the tables.
     *
     * C++11 constexpr functions must be written in purely functional
     * style, so the implementation of split() below is the
     * constexpr-enabled version of this code, which would be usable
     * as is with C++14 constexpr:
     *
     *    constexpr uint32_t split(uint32_t x) {
     *       x = (x | x << 16) & 0xff0000ff;
     *       x = (x | x <<  8) & 0x0f00f00f;
     *       x = (x | x <<  4) & 0xc30c30c3;
     *       x = (x | x <<  2) & 0x49249249;
     *       return x;
     *    }
     *
     */
    static constexpr uint32_t masks[] = {
        0,
        0x49249249, 0xc30c30c3,
        0x0f00f00f, 0xff0000ff
    };
    
    static constexpr uint32_t split(uint32_t x, int level) {
        return level == 0 ? x :
               split((x | x << (1 << level)) & masks[level], level - 1);
    }
    
    static constexpr uint32_t split(uint32_t x) {
        return split(x, 4);
    }
    
    template<uint8_t S>
    static constexpr uint32_t shiftl(uint32_t x) {
        return x << S;
    }
    
    static uint64_t morton(glm::u16vec3 coordinates)
    {
        static constexpr auto xmask = tbl::map<0, 256>(split);
        static constexpr auto ymask = tbl::map(shiftl<1>, xmask);
        static constexpr auto zmask = tbl::map(shiftl<2>, xmask);
        
        struct unpack {
            int low;
            int high;
            
            unpack(uint16_t v) : low(v & 0xFF), high(v >> 8 & 0xFF) { }
        } x = coordinates.x,
          y = coordinates.y,
          z = coordinates.z;
        
        uint64_t   answer =
                 ((zmask[z.high] | ymask[y.high] | xmask[x.high]) << 24) |
                  (zmask[z.low]  | ymask[y.low]  | xmask[x.low]);
        
        return answer;
    }
};

bool operator==(voxel const&v1, voxel const&v2) {
    return v1.code() == v2.code();
}

bool operator!=(voxel const&v1, voxel const&v2) {
    return v1.code() != v2.code();
}

bool operator<(voxel const&v1, voxel const&v2) {
    return v1.code() < v2.code();
}

bool operator>(voxel const&v1, voxel const&v2) {
    return v1.code() > v2.code();
}

bool operator<=(voxel const&v1, voxel const&v2) {
    return v1.code() <= v2.code();
}

bool operator>=(voxel const&v1, voxel const&v2) {
    return v1.code() >= v2.code();
}

static_assert(sizeof(voxel) == sizeof(uint64_t),
              "Something wrong with the voxel bitfield. "
              "Check the precision.");

} // namespace details

using details::voxel;

} // namespace ocmesh


#endif
