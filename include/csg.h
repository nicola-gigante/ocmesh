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
#include <glm/gtx/transform.hpp>

#include <std14/memory>

namespace ocmesh {

    namespace details {
        /*
         * User facing primitives and operations.
         */
        class CSG {
        public:
            virtual ~CSG();
            
            virtual float distance(glm::vec3 const& from) = 0;
        };
        
        using csg_ptr = std::unique_ptr<CSG>;
        
        class sphere_t : public CSG
        {
            float _radius;
            
        public:
            sphere_t(float radius) : _radius(radius) { }
            
            float radius() const { return _radius; }
            
            float distance(glm::vec3 const& from) override;
        };
        
        class plane_t : public CSG
        {
            glm::vec3 _normal;
            
        public:
            plane_t(glm::vec3 const&normal) : _normal(normal) { }
            
            glm::vec3 const&normal() const { return _normal; }
            
            float distance(glm::vec3 const& from) override;
        };
        
        class binary_operation_t : public CSG {
            csg_ptr _left;
            csg_ptr _right;
            
        public:
            binary_operation_t(csg_ptr left, csg_ptr right)
                : _left(std::move(left)), _right(std::move(right)) { }
            
            CSG *left()  const { return _left.get();  }
            CSG *right() const { return _right.get(); }
        };
        
        class union_t : public binary_operation_t {
        public:
            using binary_operation_t::binary_operation_t;
            
            float distance(glm::vec3 const& from) override;
        };
        
        class intersection_t : public binary_operation_t {
        public:
            using binary_operation_t::binary_operation_t;
            
            float distance(glm::vec3 const& from) override;
        };
        
        class difference_t : public binary_operation_t {
        public:
            using binary_operation_t::binary_operation_t;
            
            float distance(glm::vec3 const& from) override;
        };
        
        class transform_t : public CSG
        {
            csg_ptr   _child;
            glm::mat4 _transform;
            
        public:
            transform_t(csg_ptr child, glm::mat4 const&transform)
                : _child(std::move(child)), _transform(transform) { }
            
            CSG *child() const { return _child.get(); }
            glm::mat4 const&transform() const { return _transform; }
            
            float distance(glm::vec3 const& from) override;
        };
        
        
        // Primitives
        inline csg_ptr sphere(float radius) {
            return std14::make_unique<sphere_t>(radius);
        }
        
        inline csg_ptr plane(glm::vec3 const&normal) {
            return std14::make_unique<plane_t>(glm::normalize(normal));
        }
        
        inline csg_ptr xplane() {
            return plane({1, 0, 0});
        }
        
        inline csg_ptr yplane() {
            return plane({0, 1, 0});
        }
        
        inline csg_ptr zplane() {
            return plane({0, 0, 1});
        }
        
        // Combining nodes
        inline csg_ptr unite(csg_ptr left, csg_ptr right) {
            return std14::make_unique<union_t>(std::move(left),
                                               std::move(right));
        }
        
        template<typename ...Args>
        inline csg_ptr unite(csg_ptr node, Args&& ...args) {
            return unite(std::move(node), unite(std::forward<Args>(args)...));
        }
        
        inline csg_ptr intersect(csg_ptr left, csg_ptr right) {
            return std14::make_unique<intersection_t>(std::move(left),
                                                      std::move(right));
        }
        
        template<typename ...Args>
        inline csg_ptr intersect(csg_ptr node, Args&& ...args) {
            return intersect(std::move(node), unite(std::forward<Args>(args)...));
        }
        
        inline csg_ptr subtract(csg_ptr left, csg_ptr right) {
            return std14::make_unique<difference_t>(std::move(left),
                                                    std::move(right));
        }
        
        template<typename ...Args>
        inline csg_ptr subtract(csg_ptr node, Args&& ...args) {
            return subtract(std::move(node), unite(std::forward<Args>(args)...));
        }
        
        // Transform nodes
        inline csg_ptr transform(csg_ptr node, glm::mat4 const&matrix) {
            return std14::make_unique<transform_t>(std::move(node), matrix);
        }
        
        inline csg_ptr scale(csg_ptr node, glm::vec3 const&factors) {
            return transform(std::move(node), glm::scale(factors));
        }
        
        inline csg_ptr scale(csg_ptr node, float factor) {
            return scale(std::move(node), { factor });
        }
        
        inline csg_ptr rotate(csg_ptr node, float angle, glm::vec3 const&axis) {
            return transform(std::move(node), glm::rotate(angle, axis));
        }
        
        inline csg_ptr translate(csg_ptr node, glm::vec3 const&offsets) {
            return transform(std::move(node), glm::translate(offsets));
        }
        
        inline csg_ptr xtranslate(csg_ptr node, float offset) {
            return translate(std::move(node), {offset, 0, 0});
        }
        
        inline csg_ptr ytranslate(csg_ptr node, float offset) {
            return translate(std::move(node), {0, offset, 0});
        }
        
        inline csg_ptr ztranslate(csg_ptr node, float offset) {
            return translate(std::move(node), {0, 0, offset});
        }
        
        // Compound primitives
        inline csg_ptr cube(float side) {
            return unite(xtranslate(xplane(), side / 2),
                         ytranslate(yplane(), side / 2),
                         ztranslate(zplane(), side / 2));
        }
    }
    
    namespace csg {
        using details::CSG;
        using details::csg_ptr;
        using details::sphere;
        using details::plane;
        using details::xplane;
        using details::yplane;
        using details::zplane;
        using details::unite;
        using details::intersect;
        using details::subtract;
        using details::transform;
        using details::scale;
        using details::rotate;
        using details::translate;
        using details::cube;
        
        using ptr = csg_ptr;
    }
}

#endif
