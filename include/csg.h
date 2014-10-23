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

#include <memory>

namespace ocmesh {

    namespace details {
        template<typename Derived>
        class factory_t {
        public:
            template<typename ...Args>
            static auto make() ->
            {
                return std::unique_ptr<Derived>(new Derived(std::forward<Args>(args)));
            }
        };
    }
    
    /*
     * User facing primitives and operations.
     */
    class CSG {
    public:
        virtual ~CSG() = default;
    };
    
    using csg_ptr = std::unique_ptr<CSG>;
    
    class Sphere : public CSG
    {
        float _radius;
        
    public:
        Sphere(float radius) : _radius(radius) { }
        
        float radius() const { return _radius; }
    };
    
    class Plane : public CSG
    {
        glm::vec4 _normal;
        
    public:
        Plane(glm::vec4 const&normal) : _normal(normal) { }
        
        glm::vec4 normal() const { return _normal; }
    };

    class Translation : public CSG {
        csg_ptr _child;
    public:
        Translation(csg_ptr child) : _child(std::move(child)) { }
        
        CSG *child() const { return _child.get(); }
    };
}

#endif
