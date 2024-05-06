#pragma once

#include <wayland-client.h>
#include <wayland-egl.h>

// This is NASTY (EGLNativeDisplayType will change depending on the include order)
#include <EGL/egl.h>

#include <reimu/core/error.h>
#include <reimu/core/result.h>
#include <reimu/core/util.h>

namespace reimu::video {

class EGLInstance {
public:
    static Result<reimu::video::EGLInstance *, ErrorBox> create(EGLNativeDisplayType display);

    Result<EGLSurface, ErrorBox> create_surface(EGLNativeWindowType window);

    ALWAYS_INLINE EGLDisplay get_display() {
        return m_display;
    }

    ALWAYS_INLINE EGLContext get_context() {
        return m_ctx;
    }

private:
    static constexpr int NUM_EGL_CONFIGS = 32;
    EGLConfig m_egl_configs[NUM_EGL_CONFIGS];

    EGLDisplay m_display;
    EGLContext m_ctx;
};

}