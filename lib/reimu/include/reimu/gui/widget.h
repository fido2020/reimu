#pragma once

#include <reimu/graphics/renderer.h>
#include <reimu/graphics/surface.h>
#include <reimu/graphics/vector.h>
#include <reimu/gui/layout.h>
#include <reimu/gui/style.h>

#include <functional>
#include <memory>

namespace reimu::gui {

using CreateTexture = std::function<graphics::Texture *(const Vector2i &)>;
using AddClipFn = std::function<void(const Rectf &, const Vector2f &, graphics::Texture *)>;

class Widget {
public:
    Widget(CreateTexture create_texture);
    virtual ~Widget();

    void set_parent(Widget *parent);
    virtual void remove_child(Widget *child);

    virtual void update_layout();
    virtual void repaint(UIPainter &painter);

    /**
     * Add clips to the render queue.
     * 
     * @param add_clip Function to add a clip to the render queue.
     * add_clip takes in a source rectangle, a destination position and a texture.
    */
    virtual void add_clips(AddClipFn add_clip);

    Rectf bounds;

    LayoutProperties layout;
    CalculatedLayout calculated_layout;

protected:
    CreateTexture m_create_texture_fn;
    Widget *m_parent = nullptr;

    std::unique_ptr<graphics::Surface> m_surface;
};

class Box : public Widget {
public:
    Box(CreateTexture create_texture);

    void update_layout() override;

    void repaint(UIPainter &painter) override;

    void add_child(Widget *child);
    void remove_child(Widget *child) override;

    void add_clips(
            std::function<void(const Rectf &, const Vector2f &, graphics::Texture *)> add_clip)
            override;

private:
    std::list<Widget *> m_children;
};

class Button : public Widget {
public:
    Button(CreateTexture create_texture);

    void repaint(UIPainter &painter) override;

    std::string label;
    bool is_pressed = false;
};

class RootContainer : public Box {
public:
    RootContainer(CreateTexture create_texture, const Vector2f &viewport_size);

    void update_layout(const Vector2f &viewport_size);

private:
    Vector2f m_viewport_size;
};

}
