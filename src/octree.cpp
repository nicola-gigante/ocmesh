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
    octree::neighbor(const_iterator node, voxel::face d)
    {
        voxel candidate = node->neighbor(d);
        
        return std::lower_bound(begin(), end(), candidate);
    }
    
    octree::const_iterator
    octree::neighbor(const_iterator node, voxel::face d) const
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
                _data.insert(_data.end(), children.begin() + 1, children.end());
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
    
    
    /*
     * Function object for the subdivision of the octree from the CSG scene.
     */
    class scene_builder
    {
    public:
        scene_builder(csg::scene const&scene, float precision)
            : _scene(scene), _bounding_box(_scene.bounding_box()),
              _precision(precision) { }
        
        
        voxel::material_t operator()(voxel v) const {
            for(auto *obj : _scene) {
                intersection_result r = intersection(obj, v);
                if(r == inside)
                    return obj->material();
                if(r == at_intersection)
                    return voxel::unknown_material;
            }
            
            return voxel::void_material;
        }
        
    private:
        enum intersection_result {
            inside,
            outside,
            at_intersection
        };
        
        /*
         * The core function of the subdivision procedure is here. It decides
         * if a voxel intersects a given CSG object or not.
         */
        intersection_result intersection(csg::object *obj, voxel v) const
        {
            glm::vec3 coordinates = glm::vec3(v.coordinates());
            float side = v.size();
            
            // Scale the voxel to the scene bounding box
            float scale = _bounding_box.side() / voxel::max_coordinate;
            coordinates = coordinates * scale + _bounding_box.min();
            side *= scale;
            
            glm::vec3 center = coordinates + glm::vec3{ side / 2, side / 2, side / 2 };
            float diagonal = std::sqrt(3) * side;
            
            float d = obj->distance(center);
            if(std::abs(d) < diagonal / 2 &&
               side >= _bounding_box.side() * _precision)
            {
                return at_intersection;
            }
            
            return d > 0 ? outside : inside;
        }
        
    private:
        csg::scene const&_scene;
        csg::bounding_box _bounding_box;
        float _precision;
    };
    
    void octree::build(csg::scene const&scene, float precision) {
        build(scene_builder(scene, precision));
    }
    
} // namespace details
} // namespace ocmesh
