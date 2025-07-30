#include "webgpu.h"

#include <reimu/core/logger.h>

namespace reimu::graphics::webgpu {

WGPUTextureFormat convert_color_format(ColorFormat fmt) {
    switch (fmt) {
    case ColorFormat::RGBA8:
        return WGPUTextureFormat_RGBA8Unorm;
    }

    logger::fatal("Unsupported color format");
}

WGPUShaderStage convert_shader_stage(ShaderStage stage) {
    switch (stage) {
    case ShaderStage::Vertex:
        return WGPUShaderStage_Vertex;
    case ShaderStage::Fragment:
        return WGPUShaderStage_Fragment;
    }

    logger::fatal("Unsupported shader stage");
}

}
