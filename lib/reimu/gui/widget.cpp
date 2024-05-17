#include <reimu/gui/widget.h>

#include <reimu/core/logger.h>

namespace reimu::gui {

Widget::Widget(CreateTexture create_texture)
    : m_create_texture_fn(create_texture) {
}

Widget::~Widget() {
    if (m_parent) {
        m_parent->remove_child(this);
    }
}

void Widget::set_parent(Widget *parent) {
    m_parent = parent;
}

void Widget::remove_child(Widget *child) {
    logger::fatal("Invalid Widget::remove_child");
}

void Widget::update_layout() {

}

void Widget::repaint(UIPainter &painter) {
    // Default is to draw nothing, but resize the texture if needed
    if (m_surface) {
        auto wanted_size = vector_static_cast<int>(bounds.size());
        if (m_surface->size() != wanted_size) {
            m_surface->resize(wanted_size);
        }
    }
}

void Widget::add_clips(std::function<void(const Rectf &, const Vector2f &, graphics::Texture *)> add_clip) {
    // If there is a texture, add a clip
    if (m_surface) {
        auto tex_size = vector_static_cast<float>(m_surface->size());
        Rectf source_rect = { 0, 0, tex_size.x, tex_size.y };

        add_clip(source_rect, bounds.top_left(), &m_surface->texture());
    }
}

Box::Box(CreateTexture create_texture)
    : Widget(create_texture) {
    m_surface = std::make_unique<graphics::Surface>(m_create_texture_fn({ 1, 1 }));
}

void Box::update_layout() {
    Widget::update_layout();

    float combined_width = 0;
    float combined_height = 0;

    // Update the layout of all children
    for (Widget *child : m_children) {
        // Update the layout for each child
        child->layout.calculate_layout(child->calculated_layout, &calculated_layout);
        logger::debug("Calculated inner size: {} {}", child->calculated_layout.inner_size.x, child->calculated_layout.inner_size.y);
    
        // Get the size the child is requesting
        auto child_size = child->calculated_layout.outer_size;

        combined_width += child_size.x;
        combined_height += child_size.y;
    }

    float x_pos = bounds.x;
    float y_pos = bounds.y;

    // Now we decide how we want to lay out the children
    if (layout.layout_direction == LayoutDirection::Vertical) {
        for (auto *child : m_children) {
            auto inner_size = child->calculated_layout.inner_size;

            float widget_x = x_pos + child->calculated_layout.left_padding;
            float widget_y = y_pos + child->calculated_layout.right_padding;

            Rectf bounds = {
                widget_x,
                widget_y,
                widget_x + inner_size.x,
                widget_y + inner_size.y
            };

            child->bounds = bounds;

            y_pos += child->calculated_layout.outer_size.y;

            child->update_layout();
        }
    } else {
        for (auto *child : m_children) {
            auto inner_size = child->calculated_layout.inner_size;

            float widget_x = x_pos + child->calculated_layout.left_padding;
            float widget_y = y_pos + child->calculated_layout.top_padding;

            Rectf bounds = {
                widget_x,
                widget_y,
                widget_x + inner_size.x,
                widget_y + inner_size.y
            };

            logger::debug("x_pos: {} y_pos: {} inner_size.x: {} inner_size.y: {}", x_pos, y_pos, inner_size.x, inner_size.y);

            child->bounds = bounds;

            x_pos += child->calculated_layout.outer_size.x;

            child->update_layout();
        }
    }
}

void Box::repaint(UIPainter &painter) {
    Widget::repaint(painter);

    graphics::Painter p(*m_surface);
    p.draw_rect({ 0, 0, bounds.width(), bounds.height() }, { 255, 0, 128, 255 });

    // Draw the children
    for (Widget *child : m_children) {
        child->repaint(painter);
    }
}

void Box::add_child(Widget *child) {
    m_children.push_back(child);

    child->set_parent(this);
}

void Box::remove_child(Widget *child) {
    m_children.remove(child);
}

void Box::add_clips(
        std::function<void(const Rectf &, const Vector2f &, graphics::Texture *)> add_clip) {
    Widget::add_clips(add_clip);

    for (Widget *child : m_children) {
        child->add_clips(add_clip);
    }
}

}
