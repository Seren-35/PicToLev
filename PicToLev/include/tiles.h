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

#ifndef PICTOLEV_TILES_H
#define PICTOLEV_TILES_H

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <vector>
#include <gsl/gsl_util>
#include <gsl/span>

template<class T>
using image_fragment = std::vector<gsl::span<T>>;

template<class T>
auto buffer_to_image(gsl::span<T> buffer, std::size_t width, std::size_t height) {
	image_fragment<T> image(height);
	std::generate(image.begin(), image.end(), [buffer, width]() mutable {
		gsl::span<T> row = buffer.subspan(0, width);
		buffer = buffer.subspan(width);
		return row;
	});
	return image;
}

template<class T>
using tile_vector = std::vector<image_fragment<T>>;

template<class ForwardIt>
auto image_to_tile_list(ForwardIt first, ForwardIt last, std::size_t tile_size) {
	using container_type = std::remove_reference_t<typename ForwardIt::reference>;
	using value_type = std::remove_reference_t<typename container_type::reference>;
	std::size_t height = std::distance(first, last) / tile_size;
	std::size_t width = first != last ? first->size() / tile_size : 0;
	tile_vector<value_type> tiles(width * height, image_fragment<value_type>(tile_size));
	auto out = tiles.begin();
	while (first != last) {
		ForwardIt second = first;
		std::advance(second, tile_size);
		for (gsl::index x = 0; x != first->size(); x += tile_size) {
			std::transform(first, second, out->begin(), [=](const auto& row) {
				return gsl::span<value_type>(row.data() + x, tile_size);
			});
			++out;
		}
		first = second;
	}
	return tiles;
}

#endif
