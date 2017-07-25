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

#ifndef OCMESH_OCTREE_H
#define OCMESH_OCTREE_H

#include "csg.h"
#include "voxel.h"
#include "glm.h"

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
    
    /*
     * Returns the number of voxels currently represented in the octree
     */
    size_t size() const { return _data.size();  }
    
    /*
     * Returns true if the octree is empty
     */
    bool empty() const { return _data.empty(); }
    
    /*
     * Finds neighbor of a node corresponding to the given face.
     * A second face can be specified, to find 
     * "the neighbor at face f2 of the neighbor at face f1" of this voxel, i.e.
     * edge neighbors of this voxel.
     */
    iterator       neighbor(const_iterator node, voxel::face f);
    const_iterator neighbor(const_iterator node, voxel::face f) const;

    iterator       neighbor(const_iterator node,
                            voxel::face f1, voxel::face f2);
    const_iterator neighbor(const_iterator node,
                            voxel::face f1, voxel::face f2) const;
    
    /*
     * This is the main function to recursively build an octree with a given
     * splitting rule. The octree is built by applying the predicate
     * to each voxel to determine if it has to be split.
     *
     * The argument must be a function that given a voxel will return its
     * material. If the function returns voxel::unknown_material, the voxel
     * will be split and its children recursively examined.
     *
     * Otherwise, if the function returns any material value, then the material
     * will be set to the voxel, which will be definitely added to the octree.
     *
     * Note: during the execution of this function, the octree is in an
     * inconsistent state. This means that the testing function should only
     * use the given voxel and avoid to use the octree in any other way.
     */
    using split_function_t = std::function<voxel::material_t(voxel)>;
    void build(split_function_t split_function);
    
    /*
     * Version to directly build the octree from a CSG scene.
     * The precision is a percentage of the size of the scene's bounding box
     */
    void build(csg::scene const&scene, float precision);
    
    /*
     * Different mesh formats supported by the mesh() function
     */
    enum mesh_t {
        obj
    };
    
    /*
     * Export function, to dump the octree to a mesh file.
     */
    void mesh(mesh_t mesh_type, std::ostream &outs) const;
    
private:
    container_t  _data;
    glm::f32mat4 _transform; // default-constructed as the identity matrix
};

    

} // namespace details

using details::octree;
    
} // namespace ocmesh

#endif

