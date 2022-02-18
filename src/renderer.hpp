#pragma once

#include "stb.hpp"
#include <cstdint>
#include <vector>

struct ivec2 {
	int32_t x;
	int32_t y;

	using own = ivec2;

	inline ivec2(int32_t x, int32_t y) :
		x(x),
		y(y)
	{
	}

	inline auto operator-(const own &other)
	{
		return own(x - other.x, y - other.y);
	}

	inline auto operator+(const own &other)
	{
		return own(x + other.x, y + other.y);
	}

	inline void operator=(const own &other)
	{
		x = other.x;
		y = other.y;
	}

	inline void operator-=(const own &other)
	{
		*this = *this - other;
	}

	inline void operator+=(const own &other)
	{
		*this = *this + other;
	}
};

inline int32_t min(int32_t a, int32_t b)
{
	return a < b ? a : b;
}

inline int32_t max(int32_t a, int32_t b)
{
	return a > b ? a : b;
}

struct Wall {
	ivec2 a;
	ivec2 b;
	int32_t ele_low;
	int32_t ele_up;
};

class Renderer
{
	uint32_t *m_fb;
	uint32_t m_w;
	uint32_t m_h;
	int32_t m_wh;
	int32_t m_hh;
	int32_t m_wm;
	int32_t m_hm;

	std::vector<Wall> walls;

	stb::Img t0;

public:
	Renderer(uint32_t *fb, uint32_t w, uint32_t h) :
		m_fb(fb),
		m_w(w),
		m_h(h),
		m_wh(m_w / 2),
		m_hh(m_h / 2),
		m_wm(m_w - 1),
		m_hm(m_h - 1),
		t0("res/t0.png", false)
	{
		walls.emplace_back(Wall{
			ivec2(-500, 500),
			ivec2(2000, 3000),
			-800,
			200
		});
		walls.emplace_back(Wall{
			ivec2(-2000, 3000),
			ivec2(500, 500) + ivec2(-1200, 0),
			-800,
			200
		});
	}

	int32_t proj_x(const ivec2 &p)
	{
		return (p.x * m_hh) / p.y + m_wh;
	}

	int32_t proj_y(const ivec2 &p, int32_t ele)
	{
		return (ele * m_hh) / p.y + m_hh;
	}

	int32_t lerp_x(int32_t a, int32_t b, int32_t scale, int32_t x)
	{
		return a * (scale - 1 - x) / scale + b * x / scale;
	}

	void render(ivec2 camp, int32_t camele)
	{
		std::memset(m_fb, 0, m_w * m_h * sizeof(uint32_t));
		for (auto w : walls) {
			w.a -= camp;
			w.b -= camp;
			w.ele_low -= camele;
			w.ele_up -= camele;

			if (w.a.y <= 0 || w.b.y <= 0)
				continue;

			int32_t l = max(proj_x(w.a), 0);
			int32_t r = min(proj_x(w.b), m_wm);
			if (l > m_wm || r < 0 || l >= r)
				continue;
			int32_t rl = r - l;

			int32_t ta = proj_y(w.a, w.ele_low);
			int32_t ba = proj_y(w.a, w.ele_up);
			int32_t tb = proj_y(w.b, w.ele_low);
			int32_t bb = proj_y(w.b, w.ele_up);

			for (int32_t i = l; i < r; i++) {
				auto col = m_fb + i * m_h;
				int32_t t = max(lerp_x(ta, tb, rl, i - l), 0);
				int32_t b = min(lerp_x(ba, bb, rl, i - l), m_hm);
				for (int32_t j = t; j < b; j++)
					col[j] = t0.sample(i, j);
			}
		}
	}
};