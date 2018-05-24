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

#ifndef PICTOLEV_CONTAINER_HASH_H
#define PICTOLEV_CONTAINER_HASH_H

#include <cstddef>
#include <functional>
#include <iterator>
#include <numeric>
#include <utility>
#include <type_traits>
#include "container_traits.h"

template<class T, class Hash, class = void>
struct container_hash {
	container_hash(const container_hash&) = delete;
	container_hash& operator=(const container_hash&) = delete;
};

template<class T, class Hash>
struct container_hash<T, Hash, std::enable_if_t<is_iterable_t<T>>> {
	std::size_t operator()(const T& container) const {
		using value_type = typename container_iterator_traits<T>::value_type;
		return std::accumulate(
			std::begin(container),
			std::end(container),
			std::size_t(),
			[hash = Hash()](std::size_t seed, const value_type& value) {
				return seed ^ (hash(value) + 0x9E3779B9 + (seed << 6) + (seed >> 2));
			}
		);
	}
};

template<class T, class Hash, class = void>
struct container_deep_hash {
	using type = Hash;
};

template<class T, class Hash>
struct container_deep_hash<T, Hash, std::enable_if_t<is_iterable_t<T>>> {
	using type = container_hash<T, typename container_deep_hash<typename T::value_type, Hash>::type>;
};

template<class T, class Hash>
using container_deep_hash_t = typename container_deep_hash<T, Hash>::type;

#endif
