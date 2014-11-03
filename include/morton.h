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
        
        inline constexpr uint64_t cpow2(uint8_t v) {
            return v == 0 ? 1 : 2 * cpow2(v - 1);
        }
        
        class voxel
        {
        public:
            static constexpr size_t precision     = 13;
            static constexpr size_t location_bits = precision * 3;
            static constexpr size_t level_bits    = clog2(precision) + 1;
            static constexpr size_t material_bits = 64 - location_bits - level_bits;
            
            static constexpr size_t max_level    = precision;
            static constexpr size_t max_material = cpow2(material_bits) - 1;
            
        public:
            voxel(glm::u16vec3 coordinates, uint8_t level, uint32_t material)
                : _location(morton(coordinates)),
                  _level(level), _material(material)
            {
                assert(level    < max_level    && "Cubie level out of range");
                assert(material < max_material && "Material index out of range");
            }
            
            voxel           (voxel const&) = default;
            voxel &operator=(voxel const&) = default;
            
            uint8_t  level()    const { return _level;    }
            uint32_t material() const { return _material; }
            uint64_t location() const { return _location; }
            uint64_t code()     const { return _code;     }
            
            glm::u16vec3 coordinates() const { assert("Unimplemented"); return {}; }
            
        private:
            
            union {
                uint64_t _code;
                struct {
                    uint64_t _material : material_bits;
                    uint64_t _level    : level_bits;
                    uint64_t _location : location_bits;
                };
            };
            
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
            
            uint64_t morton(glm::u16vec3 coordinates)
            {
                static constexpr auto xmask = tbl::map<0, 256>(split);
                static constexpr auto ymask = tbl::map(shiftl<1>, xmask);
                static constexpr auto zmask = tbl::map(shiftl<2>, xmask);
                
                // All this union/bitfield stuff is far more clear than
                // a bunch of shift/masks etc...
                struct {
                    int low;
                    int high;
                } x = { coordinates.x & 0xFF, (coordinates.x >> 8) & 0xFF },
                  y = { coordinates.y & 0xFF, (coordinates.y >> 8) & 0xFF },
                  z = { coordinates.z & 0xFF, (coordinates.z >> 8) & 0xFF };
                
                uint64_t answer =
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
    }
    
    using details::voxel;
}


#endif
