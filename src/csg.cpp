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

#include "csg.h"

#include <algorithm>
#include <cmath>

namespace ocmesh {
    
    namespace details {
        
        object::~object() = default;
        binary_operation_t::~binary_operation_t() = default;
        
        float sphere_t::distance(glm::vec3 const&from) {
            return glm::length(from) - _radius;
        }
        
        float cube_t::distance(glm::vec3 const&from) {
            return std::max({std::abs(from.x),
                             std::abs(from.y),
                             std::abs(from.z)}) - _side / 2;
        }
        
        float transform_t::distance(glm::vec3 const&from)
        {
            glm::vec4 v = { from.x, from.y, from.z, 1.0f };
            
            return child()->distance((transform() * v).xyz());
        }
        
        float union_t::distance(const glm::vec3 &from) {
            return std::min(left()->distance(from), right()->distance(from));
        }
        
        float intersection_t::distance(const glm::vec3 &from) {
            return std::max(left()->distance(from), - right()->distance(from));
        }
        
        float difference_t::distance(const glm::vec3 &from) {
            return std::max(left()->distance(from), right()->distance(from));
        }
    }
}
