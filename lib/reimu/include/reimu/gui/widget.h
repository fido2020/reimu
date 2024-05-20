#pragma once

#include <reimu/graphics/renderer.h>
#include <reimu/graphics/surface.h>
#include <reimu/graphics/vector.h>
#include <reimu/gui/layout.h>
#include <reimu/gui/style.h>

#include <functional>
#include <memory>

namespace reimu::gui {

using CreateTextureFn = std::function<graphics::Texture *(const Vector2i &)>;
using AddClipFn = std::function<void(const Recti &, const Vector2i &, graphics::Texture *)>;

class Widget {
public:
    Widget();
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

    /**
     * @brief Signal to the widget and its parents that the layout has changed.
     */
    virtual void signal_layout_changed();

    virtual void create_texture_if_needed(CreateTextureFn fn);

    Rectf bounds;

    LayoutProperties layout;
    CalculatedLayout calculated_layout;

protected:
    Widget *m_parent = nullptr;

    std::unique_ptr<graphics::Surface> m_surface;
};

class Box : public Widget {
public:
    void update_layout() override;

    void repaint(UIPainter &painter) override;
    void remove_child(Widget *child) override;
    void add_clips(AddClipFn add_clip) override;
    void create_texture_if_needed(CreateTextureFn fn) override;

    virtual void add_child(Widget *child);

private:
    CreateTextureFn m_create_texture_fn;
    std::list<Widget *> m_children;
};

class Button : public Widget {
public:
    void repaint(UIPainter &painter) override;

    std::string label;
    bool is_pressed = false;
};

class RootContainer : public Box {
public:
    RootContainer(const Vector2f &viewport_size);

    void update_layout(const Vector2f &viewport_size);

    void signal_layout_changed() override;
    inline bool needs_layout_update() const {
        return m_recalculate_layout;
    }

private:
    Vector2f m_viewport_size;
    bool m_recalculate_layout = true;
};

}
