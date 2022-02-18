#pragma once

#include <cstdint>

namespace stb {

struct Img {
	static constexpr uint32_t size = 128;
	static constexpr uint32_t size_mask = 0x7F;
	uint32_t *data;

	Img(const char *path, bool is_alpha);
	~Img(void);

	inline uint32_t sample(uint32_t x, uint32_t y)
	{
		return data[(x & size_mask) * size + (y & size_mask)];
	}
};

}