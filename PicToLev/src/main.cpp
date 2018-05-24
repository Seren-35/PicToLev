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

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <gsl/gsl_util>
#include <gsl/span>
#include "container_hash.h"
#include "lodepng.h"
#include "tiles.h"

constexpr unsigned image_count = 2;
constexpr unsigned tile_size = 32;
constexpr unsigned char empty_tile_row[tile_size] {};
constexpr unsigned max_tiles = 4090;
constexpr unsigned tileset_width = 10;
constexpr unsigned tileset_image_width = tileset_width * tile_size;
constexpr unsigned word_size = 4;

template<class T>
struct image_file_context {
	std::string filename;
	std::vector<unsigned char> buffer;
	bool known_size;
	unsigned width;
	unsigned height;
	lodepng::State state;
	tile_vector<T> tiles;
	typename tile_vector<T>::iterator tiles_it;
};

bool get_tile_list(image_file_context<const unsigned char>& context) {
	std::vector<unsigned char> file_buffer;
	unsigned error = lodepng::load_file(file_buffer, context.filename);
	if (error != 0) {
		std::cerr << "An error has occurred when loading file ";
		std::cerr << context.filename << ":\n";
		std::cerr << lodepng_error_text(error) << std::endl;
		return false;
	}
	unsigned width;
	unsigned height;
	error = lodepng::decode(context.buffer, width, height, context.state, file_buffer);
	if (error != 0) {
		std::cerr << "An error has occurred when decoding file ";
		std::cerr << context.filename << ":\n";
		std::cerr << lodepng_error_text(error) << std::endl;
		return false;
	}
	if (context.known_size) {
		if (width != context.width || height != context.height) {
			std::cerr << "File " << context.filename << " has incorrect image size\n";
			std::cerr << "Sizes of all images must be equal" << std::endl;
			return false;
		}
	} else {
		context.width = width;
		context.height = height;
	}
	if (width & 31 || height & 31) {
		std::cerr << "File " << context.filename << " has incorrect image size\n";
		std::cerr << "Width and height must be multiples of 32" << std::endl;
		return false;
	}
	const auto image = buffer_to_image(std::as_const(context.buffer).data(), width, height);
	context.tiles = image_to_tile_list(image.begin(), image.end(), tile_size);
	return true;
}

struct level_file_context {
	unsigned layer_width;
	unsigned layer_height;
	std::vector<std::vector<unsigned>> layer;
};

void write_data_streams(level_file_context& context) {
	unsigned reduced_width = (context.layer_width - 1) / word_size + 1;
	unsigned rounded_width = reduced_width * word_size;
	std::vector<std::size_t> words(context.layer_height * reduced_width);
	std::map<std::vector<unsigned>, std::size_t> tile_dictionary {{std::vector<unsigned>(word_size, 0), 0}};
	auto word_it = words.begin();
	for (auto&& layer_row : context.layer) {
		layer_row.resize(rounded_width);
		for (auto it = layer_row.begin(); it != layer_row.end(); it += word_size) {
			auto result = tile_dictionary.emplace(std::vector<unsigned>(it, it + word_size), tile_dictionary.size());
			*word_it++ = result.first->second;
		}
	}
	std::size_t dictionary_size = tile_dictionary.size();
	std::vector<std::vector<unsigned>> ordered_dictionary(dictionary_size);
	for (const auto& key_value : tile_dictionary) {
		ordered_dictionary[key_value.second] = key_value.first;
	}
	std::ofstream stream3("Stream3", std::ios::binary);
	for (const auto& word : ordered_dictionary) {
		for (const auto& tile : word) {
			stream3.write(reinterpret_cast<const char*>(&tile), 2);
		}
	}
	std::ofstream stream4("Stream4", std::ios::binary);
	for (const auto& word : words) {
		stream4.write(reinterpret_cast<const char*>(&word), 2);
	}
}

