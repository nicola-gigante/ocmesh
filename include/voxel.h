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

#ifndef OCMESH_VOXEL_H__
#define OCMESH_VOXEL_H__

#include <array>
#include <cmath>
#include <ostream>

#include "morton.h"

namespace ocmesh {
namespace details {

/*
 * Little utilities needed below
 */
template<typename T, REQUIRES(std::is_integral<T>::value)>
constexpr uint8_t clog2(T v, uint8_t log = 0) {
    return v == 1 ? log : clog2(v / 2, log + 1);
}
    
constexpr uint64_t mask(uint8_t bits) {
    return bits == 0 ? 0 : uint64_t(-1) >> (64 - bits);
}
    
class voxel
{
    // The only piece of data inside a voxel is a single compact word.
    uint64_t _code = 0;
    
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
        assert(level    <= max_level    && "Cubie level out of range");
        assert(material <= max_material && "Material index out of range");
    }
    
    // Piecewise construction with unpacked coordinates.
    voxel(glm::u16vec3 coordinates, uint8_t level, uint32_t material)
    : voxel(details::morton(glm::u32vec3(coordinates)), level, material)
    {
        assert(coordinates.x <= max_coordinate && "X coordinate out of range");
        assert(coordinates.y <= max_coordinate && "Y coordinate out of range");
        assert(coordinates.z <= max_coordinate && "Z coordinate out of range");
    }
    
    // Copy and assignment is trivial
    voxel           (voxel const&) = default;
    voxel &operator=(voxel const&) = default;
    
    /*
     * Functions to obtain the various components of voxel data
     */
    uint8_t level() const {
        return uint8_t((_code >> material_bits) & mask(level_bits));
    }
    
    uint8_t height() const {
        return max_level - level();
    }
    
    uint32_t material() const {
        return uint32_t( _code & mask(material_bits) );
    }
    
    uint64_t morton() const {
        return (_code >> (material_bits + level_bits)) & mask(location_bits);
    }
    
    uint64_t code() const { return _code; }
    
    glm::u16vec3 coordinates() const {
        return glm::u16vec3(unmorton(morton()));
    }
    
    // The size of the voxel edge expressed in base units
    uint16_t size() const {
        return 1 << height();
    }
    
    /*
     * Navigation functions
     */
    
    // Statically obtain a root voxel.
    static voxel root() {
        return { { 0, 0, 0 }, max_level, 0 };
    }
    
    /*
     * Get the children of the voxel, in Morton order.
     * Note that the children inherit the material from the parent.
     * Note also that the morton code of the first child (Left/Bottom/Back),
     * is the same of the parent, the only difference being the level field.
     */
    std::array<voxel, 8> children() const;
    
    /*
     * Enumeration of possible directions of the neighbors of a voxel.
     */
    enum direction : uint8_t
    {
        left,
        right,
        down,
        up,
        back,
        front
    };
    
    /*
     * Get a voxel represeting the neighbor of the voxel at the given direction,
     * and with the same size.
     *
     * Note that this voxel might not actually exist in the octree, but it's
     * needed to find the actual neighbor by searching for its immediate 
     * ancestor in the octree.
     */
    voxel neighbor(direction d) const;
    
    /*
     * Get all neighbors of the voxel
     */
    std::array<voxel, 6> neighborhood() const;
    
private:
    static uint64_t pack(uint64_t morton, uint8_t level, uint32_t material)
    {
        return morton << (material_bits + level_bits) |
               level  <<  material_bits               |
               material;
    }
};

/*
 *
 * children() member function implementation.
 *
 * To get all the children is sufficient to increment the octal
 * digit that corresponds to their level. In a well formed location
 * code, the "don't care" digits are zero, so we can simply increment
 * from the current value, without erasing anything first.
 *
 */
inline
std::array<voxel, 8> voxel::children() const
{
    assert(level() > 0 && "Can't subdivide a zero-level node");
    
    std::array<voxel, 8> results;
    
    // Decrement the level
    uint64_t l = level() - 1;

    // Increment for each child
    uint64_t inc = 1 << (l * 3);
    
    uint64_t m = morton();
    for(voxel &v : results) {
        v = voxel(m, l, material());
        m += inc;
    }
    
    return results;
}

constexpr bool add_is_safe(uint16_t x, uint16_t y) {
    return x <= std::numeric_limits<uint16_t>::max() - y;
}

/*
 * neighbor() member function implementation.
 *
 * Depending on the direction we have to do different things.
 *
 * The coordinate of the voxel represents the Left/Bottom/Back corner of the
 * cubie. This means that to go in the left, down and back directions we only
 * need to subtract one from the corresponding coordinate to reach a point
 * that for sure belongs to the neighbor.
 *
 * Instead, for the right, up and front direction we have to jump over the
 * entire voxel, so we add the size of the voxel itself.
 *
 * All this have to take into account the case when the needed neighbor does
 * not exist at all. This case manifests itself as a possible overflow or
 * underflow in the operations outlined above, and it happens when we're trying
 * to find the neighbor of a voxel that is at the extreme edge of the space,
 * so that the neighbor in this particular direction would fall of the border.
 *
 * In this case the function returns a void voxel.
 */
inline
voxel voxel::neighbor(direction d) const
{
    bool add_size = d == right ||
                    d == up    ||
                    d == front;
    
    uint8_t index = d == left || d == right ? 0 :
                    d == up   || d == down  ? 1 : 2 ;
    
    glm::u16vec3 coordinates = this->coordinates();
    
    // We add the size if add_size is true,
    // or we add -1 if add_size is false
    if(add_size && add_is_safe(coordinates[index], size()))
        coordinates[index] += size();
    else if(!add_size && coordinates[index] > 0)
        coordinates[index] -= 1;
    else
        return voxel(); // Neighbor does not exist
    
    return voxel(coordinates, level(), material());
}
    
inline
std::array<voxel, 6> voxel::neighborhood() const {
    return {
        neighbor(left ),
        neighbor(right),
        neighbor(down ),
        neighbor(up   ),
        neighbor(back ),
        neighbor(front),
    };
}

/*
 * Miscellaneous operators for convenient use of voxels.
 */
std::ostream &operator<<(std::ostream &s, voxel v) {
    s << "{ " << v.coordinates().x << ", "
              << v.coordinates().y << ", "
              << v.coordinates().z << " }"
      << " - level: "    << unsigned(v.level())
      << " - size: "     << v.size()
      << " - material: " << v.material();
    return s;
}
    
bool operator==(voxel v1, voxel v2) {
    return v1.code() == v2.code();
}

bool operator!=(voxel v1, voxel v2) {
    return v1.code() != v2.code();
}

bool operator<(voxel v1, voxel v2) {
    return v1.code() < v2.code();
}

bool operator>(voxel v1, voxel v2) {
    return v1.code() > v2.code();
}

bool operator<=(voxel v1, voxel v2) {
    return v1.code() <= v2.code();
}

bool operator>=(voxel v1, voxel v2) {
    return v1.code() >= v2.code();
}
    
} // namespace details
    
using details::voxel;
    
} // namespace ocmesh

#endif
