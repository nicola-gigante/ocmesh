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

#include "octree.h"

#include <algorithm>

namespace ocmesh {
namespace details {
    
    octree::iterator
    octree::neighbor(const_iterator node, voxel::direction d)
    {
        voxel candidate = node->neighbor(d);
        
        return std::lower_bound(begin(), end(), candidate);
    }
    
    octree::const_iterator
    octree::neighbor(const_iterator node, voxel::direction d) const
    {
        voxel candidate = node->neighbor(d);
        
        return std::lower_bound(begin(), end(), candidate);
    }
    
    // TODO: handle the case when the subdivision reaches the final level
    //       but the split function still has not decided the material
    void octree::build(std::function<voxel::material_t(voxel)> split_function)
    {
        _data.push_back(voxel{});
        
        for(size_t i = 0; i < _data.size(); ++i)
        {
            voxel v = _data[i];
            uint8_t level = v.level();
            
            voxel::material_t material = split_function(v);
            
            if(level < voxel::max_level &&
               material == voxel::unknown_material)
            {
                auto children = v.children();
                _data[i] = children[0];
                _data.insert(_data.end(),
                             children.begin() + 1, children.end());
                --i; // Decrement i to continue from this same voxel
            } else {
                _data[i] = _data[i].with_material(material);
            }
        }

        assert(std::none_of(_data.begin(), _data.end(), [](voxel v) {
            return v.material() == voxel::unknown_material;
        }));
        
        std::sort(_data.begin(), _data.end());
    }
} // namespace details
} // namespace ocmesh
