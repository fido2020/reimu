#pragma once

#include <reimu/core/event.h>
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

class Widget : public EventDispatcher {
public:
    Widget();
    virtual ~Widget();

    void set_parent(Widget *parent);

    /**
     * @brief Get the widget at a given position.
     * 
     * Returns a pointer to itself or a child located at 'pos'.
     * 
     * @param pos 
     * @return Widget* 
     */
    virtual Widget *get_widget_at(const Vector2f &pos);

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
    virtual void remove_child(Widget *child);

    Widget *m_parent = nullptr;
    class Window *m_window = nullptr;

    std::unique_ptr<graphics::Surface> m_surface = nullptr;
};

class Box : public Widget {
public:
    ~Box();

    Widget *get_widget_at(const Vector2f &pos) override;

    virtual void repaint(UIPainter &painter) override;

    void add_clips(AddClipFn add_clip) override;
    void create_texture_if_needed(CreateTextureFn fn) override;

protected:
    virtual void add_child(Widget *child);
    virtual void remove_child(Widget *child) override;

    virtual Rectf inner_bounds() const;

    CreateTextureFn m_create_texture_fn;
    std::list<Widget *> m_children;
};

class FlowBox : public Box {
public:
    void add_child(Widget *child) override;

    void update_layout() override;
};

class GridBox : public Box {
public:
    struct Item {
        Widget *widget;
        Size column_size;
    };

    struct Row {
        Size size;
        std::vector<Item> items;
    };

    GridBox() = default;
    GridBox(std::vector<Row> grid);

    void add_item(Widget *widget, const Size &column_size);
    void add_row(const Size &size);

    void update_layout() override;

private:
    std::vector<Row> m_grid;
};

class Button : public Widget {
public:
    Button();
    void repaint(UIPainter &painter) override;

    std::string label;
    bool is_pressed = false;
};

class RootContainer : public FlowBox {
public:
    RootContainer(class Window *win, const Vector2f &viewport_size,
        bool decorate = true);

    void repaint(UIPainter &painter) override;
    void update_layout(const Vector2f &viewport_size);

    void signal_layout_changed() override;

    inline bool needs_layout_update() const {
        return m_recalculate_layout;
    }

    inline bool needs_repaint() const {
        return m_repaint;
    }

    inline void set_is_decorated(bool decorate) {
        if (m_decorate == decorate) {
            return;
        }

        m_decorate = decorate;
        m_recalculate_layout = true;
        
        create_window_controls();
    }

    std::string window_title = "れいむ";

private:
    Rectf inner_bounds() const override;
    void create_window_controls();
    
    std::list<std::unique_ptr<Widget>> m_window_controls;

    Vector2f m_viewport_size;
    bool m_recalculate_layout = true;
    bool m_repaint = true;
    bool m_decorate = true;
};

class Label : public Widget {
public:
    void repaint(UIPainter &painter) override;

    void set_text(const std::string &text);

private:
    std::string m_text;
};

class TextBox : public Widget {
public:
    TextBox();
    void repaint(UIPainter &painter) override;

    void set_text(std::string text);
    std::string_view get_text() const;

private:
    std::string m_text;
};

}
