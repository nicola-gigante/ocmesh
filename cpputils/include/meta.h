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

#ifndef CPPUTILS_META_H__
#define CPPUTILS_META_H__

#include <type_traits>

#define CPPUTILS_CONCAT__(x, y) CPPUTILS_CONCAT_2__(x,y)
#define CPPUTILS_CONCAT_2__(x, y) x ## y

#define CPPUTILS_REQUIRES_FRESH CPPUTILS_CONCAT__(UNFULFILLED_TEMPLATE_REQUIREMENT_, __LINE__)


// Note: this must stay on the same line.
#define REQUIRES(...) \
typename CPPUTILS_REQUIRES_FRESH = void, typename std::enable_if<::utils::details::true_t<CPPUTILS_REQUIRES_FRESH>::value && (__VA_ARGS__), int>::type = 0

namespace utils {
    
    namespace details
    {
        template<typename T>
        struct true_t : std::true_type { };
        
        template<typename T>
        using void_t = void;

        constexpr bool all() { return true; }
        
        template<typename T, typename ...Args,
                 REQUIRES(    std::is_same<T,    bool>::value),
                 REQUIRES(all(std::is_same<Args, bool>::value...))>
        bool all(T b, Args ...args)
        {
            return b && all(args...);
        }
        
        template<typename T, typename ...Args>
        constexpr bool same_type() {
            return all(std::is_same<T, Args>::value...);
        }
        
        constexpr bool any() { return false; }
        
        template<typename T, typename ...Args,
                 REQUIRES(same_type<bool, T, Args...>())>
        constexpr bool any(T b, Args ...args) {
            return b || any(args...);
        }
    }
    
    using details::true_t;
    using details::void_t;
    
    using details::all;
    using details::any;
    
    using details::same_type;
}

#endif
