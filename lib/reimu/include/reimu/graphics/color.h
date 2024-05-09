#pragma once

#include <reimu/graphics/vector.h>

#include <cstdint>

namespace reimu {

union Color final {
	struct {
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
	} __attribute__((packed));
	uint32_t value;

	inline constexpr Color() : value(0) {}
	inline constexpr Color(uint32_t _value) : value(_value) {}
	inline constexpr Color(uint8_t _r, uint8_t _g, uint8_t _b) : r(_r), g(_g), b(_b), a(0xff) {}
	inline constexpr Color(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a) : r(_r), g(_g), b(_b), a(_a) {}

	inline constexpr Vector4f AsFloat() const {
		return { r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f };
	}
};

}
