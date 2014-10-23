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

#ifndef OCMESH_CSG_H__
#define OCMESH_CSG_H__

#include <glm/glm.hpp>

#include <std14/memory>

namespace ocmesh {

    namespace details {
        /*
         * User facing primitives and operations.
         */
        class CSG {
        public:
            virtual ~CSG() = default;
        };
        
        using csg_ptr = std::unique_ptr<CSG>;
        
        class sphere_t : public CSG
        {
            float _radius;
            
        public:
            sphere_t(float radius) : _radius(radius) { }
            
            float radius() const { return _radius; }
        };
        
        class plane_t : public CSG
        {
            glm::vec4 _normal;
            
        public:
            plane_t(glm::vec4 const&normal) : _normal(normal) { }
            
            glm::vec4 normal() const { return _normal; }
        };
        
        class transform_t : public CSG
        {
            csg_ptr   _child;
            glm::mat4 _matrix;
            
        public:
            transform_t(csg_ptr child, glm::mat4 const&matrix)
                : _child(std::move(child)) { }
            
            CSG *child() const { return _child.get(); }
            glm::mat4 const&matrix() const { return _matrix; }
        };
        
        inline csg_ptr sphere(float radius) {
            return std14::make_unique<sphere_t>(radius);
        }
        
        inline csg_ptr plane(glm::vec4 normal) {
            return std14::make_unique<plane_t>(normal);
        }
        
        inline csg_ptr transform(csg_ptr child, glm::mat4 const&matrix) {
            return std14::make_unique<transform_t>(std::move(child), matrix);
        }
    }
}

#endif
