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
#include "std14/memory"

#include <iostream>

using namespace tbl;

constexpr int x2(int n) { return n * 2; }
constexpr int sum(int a, int b) { return a + b; }

constexpr auto a = map(x2, irange<int, 0, 10>());
constexpr auto result = foldr(sum, 0, a);

void test_table() {
    for (auto i : a) {
        std::cout << i << "\n";
    }
    
    std::cout << result << "\n";
}

void test_std14() {
    auto i = std14::make_unique<int>(42);
}

int main(int argc, const char * argv[])
{
    test_table();
    test_std14();
    
    return 0;
}

