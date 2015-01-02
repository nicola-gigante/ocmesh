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

#include "utils/map_iterator.h"

#include <ostream>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace ocmesh {
    
    namespace details {
        
        object::~object() = default;
        binary_operation_t::~binary_operation_t() = default;
        
        float sphere_t::distance(glm::vec3 const&from) {
            return glm::length(from) - _radius;
        }
        
        bounding_box sphere_t::bounding_box() const
        {
            glm::vec3 left_bottom_back = { -radius(), -radius(), -radius() };
            glm::vec3 right_up_front   = {  radius(),  radius(),  radius() };
            
            return { left_bottom_back, right_up_front };
        }
        
        void sphere_t::dump(std::ostream &o) const {
            o << "sphere(" << _radius << ")";
        }
        
        float cube_t::distance(glm::vec3 const&from) {
            return std::max({std::abs(from.x),
                             std::abs(from.y),
                             std::abs(from.z)}) - _side / 2;
        }
        
        bounding_box cube_t::bounding_box() const {
            float half = side() / 2;
            
            glm::vec3 left_bottom_back = { -half, -half, -half };
            glm::vec3 right_up_front   = {  half,  half,  half };
            
            return { left_bottom_back, right_up_front };
        }
        
        void cube_t::dump(std::ostream &o) const {
            o << "cube(" << _side << ")";
        }
        
        float toplevel_t::distance(glm::vec3 const& from) {
            return _child->distance(from);
        }
        
        bounding_box toplevel_t::bounding_box() const {
            return _child->bounding_box();
        }
        
        void toplevel_t::dump(std::ostream &o) const {
            o << "build " << _material << " ";
            _child->dump(o);
        }
        
        bounding_box scene::bounding_box() const
        {
            return std::accumulate(begin() + 1, end(),
                                   (*begin())->bounding_box(),
               [](struct bounding_box bb, object *obj2) {
                   return bb + obj2->bounding_box();
               });
        }
        
        float union_t::distance(const glm::vec3 &from) {
            return std::min(left()->distance(from), right()->distance(from));
        }
        
        /*
         * Component-wise min/max functions, useful for bounding boxes
         */
        glm::vec3 cmin(glm::vec3 v1, glm::vec3 v2) {
            return {
                std::min(v1.x, v2.x),
                std::min(v1.y, v2.y),
                std::min(v1.z, v2.z)
            };
        }
        
        glm::vec3 cmax(glm::vec3 v1, glm::vec3 v2) {
            return {
                std::max(v1.x, v2.x),
                std::max(v1.y, v2.y),
                std::max(v1.z, v2.z)
            };
        }
        
        /*
         * This is factored out because we reuse it also for the intersection
         * and to find the bounding box of an entire scene as the union of the
         * BBs of single objects
         */
        bounding_box operator+(bounding_box const&bb1, bounding_box const&bb2) {
            return {
                cmin(bb1.min, bb2.min),
                cmax(bb1.max, bb2.max)
            };
        }
        
        std::ostream &operator<<(std::ostream &s, bounding_box const&bb) {
            s << "{" << bb.min.x << ", " << bb.min.y << ", " << bb.min.z << "} - ";
            s << "{" << bb.max.x << ", " << bb.max.y << ", " << bb.max.z << "}";
            return s;
        }
        
        bounding_box union_t::bounding_box() const {
            return left()->bounding_box() + right()->bounding_box();
        }
        
        void union_t::dump(std::ostream &o) const {
            o << "unite(";
            left()->dump(o);
            o << ", ";
            right()->dump(o);
            o << ")";
        }
        
        float intersection_t::distance(const glm::vec3 &from) {
            return std::max(left()->distance(from), - right()->distance(from));
        }
        
        /*
         * TODO: find a better intersection bounding box
         */
        bounding_box intersection_t::bounding_box() const {
            return left()->bounding_box() + right()->bounding_box();
        }
        
        void intersection_t::dump(std::ostream &o) const {
            o << "intersect(";
            left()->dump(o);
            o << ", ";
            right()->dump(o);
            o << ")";
        }
        
        float difference_t::distance(const glm::vec3 &from) {
            return std::max(left()->distance(from), right()->distance(from));
        }
        
        void difference_t::dump(std::ostream &o) const {
            o << "subtract(";
            left()->dump(o);
            o << ", ";
            right()->dump(o);
            o << ")";
        }
        
        bounding_box difference_t::bounding_box() const {
            return left()->bounding_box();
        }
        
        float transform_t::distance(glm::vec3 const&from)
        {
            glm::vec4 v = { from.x, from.y, from.z, 1.0f };
            
            return child()->distance((world_to_object() * v).xyz());
        }
        
        /*
         * Axis-aligned bounding box of a transformed axis-aligned bounding box
         *
         * http://dev.theomader.com/transform-bounding-boxes/
         */
        bounding_box transform_t::bounding_box() const {
            auto bb = _child->bounding_box();
            auto transform = object_to_world();
            
            auto xmin = glm::column(transform, 0).xyz() * bb.min.x;
            auto xmax = glm::column(transform, 0).xyz() * bb.max.x;
            
            auto ymin = glm::column(transform, 1).xyz() * bb.min.y;
            auto ymax = glm::column(transform, 1).xyz() * bb.max.y;
            
            auto zmin = glm::column(transform, 2).xyz() * bb.min.z;
            auto zmax = glm::column(transform, 2).xyz() * bb.max.z;
            
            auto offset = glm::column(transform, 3).xyz();
            
            return {
                cmin(xmin, xmax) + cmin(ymin, ymax) + cmin(zmin, zmax) + offset,
                cmax(xmin, xmax) + cmax(ymin, ymax) + cmax(zmin, zmax) + offset,
            };
        }
        
        void transform_t::dump(std::ostream &o) const {
            o << "transform(matrix..., ";
            _child->dump(o);
            o << ")";
        }
    }
}
