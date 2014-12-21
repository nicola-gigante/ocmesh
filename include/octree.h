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

#ifndef OCMESH_OCTREE_H__
#define OCMESH_OCTREE_H__

#include "voxel.h"

#include <deque>

namespace ocmesh {
namespace details {

class octree
{
    using container_t = std::deque<voxel>;
    
public:
    using value_type             = voxel;
    using pointer                = container_t::pointer;
    using const_pointer          = container_t::const_pointer;
    using reference              = container_t::reference;
    using const_reference        = container_t::const_reference;
    using iterator               = container_t::iterator;
    using const_iterator         = container_t::const_iterator;
    using reverse_iterator       = container_t::reverse_iterator;
    using const_reverse_iterator = container_t::const_reverse_iterator;
    using difference_type        = container_t::difference_type;
    
public:
    octree() = default;
    octree(octree const&) = default;
    octree(octree     &&) = default;
    
    octree &operator=(octree const&) = default;
    octree &operator=(octree     &&) = default;
    
    /*
     * We expose the voxels in their natural order
     */
    iterator        begin()       { return _data.begin();  }
    const_iterator  begin() const { return _data.begin();  }
    const_iterator cbegin() const { return _data.cbegin(); }

    iterator        end()         { return _data.end();    }
    const_iterator  end()   const { return _data.end();    }
    const_iterator cend()   const { return _data.cend();   }
        
    // Find neighbor of a node in a given direction
    const_iterator neighbor(iterator node, voxel::direction d) const;
    
private:
    container_t _data = { voxel{} };
};
    

} // namespace details

using details::octree;
    
} // namespace ocmesh

#endif

