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

#ifndef PICTOLEV_BINARY_SERIALIZATION_H
#define PICTOLEV_BINARY_SERIALIZATION_H

#include <algorithm>
#include <iterator>
#include <limits>
#include <ostream>
#include <type_traits>
#include <gsl/gsl_util>

enum class endian {
#ifdef _WIN32
	little = 0,
	big = 1,
	native = little
#else
	little = __ORDER_LITTLE_ENDIAN__,
	big = __ORDER_BIG_ENDIAN__,
	native = __BYTE_ORDER__
#endif
};

template<std::size_t N>
void write_buffer(std::ostream& stream, const char (&buffer)[N]) {
	stream.write(buffer, N);
}

template<endian Endian = endian::native, typename T>
std::enable_if_t<std::is_integral_v<T> && (sizeof(T) == 1)>
write_binary(std::ostream& stream, T value) {
	stream.put(value);
}

template<endian Endian, typename T>
std::enable_if_t<std::is_integral_v<T> && (sizeof(T) > 1)>
write_binary(std::ostream& stream, T value) {
	static_assert(Endian == endian::little || Endian == endian::big);
	std::make_unsigned_t<T> u_value = value;
	const auto generator = [&u_value]() {
		const char result = gsl::narrow_cast<unsigned char>(u_value);
		u_value >>= std::numeric_limits<unsigned char>::digits;
		return result;
	};
	char buffer[sizeof(T)] {};
	if constexpr (Endian == endian::little)
		std::generate(std::begin(buffer), std::end(buffer), generator);
	else
		std::generate(std::rbegin(buffer), std::rend(buffer), generator);
	write_buffer(stream, buffer);
}

#endif
