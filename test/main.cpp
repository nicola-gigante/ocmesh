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

#include <iostream>
#include <fstream>
#include <utility>
#include <cassert>
#include <vector>
#include <random>
#include <cmath>
#include <limits>

#include "csg.h"
#include "octree.h"

using namespace ocmesh;

enum intersection_result {
    inside,
    outside,
    intersect
};

intersection_result intersection(csg::object *obj, voxel v) {
    glm::vec3 coordinates = glm::vec3(v.coordinates());
    float side = v.size();
    float diagonal = std::sqrt(3) * side;
    
    glm::vec3 center = coordinates + glm::vec3{ side / 2, side / 2, side / 2 };
    
    float d = obj->distance(center);
    if(std::abs(d) < diagonal / 2 && v.level() <= 7)
        return intersect;
    
    return d > 0 ? outside : inside;
}

int main(int argc, char *argv[])
{
    if(argc < 3) {
        std::cerr << "Usage: ocmesh <CSG input> <mesh output>\n";
        return 1;
    }
    
    std::string inputfile = argv[1];
    std::string outputfile = argv[2];
    
    std::ifstream input(argv[1]);
    std::ofstream output(argv[2]);

    
    if(!input) {
        std::cerr << "Unable to open file for reading: '" << argv[1] << "'\n";
        return 2;
    }
    
    if(!output) {
        std::cerr << "Unable to open file for writing: '" << argv[2] << "'\n";
        return 3;
    }
    
    csg::scene scene;
    
    auto result = scene.parse(input);
    
    if(!result) {
        std::cerr << result.error() << "\n";
        return 4;
    }
    
    std::cout << "Scene: \n";
    std::cout << scene << "\n";
    
    octree c;
    
    c.build([&](voxel v) -> voxel::material_t {
        for(auto *obj : scene) {
            intersection_result r = intersection(obj, v);
            if(r == inside)
                return obj->material();
            if(r == intersect)
                return voxel::unknown_material;
        }
        
        return voxel::void_material;
    });
    
    std::cout << "Octree built\n";
    
    c.mesh(octree::obj, output);
    
    return 0;
}
