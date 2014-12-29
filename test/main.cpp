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
#include <limits>

#include "csg.h"
#include "octree.h"

using namespace ocmesh;


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
    
    scene.parse(input);
    
    std::cout << scene << "\n";
    
    octree c;
    
    c.build([](voxel v) -> voxel::material_t {
        if(v.level() > 0)
            return 2;
        return voxel::unknown_material;
    });
    
    (*c.begin()) = c.begin()->with_material(1);
    
    for(auto v : c) {
        std::cout << v << "\n";
    }
    
    c.mesh(octree::obj, output);
    
    return 0;
}
