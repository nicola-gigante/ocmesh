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

#include "utils/meta.h"
#include "morton.h"

namespace ocmesh {
namespace details {

/*
 * Little utilities needed below
 */
template<typename T, REQUIRES(std::is_integral<T>())>
constexpr uint8_t clog2(T v, uint8_t log = 0) {
    return v == 1 ? log : clog2(v / 2, log + 1);
}
    
constexpr uint64_t lowmask(uint8_t bits) {
    return bits == 0 ? 0 : uint64_t(-1) >> (64 - bits);
}

constexpr uint64_t highmask(uint8_t bits) {
    return bits == 0 ? 0 : uint64_t(-1) << (64 - bits);
}
    
class voxel
{
    // The only piece of data inside a voxel is a single compact word.
    uint64_t _code = 0;
    
public:
    using level_t    = uint8_t;
    using material_t = uint32_t;
    
    static constexpr material_t unknown_material = 0;
    static constexpr material_t void_material = 1;
    
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
    voxel() = default;
    
    // Explicitly constructing a voxel from its encoding
    explicit voxel(uint64_t code) : _code(code) { }
    
    // Piecewise construction with encoded coordinates.
    voxel(uint64_t morton, level_t level, material_t material)
        : _code(pack(morton, level, material))
    {
        assert(level    <= max_level    && "Cubie level out of range");
        assert(material <= max_material && "Material index out of range");
    }
    
    // Piecewise construction with unpacked coordinates.
    voxel(glm::u16vec3 coordinates, level_t level = 0, uint32_t material = 0)
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
    level_t level() const {
        return level_t((_code >> material_bits) & lowmask(level_bits));
    }
    
    uint8_t height() const {
        return max_level - level();
    }
    
    material_t material() const {
        return material_t( _code & lowmask(material_bits) );
    }
    
    uint64_t morton() const {
        return (_code >> (material_bits + level_bits)) & lowmask(location_bits);
    }
    
    uint64_t code() const { return _code; }
    
    glm::u16vec3 coordinates() const {
        return glm::u16vec3(unmorton(morton()));
    }
    
    /*
     * The size of the voxel edge expressed in base units
     */
    uint16_t size() const {
        return 1 << height();
    }
    
    /*
     * These functions create a new voxel with a given field modified.
     */
    voxel with_level(level_t l) const {
        uint64_t mask = lowmask(material_bits) | highmask(location_bits);
        return voxel( (_code & mask) | (l << material_bits) );
    }
    
    voxel with_material(material_t mat) const {
        return voxel( (_code & highmask(location_bits + level_bits)) | mat );
    }
    
    voxel with_morton(uint64_t m) const {
        uint64_t mask = lowmask(material_bits + level_bits);
        
        return voxel( (_code & mask) | (m << (material_bits + level_bits)) );
    }
    
    voxel with_coordinates(glm::u16vec3 c) const {
        return with_morton(ocmesh::morton(glm::u32vec3(c)));
    }
    
    /*
     * Navigation functions
     */
    
    // Statically obtain a root voxel.
    static voxel root() {
        return { { 0, 0, 0 }, max_level, 0 };
    }
    
    /*
     * Enumeration of possible faces of the cube.
     */
    enum face : uint8_t
    {
        left,
        right,
        bottom,
        top,
        back,
        front
    };
    
    /*
     * Enumeration of possible corners of the cube. Corners are returned by
     * the corners() function in this order. Note that this corresponds to
     * navigating corners in Morton order again
     */
    enum corner : uint8_t
    {
        left_bottom_back,    // (0,0,0)
        right_bottom_back,   // (1,0,0)
        left_top_back,       // (0,1,0)
        right_top_back,      // (1,1,0)
        left_bottom_front,   // (0,0,1)
        right_bottom_front,  // (1,0,1)
        left_top_front,      // (0,1,1)
        right_top_front,     // (1,1,1)
        last_corner = right_top_front
    };
    
    
    /*
     * This function returns an array with the coordinates of the eight
     * corners of the voxel. The coordinates are still expressed in the
     * virtual, unsigned integer coordinate space of the voxel
     */
    template<typename Vec = glm::u16vec3>
    std::array<Vec, 8> corners() const;
    
    /*
     * Get the children of the voxel, in Morton order.
     * Note that the children inherit the material from the parent.
     * Note also that the morton code of the first child (Left/Bottom/Back),
     * is the same of the parent, the only difference being the level field.
     */
    std::array<voxel, 8> children() const;
    
