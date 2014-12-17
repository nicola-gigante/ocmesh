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
#include <utility>
#include <cassert>
#include <vector>
#include <random>
#include <limits>

#include "csg.h"
#include "voxel.h"

using namespace ocmesh;

int main()
{
    csg::ptr sphere = csg::sphere(42);
    
    assert(sphere->distance({  0, 0, 0 }) == -42);
    assert(sphere->distance({ 43, 0, 0 }) ==   1);
    
    csg::ptr cube = csg::cube(42);
    
    assert(cube->distance({   0,   0,   0}) == -21);
    assert(cube->distance({  21,  21,  21}) ==   0);
    assert(cube->distance({  22,  21,  21}) ==   1);
    
    std::random_device dev;
    std::mt19937 gen(dev());
    std::uniform_int_distribution<uint16_t> dist(0, voxel::max_coordinate);
    
    for(int i = 0; i < 1000; ++i) {
        uint16_t x = dist(gen), y = dist(gen), z = dist(gen);
        
        glm::u16vec3 coordinates = {x, y, z};
        glm::u16vec3 unpacked = voxel(coordinates, 0, 0).coordinates();
    
        assert(unpacked == coordinates);
    }
    
    return 0;
}
