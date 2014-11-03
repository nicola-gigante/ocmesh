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

#include "static_table.h"

#include <iostream>

using namespace tbl;

constexpr int x2(int n) { return n * 2; }
constexpr int sum(int a, int b) { return a + b; }

constexpr auto a1 = map<0, 10>(x2);
constexpr auto a2 = map(x2, irange<int, 0, 10>());

constexpr auto r1 = foldr(sum, 0, a1);
constexpr auto r2 = foldr<0, 10>(sum, 0);
constexpr auto r3 = foldr<0, 10>(sum, 0);

void test_table() {
    for (auto i : a1) {
        std::cout << i << "\n";
    }
    
    for (auto i : a2) {
        std::cout << i << "\n";
    }
    
    std::cout << r1 << "\n";
    std::cout << r2 << "\n";
    std::cout << r3 << "\n";
}

int main(int argc, const char * argv[])
{
    test_table();
    
    return 0;
}

