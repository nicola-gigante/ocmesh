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

#ifndef CPPUTILS_HAS_MEMBER_H
#define CPPUTILS_HAS_MEMBER_H

/*
 * Traits utilities to recognize the existence of a member function in a class.
 *
 * An invocation like DECLARE_HAS_MEMBER_TRAIT(save) generates a
 * trait class called has_member_save, which take a class and a function signature.
 * If the given class has an accessible member function called 'save' that
 * has a compatible signature, the ::value static member results true, false
 * otherwise. Moreover, the actual type of the trait is an alias for
 * the 'has_member_save_tag' type if the member exists and can be called,
 * and 'doesnt_have_member_save_tag' otherwise. This allows an easy tag dispatch.
 *
 *
 *     DECLARE_HAS_MEMBER_TRAIT(save)
 *
 *     template<typename T>
 *     bool func(T obj) {
 *        return save_if_possible(obj, has_member_save<T, bool(std::string)>());
 *     }
 *
 *     template<typename T>
 *     bool save_if_possible(T obj, has_member_save_tag) {
 *        return obj.save("file.txt");
 *     }
 *
 *     template<typename T>
 *     bool save_if_possible(T obj, doesnt_have_member_save_tag) {
 *        // Do something else...
 *        return false;
 *     }
 */
#define DECLARE_HAS_MEMBER_TRAIT(Member)                                       \
                                                                               \
struct has_member_##Member##_tag : public std::true_type {};                   \
struct doesnt_have_member_##Member##_tag : public std::false_type {};          \
                                                                               \
template<typename T, typename S = void,                                        \
         bool _ = std::is_class<T>::value>                                     \
struct has_member_##Member##_trait : public std::false_type {                  \
    using type = doesnt_have_member_##Member##_tag;                            \
};                                                                             \
                                                                               \
template<typename T, typename R, typename ...Args>                             \
struct has_member_##Member##_trait<T, R(Args...), true>                        \
{                                                                              \
private:                                                                       \
    template<typename TT, typename ...AArgs>                                   \
    static auto invoke(TT &&obj, AArgs&& ...args)                              \
        -> decltype(std::forward<TT>(obj).Member(std::forward<AArgs>(args)...),\
                    std::true_type());                                         \
                                                                               \
    static std::false_type invoke(...);                                        \
                                                                               \
    using _return_type = decltype(invoke(std::declval<T>(),                    \
                                         std::declval<Args>()...));            \
    static const bool _has_member = _return_type::value;                       \
                                                                               \
public:                                                                        \
    static const bool value = _has_member &&                                   \
                             (std::is_void<R>::value ||                        \
                              std::is_convertible<_return_type, R>::value);    \
    using type = typename                                                      \
                 std::conditional<value,                                       \
                                  has_member_##Member##_tag,                   \
                                  doesnt_have_member_##Member##_tag>::type;    \
                                                                               \
explicit constexpr operator bool() const { return value; }                     \
};                                                                             \
                                                                               \
template<typename T, typename F>                                               \
using has_member_##Member = typename has_member_##Member##_trait<T, F>::type;  \

#endif
