#pragma once

#include "stb.hpp"
#include <cstdint>

class Renderer
{
	uint32_t *m_fb;
	uint32_t m_w;
	uint32_t m_h;

	stb::Img t0;

public:
	Renderer(uint32_t *fb, uint32_t w, uint32_t h) :
		m_fb(fb),
		m_w(w),
		m_h(h),
		t0("res/t0.png", false)
	{
	}

	void render(void)
	{
		for (uint32_t i = 0; i < m_w; i++) {
			auto col = m_fb + i * m_h;
			for (uint32_t j = 0; j < m_h; j++)
				col[j] = t0.sample(i, j);
		}
	}
};