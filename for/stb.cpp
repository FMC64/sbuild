#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include <cstdio>
#include <cmath>
#include <stdexcept>
#include "stb.hpp"

namespace stb {

/*static uint32_t log2(uint32_t s)
{
	for (size_t i = 0; i < 32; i++)
		if (1 << i == s)
			return i;
	return 0;
}*/

static uint8_t srgb_to_lin(uint8_t val)
{
	auto n = static_cast<double>(val) / 255.0;
	return std::pow(n, 2.2) * 255.2;
}

Img::Img(const char *path, bool is_alpha)
{
	int x, y, chan;
	int chan_req = is_alpha ? 4 : 3;
	auto img = stbi_load(path, &x, &y, &chan, chan_req);
	if (img == nullptr) {
		std::printf("ERR: can't load\n");
		throw std::runtime_error(path);
	}
	if (chan != chan_req) {
		std::printf("ERR: chan mismatch (req %d, got %d)\n", chan_req, chan);
		throw std::runtime_error(path);
	}
	if (x != y) {
		std::printf("ERR: not squared (w=%d, h=%d)\n", x, y);
		throw std::runtime_error(path);
	}
	if (x != size) {
		std::printf("ERR: size mismatch (exp=%u, got=%d)\n", size, x);
		throw std::runtime_error(path);
	}
	/*size = x;
	auto p2 = log2(size);
	if (1 << p2 != size) {
		std::printf("ERR: size not pow2 (w=%d, h=%d)\n", x, y);
		throw std::runtime_error(path);
	}
	size_mask = 0;
	for (size_t i = 0; i < p2; i++)
		size_mask |= 1 << i;*/

	size_t c = chan;
	data = new uint32_t[size * size] {};
	auto udata = reinterpret_cast<uint8_t*>(data);
	for (size_t i = 0; i < size; i++)
		for (size_t j = 0; j < size; j++)
			for (size_t k = 0; k < c; k++)
				udata[(i * size + j) * sizeof(uint32_t) + k] = srgb_to_lin(img[(j * size + i) * c + k]);
	stbi_image_free(img);
}

Img::~Img(void)
{
	delete[] data;
}

}