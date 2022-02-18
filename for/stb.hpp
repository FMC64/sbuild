#pragma once

#include <cstdint>

namespace stb {

struct Img {
	uint32_t size;
	uint32_t size_mask;
	uint32_t *data;

	Img(const char *path, bool is_alpha);
	~Img(void);

	inline uint32_t sample(uint32_t x, uint32_t y)
	{
		return data[(x & size_mask) * size + (y & size_mask)];
	}
};

}