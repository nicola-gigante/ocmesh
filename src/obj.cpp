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
#include "glm.h"

#include "utils/map_iterator.h"

#include <vector>
#include <tuple>
#include <iterator>

namespace ocmesh {
namespace details {
    
    class obj
    {
    public:
        void cube(voxel v) {
            std::array<glm::u16vec3, 8> c = v.corners();
            
            _faces.push_back(_vertices.size());
            
            auto cast = [](glm::u16vec3 v) {
                return glm::f32vec3(v);
            };
            
            std::copy(utils::make_map_iterator(c.begin(), cast),
                      utils::make_map_iterator(c.end(), cast),
                      std::back_inserter(_vertices));
        }
        
    private:
        static std::array<glm::f32vec3, 6> normals() {
            // FIXME: declare these two arrays as constexpr when GLM will
            // support it.
            std::array<glm::f32vec3, 6> results;
            
            results[voxel::left ] = { -1,  0,  0 };
            results[voxel::right] = {  1,  0,  0 };
            results[voxel::down ] = {  0, -1,  0 };
            results[voxel::up   ] = {  0,  1,  0 };
            results[voxel::back ] = {  0,  0, -1 };
            results[voxel::front] = {  0,  0,  1 };
            
            return results;
        }
        
    private:
        struct face_point {
            size_t vertex;
            size_t normal;
        };
        
        using face = std::array<face_point, 3>;
        
    private:
        std::vector<glm::f32vec3> _vertices;
        std::vector<size_t> _faces;
        
    };
    
    void obj_mesh(octree const&oc, std::ostream &out)
    {
        
    }
    
    void octree::mesh(mesh_t mesh_type, std::ostream &out) const {
        switch (mesh_type) {
            case obj:
                obj_mesh(*this, out);
                return;
        }
        assert(!"Unimplemented mesh type");
    }
    
} // namespace details
} // namespace ocmesh
