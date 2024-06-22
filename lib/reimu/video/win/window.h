#pragma once

#include <reimu/video/window.h>

#include <windows.h>

class Win32Window : public reimu::video::Window {
public:
    Win32Window(const reimu::Vector2i &size) {
        this->size = size;
    }

    ~Win32Window() {
        DestroyWindow(handle);
    }

    void set_size(const reimu::Vector2i &size) override {
        
    }

    void set_title(const std::string &title) override {}

    reimu::Vector2i get_size() const override {
        return size;
    }

    void render() override {
        if (m_renderer) {
            m_renderer->render();
        }
    }

    void show_window() override {

    }

    void hide_window() override {

    }

    void sync_window() override {

    }

    void begin_move() override {
        reimu::logger::debug("begin move");
    }

    reimu::Result<reimu::video::NativeWindowHandle *, reimu::ReimuError> get_native_handle()
            override {
        return OK(new reimu::video::NativeWindowHandle {
                reimu::video::NativeHandleType::Win32, {
                .win32 = {
                    .hinstance = hinstance,
                    .hwnd = handle
                }
            }
        });
    }

    reimu::Vector2i size;
    HWND handle;
    HINSTANCE hinstance;
};