    /*
     * Get a voxel represeting the neighbor of the voxel at the given direction,
     * and with the same size.
     *
     * Note that this voxel might not actually exist in the octree, but it's
     * needed to find the actual neighbor by searching for its immediate 
     * ancestor in the octree.
     */
    voxel neighbor(face d) const;
    
    /*
     * Get all neighbors of the voxel
     */
    std::array<voxel, 6> neighborhood() const;
    
private:
    static uint64_t pack(uint64_t morton, level_t level, material_t material)
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
    assert(height() > 0 && "Can't subdivide a leaf node");
    
    std::array<voxel, 8> results;
    
    // Increment the level
    uint64_t l = level() + 1;
    uint64_t h = height() - 1;

    // Increment for each child
    uint64_t inc = uint64_t(1) << (h * 3);
    
    uint64_t m = morton();
    for(voxel &v : results) {
        v = voxel(m, l, material());
        m += inc;
    }
    
    return results;
}

/*
 * corners() member function implementation. Note that the corners are produced
 * in morton order, which in turn is reflected also by the order of the
 * voxel::corner enumeration
 */
template<typename Vec>
inline
std::array<Vec, 8> voxel::corners() const
{
    Vec c = Vec(coordinates());
    uint16_t edge = size();
        
    return {
        c,
        c + Vec{ edge,    0,    0 },
        c + Vec{    0, edge,    0 },
        c + Vec{ edge, edge,    0 },
        c + Vec{    0,    0, edge },
        c + Vec{ edge,    0, edge },
        c + Vec{    0, edge, edge },
        c + Vec{ edge, edge, edge }
    };
}
    
// Check if we can add x and y without overflow
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
 * so that the neighbor in this particular direction would fall off the border.
 *
 * In this case the function returns a void voxel.
 */
inline
voxel voxel::neighbor(face d) const
{
    /*
     * This will maybe be the most frequently called function in the entire
     * application, so we have to increase a bit its complexity to avoid
     * branches. Everything is computed in one linear flow control.
     */
    // First we choose if we need to add the size or subtract 1
    bool add_size = d == right  ||
                    d == top    ||
                    d == front;
    
    // Select the index of the coordinate to change.
    // Note: these ternary operators are probably not compiled as branches
    // TODO: Check the above statement
    uint8_t index = d == left || d == right  ? 0 :
                    d == top  || d == bottom ? 1 : 2 ;
    
    // Get the coordinates of the voxel
    glm::u16vec3 coordinates = this->coordinates();
    
    // Check if we have a neighbor at all
    uint16_t has_neighbor =
        (add_size && add_is_safe(coordinates[index], size())) ||
        (!add_size && coordinates[index] > 0);
    
    // We add the size if add_size is true,
    // or we add -1 if add_size is false,
    // but only if the neighbor actually exists
    coordinates[index] += has_neighbor * (add_size * size() - (!add_size));

    // Build the new voxel. If has_neighbor is false we obtain a void voxel.
    return voxel(has_neighbor * coordinates,
                 has_neighbor * max_level,
                 has_neighbor * material());
}
    
inline
std::array<voxel, 6> voxel::neighborhood() const {
    return {
        neighbor(left  ),
        neighbor(right ),
        neighbor(bottom),
        neighbor(top   ),
        neighbor(back  ),
        neighbor(front )
    };
}

/*
 * Miscellaneous operators for convenient use of voxels.
 */
inline
std::ostream &operator<<(std::ostream &s, voxel v) {
    s << "{ " << v.coordinates().x << ", "
              << v.coordinates().y << ", "
              << v.coordinates().z << " }"
      << " - level: "    << unsigned(v.level())
      << " - size: "     << v.size()
      << " - material: " << v.material();
    return s;
}

inline
bool operator==(voxel v1, voxel v2) {
    return v1.code() == v2.code();
}

inline
bool operator!=(voxel v1, voxel v2) {
    return v1.code() != v2.code();
}

inline
bool operator<(voxel v1, voxel v2) {
    return v1.code() < v2.code();
}

inline
bool operator>(voxel v1, voxel v2) {
    return v1.code() > v2.code();
}

inline
bool operator<=(voxel v1, voxel v2) {
    return v1.code() <= v2.code();
}

inline
bool operator>=(voxel v1, voxel v2) {
    return v1.code() >= v2.code();
}
    
} // namespace details
    
using details::voxel;
    
} // namespace ocmesh

#endif
