////////////////////////////////////////////////////////////
//
// Copyright (c) 2018 Sir Ementaler
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////

#ifndef PICTOLEV_CONTAINER_TRAITS_H
#define PICTOLEV_CONTAINER_TRAITS_H

#include <iterator>
#include <type_traits>
#include <utility>

template<class T, class = void>
struct is_iterable : std::false_type {};

template<class T>
struct is_iterable<T, std::enable_if_t<std::is_convertible_v<
	decltype(std::begin(std::declval<T>()) != std::end(std::declval<T>())),
	bool
>>> : std::true_type {};

template<class T>
inline constexpr bool is_iterable_t = is_iterable<T>::value;

template<class T>
using container_iterator_traits = std::iterator_traits<
	std::decay_t<decltype(std::begin(std::declval<T>()))>
>;

#endif
