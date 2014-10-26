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

#ifndef CPPUTILS_STATIC_TABLE_H
#define CPPUTILS_STATIC_TABLE_H

#include <array>
#include <utility>
#include <type_traits>

#include <std14/utility>

/*
 * This header defines constexpr utility functions that come handy to 
 * precompute an std::array of values at compile time to implement lookup tables,
 * precomputations steps for bigger algorithms, etc...
 */
namespace tbl {
    namespace details {
        /*
         * We cannot directly manipulate std::array instances because it doesn't
         * have constexpr member functions (which have been added in C++14)
         */
        template<typename T, size_t N>
        class table
        {
        public:
            using value_type      = T;
            using const_iterator  = T const*;
            using const_pointer   = T const*;
            using const_reference = T const&;
            using iterator        = const_iterator;
            using pointer         = const_pointer;
            using reference       = const_reference;
            using difference_type = uintptr_t;
            using size_type       = size_t;
            
            template<typename ...Args,
                     typename =
                     typename std::enable_if<sizeof...(Args) == N>::type>
            constexpr table(Args ...args) : _data{ args... } { }
            
            constexpr size_type size() const { return N; }
            
            constexpr iterator       begin()  const { return _data; }
            constexpr iterator       end()    const { return _data + size(); }
            constexpr const_iterator cbegin() const { return _data; }
            constexpr const_iterator cend()   const { return _data + size(); }
            
            constexpr T operator[](size_type i) const {
                return _data[i];
            }
            
            constexpr operator std::array<T, N>() const {
                return to_array(std14::make_index_sequence<N>());
            }
            
        private:
            template<size_t ...Idxs>
            std::array<T, N> to_array(std14::index_sequence<Idxs...>) const {
                return { _data[Idxs]... };
            }
            
            T _data[N];
        };
    
        // irange() implementation
        template<typename T, size_t Begin, size_t ...Idxs>
        constexpr
        table<T, sizeof...(Idxs)> irange(std14::index_sequence<Idxs...>)
        {
            return { T(Begin + Idxs)... };
        }
        
        // map() implementation
        template<typename T, size_t N, typename Ret, size_t ...Idxs>
        constexpr
        table<Ret, N> map(table<T, N> const&data, Ret (*f)(T),
                          std14::index_sequence<Idxs...>) {
            return { f(data[Idxs])... };
        }
        
        // Foldr implementation
        template<typename A, typename B, size_t N>
        constexpr
        B foldr(B (*f)(A, B), B zero,
                table<A, N> const&data, size_t index)
        {
            return index == N ? zero
                              : f(data[index], foldr(f, zero, data, index + 1));
        }
        
        // Foldl implementation
        template<typename A, typename B, size_t N>
        constexpr
        B foldl(B (*f)(A, B), B zero,
                table<A, N> const&data, size_t index)
        {
            return index == N ? zero
                              : foldl(f, f(zero, data[index]), index + 1);
        }
    } // namespace details
    
    using details::table;
    
    /*
     * The irange() function returns a sequence filled with all the values of
     * type T in the range [Begin, End)
     */
    template<typename T, T Begin, T End>
    constexpr
    table<T, End - Begin> irange() {
        return details::irange<T, Begin>(std14::make_index_sequence<End - Begin>());
    }

    /*
     * The iota() function is a special case of irange(), that returns a 
     * sequence filled with all the values of type T in the range [0, N)
     */
    template<typename T, T N>
    constexpr table<T, N> iota() {
        return irange<T, T{}, N>();
    }
    
    /*
     * The map() function returns the given sequence transformed by applying
     * the given function to every element of the sequence
     */
    template<typename T, size_t N, typename Ret>
    constexpr
    table<Ret, N> map(Ret (*f)(T), table<T, N> const&data) {
        return details::map(data, f, std14::make_index_sequence<N>());
    }
    
    /*
     * The foldr() function accumulates on the right the result of a 
     * binary function applied recursively to all the elements of a sequence
     */
    template<typename A, typename B, size_t N>
    constexpr
    B foldr(B (*f)(A, B), B zero, table<A, N> const&data) {
        return foldr(f, zero, data, 0);
    }
    
    /*
     * Same as foldr() but accumulates on the left
     */
    template<typename A, typename B, size_t N>
    constexpr
    B foldl(B (*f)(B, A), B zero, table<A, N> const&data) {
        return foldl(f, zero, data, 0);
    }

    
    
} // namespace tbl
#endif