int main(int argc, char* argv[]) try {
	if (argc != image_count + 1) {
		std::cerr << "PicToLev expects exactly " << image_count << " arguments" << std::endl;
		return 1;
	}
	image_file_context<const unsigned char> inputs[image_count];
	for (unsigned i = 0; i < image_count; i++) {
		auto& input = inputs[i];
		input.filename = argv[i + 1];
		if (i != 0) {
			input.known_size = true;
			input.width = inputs->width;
			input.height = inputs->height;
		} else {
			input.known_size = false;
		}
		input.state.decoder.color_convert = false;
		input.state.info_raw.colortype = LCT_PALETTE;
		if (!get_tile_list(inputs[i]))
			return 1;
	}
	for (auto&& index : inputs[1].buffer) {
		index = index != 0;
	}
	level_file_context level;
	level.layer_width = inputs->width / tile_size;
	level.layer_height = inputs->height / tile_size;
	level.layer.assign(level.layer_height, std::vector<unsigned>(level.layer_width));
	using image_t = image_fragment<const unsigned char>;
	using tile_t = std::vector<image_t>;
	tile_t empty_tile(image_count, image_t(tile_size, gsl::span<const unsigned char>(empty_tile_row)));
	std::unordered_map<tile_t, std::size_t, container_deep_hash_t<tile_t, std::hash<unsigned char>>> tiles {{std::move(empty_tile), 0}};
	for (auto&& context : inputs) {
		context.tiles_it = context.tiles.begin();
	}
	for (auto&& layer_row : level.layer) {
		for (auto&& layer_tile : layer_row) {
			tile_t tile;
			for (auto&& context : inputs) {
				tile.emplace_back(std::move(*context.tiles_it));
				++context.tiles_it;
			}
			auto result = tiles.emplace(std::move(tile), tiles.size());
			layer_tile = gsl::narrow_cast<unsigned>(result.first->second);
		}
	}
	unsigned tile_count = gsl::narrow_cast<unsigned>(tiles.size());
	if (tile_count > max_tiles) {
		std::cerr << "The resulting tileset would have more than ";
		std::cerr << max_tiles << " tiles" << std::endl;
		return 1;
	}
	unsigned tileset_height = (tile_count - 1) / tileset_width + 1;
	unsigned tileset_image_height = tileset_height * tile_size;
	image_file_context<unsigned char> outputs[image_count];
	for (unsigned i = 0; i < image_count; i++) {
		const auto& input = inputs[i];
		auto& output = outputs[i];
		output.width = tileset_image_width;
		output.height = tileset_image_height;
		output.buffer.assign(tileset_image_width * tileset_image_height, 0);
		const auto output_image = buffer_to_image(output.buffer.data(), tileset_image_width, tileset_image_height);
		output.tiles = image_to_tile_list(output_image.begin(), output_image.end(), tile_size);
		for (const auto& key_value : tiles) {
			const auto& src = key_value.first[i];
			const auto& dest = output.tiles[key_value.second];
			auto out = dest.begin();
			for (auto it = src.begin(); it != src.end(); ++it, ++out) {
				std::copy(it->begin(), it->end(), out->begin());
			}
		}
		const std::string file_prefix = input.filename.substr(0, input.filename.rfind('.'));
		output.filename = file_prefix + "-output-" + std::to_string(i + 1) + ".png";
		output.state = input.state;
		output.state.encoder.auto_convert = false;
		output.state.info_raw.colortype = LCT_PALETTE;
		std::vector<unsigned char> file_buffer;
		int error = lodepng::encode(file_buffer, output.buffer, tileset_image_width, tileset_image_height, inputs[i].state);
		if (error != 0) {
			std::cerr << "An error has occurred when decoding file ";
			std::cerr << output.filename << ":\n";
			std::cerr << lodepng_error_text(error) << std::endl;
			return 1;
		}
		error = lodepng::save_file(file_buffer, output.filename);
		if (error != 0) {
			std::cerr << "An error has occurred when saving file ";
			std::cerr << output.filename << ":\n";
			std::cerr << lodepng_error_text(error) << std::endl;
			return 1;
		}
	}
	write_data_streams(level);
	return 0;
} catch (const std::exception& e) {
	std::cerr << "An unexpected runtime error has occurred:\n";
	std::cerr << e.what() << std::endl;
	return 2;
} catch (...) {
	std::cerr << "An unknown error has occurred" << std::endl;
	return 2;
}
