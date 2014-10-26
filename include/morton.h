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

#include <cstdint>
#include <glm/glm.hpp>

#include "static_table.h"

namespace ocmesh {

    /*
     * split()
     *
     * Compile-time precomputation of bits interleaving table for morton 
     * encoding construction.
     * C++11 constexpr functions must be written in purely functional style, so
     * what follows is the constexpr-enabled version of this code:
     *
     *    inline uint32_t split(uint32_t x) {
     *       x = (x | x << 16) & 0xff0000ff;
     *       x = (x | x <<  8) & 0x0f00f00f;
     *       x = (x | x <<  4) & 0xc30c30c3;
     *       x = (x | x <<  2) & 0x49249249;
     *       return x;
     *    }
     */
    namespace details {
        constexpr uint32_t masks[] = {
            0,
            0x49249249, 0xc30c30c3,
            0x0f00f00f, 0xff0000ff
        };
        
        constexpr uint32_t split(uint32_t x, int level) {
            return level == 0 ? x :
                    split((x | x << (1 << level)) & masks[level], level - 1);
        }
        
        constexpr uint32_t split(uint32_t x) {
            return details::split(x, 4);
        }
        
        template<uint8_t S>
        constexpr uint32_t shiftl(uint32_t x) {
            static_assert(S < 32, "Please don't invoke undefined behaviour");
            return x << S;
        }
        
        inline uint64_t morton(uint32_t x, uint32_t y, uint32_t z)
        {
            static constexpr auto xmask =
                tbl::map(split, tbl::irange<uint32_t, 0, 256>());
            static constexpr auto ymask = tbl::map(shiftl<1>, xmask);
            static constexpr auto zmask = tbl::map(shiftl<2>, xmask);
            
            // TODO...
            
            return xmask[0] + ymask[0] + zmask[0];
        }
    }
}


#endif
