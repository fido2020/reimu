#pragma once

#include <reimu/graphics/texture.h>
#include <reimu/graphics/render_pass.h>

#include <webgpu/webgpu.h>

namespace reimu::graphics::webgpu {

WGPUTextureFormat convert_color_format(ColorFormat fmt);
WGPUShaderStage convert_shader_stage(ShaderStage stage);

consteval WGPUStringView to_sv(const char *string) {
    return {string, std::string_view(string).size()};
}

}
