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

	inline constexpr ColorRGBA8 operator*(const ColorRGBA8 &other) const {
		// Upcast to uint16_t
		uint16_t alpha = other.a;
		uint16_t one_minus = 255 - alpha;

		// Split up red-blue, alpha-green
		uint32_t rb = (value & 0x00ff00ff) * one_minus + (other.value & 0x00ff00ff) * alpha;
		uint32_t ag = ((value & 0xff00ff00) >> 8) * one_minus + ((other.value & 0xff00ff00) >> 8) * alpha;

		return ((rb & 0xff00ff00) >> 8) | ((ag & 0xff00ff00));
	}

	inline static constexpr ColorRGBA8 white() {
		return { 255, 255, 255, 255 };
	}

	inline static constexpr ColorRGBA8 black() {
		return { 0, 0, 0, 255 };
	}

	inline static constexpr ColorRGBA8 red() {
		return { 255, 0, 0, 255 };
	}

	inline static constexpr ColorRGBA8 green() {
		return { 0, 255, 0, 255 };
	}

	inline static constexpr ColorRGBA8 blue() {
		return { 0, 0, 255, 255 };
	}

	inline static constexpr ColorRGBA8 transparent() {
		return { 0, 0, 0, 0 };
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
