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
#include <glm/glm.hpp>

#include "static_table.h"

namespace ocmesh {

    
    namespace details {
        class voxel {
            union {
                uint64_t _code;
                struct {
                    uint64_t _material : 21;
                    uint64_t _level    :  4;
                    uint64_t _location : 39;
                };
            };
            
        public:
            voxel(glm::u16vec3 coordinates, uint8_t level, uint32_t material)
                : _location(morton(coordinates)),
                  _level(level), _material(material) { }
            
            voxel           (voxel const&) = default;
            voxel &operator=(voxel const&) = default;
            
            uint8_t  level()    const { return _level; }
            uint32_t material() const { return _material; }
            uint64_t code()     const { return _code; }
            
            glm::u16vec3 coordinates() const { assert("Unimplemented"); return {}; }
            
        private:
            
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
             * constexpr-enabled version of this code:
             *
             *    inline uint32_t split(uint32_t x) {
             *       x = (x | x << 16) & 0xff0000ff;
             *       x = (x | x <<  8) & 0x0f00f00f;
             *       x = (x | x <<  4) & 0xc30c30c3;
             *       x = (x | x <<  2) & 0x49249249;
             *       return x;
             *    }
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
                static constexpr
                auto xmask = tbl::map(split, tbl::irange<uint32_t, 0, 256>());
                static constexpr auto ymask = tbl::map(shiftl<1>, xmask);
                static constexpr auto zmask = tbl::map(shiftl<2>, xmask);
                
                union {
                    uint16_t value;
                    struct {
                        uint8_t low;
                        uint8_t high;
                    };
                } x = { coordinates.x },
                  y = { coordinates.y },
                  z = { coordinates.z };
                
                union {
                    struct {
                        uint64_t low  : 24;
                        uint64_t high : 60;
                    };
                    uint64_t value;
                } answer = {
                    zmask[z.low]  | ymask[y.low]  | xmask[x.low],
                    zmask[z.high] | ymask[y.high] | xmask[x.high]
                };
                
                return answer.value;
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
                      "Something wrong in the bitfield");
    }
}


#endif
