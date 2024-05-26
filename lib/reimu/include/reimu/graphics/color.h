#pragma once

#include <reimu/graphics/vector.h>

#include <cstdint>

namespace reimu {

union ColorRGBA8 final {
	struct {
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
	} __attribute__((packed));
	uint32_t value;

	inline constexpr ColorRGBA8() : value(0) {}
	inline constexpr ColorRGBA8(uint32_t _value) : value(_value) {}
	inline constexpr ColorRGBA8(uint8_t _r, uint8_t _g, uint8_t _b) : r(_r), g(_g), b(_b), a(0xff) {}
	inline constexpr ColorRGBA8(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a) : r(_r), g(_g), b(_b), a(_a) {}

	inline constexpr Vector4i as_8bit() const {
		return { r, g, b, a };
	}

	inline constexpr Vector4f as_float() const {
		return { r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f };
	}
};

union ColorRGBA16 final {
	struct {
		uint16_t r;
		uint16_t g;
		uint16_t b;
		uint16_t a;
	} __attribute__((packed));
	uint64_t value;

	inline constexpr ColorRGBA16() : value(0) {}
	
	inline explicit constexpr ColorRGBA16(const ColorRGBA8 &c) {
		value = c.a;
		value = (value << 16) | c.b;
		value = (value << 16) | c.g;
		value = (value << 16) | c.r;

		value |= (value << 8);
	}

	inline constexpr ColorRGBA16(uint64_t _value) : value(_value) {}
	inline constexpr ColorRGBA16(uint16_t _r, uint16_t _g, uint16_t _b)
		: r(_r), g(_g), b(_b), a(0xffff) {}
	inline constexpr ColorRGBA16(uint16_t _r, uint16_t _g, uint16_t _b, uint16_t _a)
		: r(_r), g(_g), b(_b), a(_a) {}
	
	inline constexpr uint32_t to_rgba8() const {
		return ((value >> 8) & 0xff) | ((value >> 16) & 0xff00) | ((value >> 24) & 0xff0000)
			| ((value >> 32) & 0xff000000);
	}

	inline constexpr Vector4f as_float() const {
		return { r / 65535.0f, g / 65535.0f, b / 65535.0f, a / 65535.0f };
	}
};

using Color = ColorRGBA8;

}
