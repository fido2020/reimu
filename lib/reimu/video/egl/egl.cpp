#include "egl.h"

#include <EGL/eglext.h>

namespace reimu::video {

DEF_SIMPLE_ERROR(EGLDisplayError, "Failed to get EGL display");
DEF_SIMPLE_ERROR(EGLSurfaceError, "Failed to get EGL surface");

Result<EGLInstance *, ErrorBox> EGLInstance::create(EGLNativeDisplayType display) {
    // Get an EGL display corresponding to the system display
    EGLDisplay egl_display = eglGetPlatformDisplay(EGL_PLATFORM_WAYLAND_KHR, display, NULL);
    if (egl_display == EGL_NO_DISPLAY) {
        return ERR(EGLDisplayError{});
    }

    EGLint egl_minor, egl_major;

    // Request an OpenGL ES2 surface with at least 8 bits per color
    const EGLint egl_attrib_list[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_NONE
    };

	const EGLint egl_context_attrib_list[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

    if (eglInitialize(egl_display, &egl_major, &egl_minor) != EGL_TRUE) {
        reimu::logger::fatal("Failed to initialize EGL");
    }

    if(eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE) {
        reimu::logger::fatal("Failed to bind EGL API");
    }

    auto *ctx = new EGLInstance;
    ctx->m_display = egl_display;

    EGLint num_configs;
    if (eglChooseConfig(egl_display, egl_attrib_list, ctx->m_egl_configs, 1/*NUM_EGL_CONFIGS*/,
            &num_configs) != EGL_TRUE || num_configs < 1) {
        reimu::logger::fatal("Failed to get EGL configs");
    }

    int buffer_size;
    eglGetConfigAttrib(egl_display,
				   ctx->m_egl_configs[0], EGL_BUFFER_SIZE, &buffer_size);
    reimu::logger::debug("Using config w/ buffer size of {}", buffer_size);

    ctx->m_ctx = eglCreateContext(egl_display, ctx->m_egl_configs[0], EGL_NO_CONTEXT, egl_context_attrib_list);
    if (ctx->m_ctx == EGL_NO_CONTEXT) {
        reimu::logger::fatal("Failed to get EGL context 0x{:x}", eglGetError());
    }

    reimu::logger::debug("{} EGL configs found!", num_configs);

    return OK(ctx);
}

Result<EGLSurface, ErrorBox> EGLInstance::create_surface(EGLNativeWindowType window) {
    EGLConfig config = m_egl_configs[0];

    EGLSurface surface = eglCreateWindowSurface(m_display, config, window, NULL);
    if (surface == EGL_NO_SURFACE) {
        return ERR(EGLSurfaceError{});
    }

    return OK(surface);
}

}
