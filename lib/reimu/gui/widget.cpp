#include <reimu/gui/widget.h>

#include <reimu/core/logger.h>

#include <assert.h>

namespace reimu::gui {

Widget::Widget() {
    bind_event_callback("ui_repaint"_hashid, [this]() {
        if (m_parent) {
            m_parent->dispatch_event("ui_repaint"_hashid);
        }
    });
}

Widget::~Widget() {
    if (m_parent) {
        m_parent->remove_child(this);
    }
}

void Widget::set_parent(Widget *parent) {
    m_parent = parent;
    
    if (parent) {
        m_window = parent->m_window;
    } else {
        m_window = nullptr;
    }
}

Widget *Widget::get_widget_at(const Vector2f &pos) {
    return this;
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
            if (wanted_size.x <= 0 || wanted_size.y <= 0) {
                wanted_size = { 1, 1 };
            }

            m_surface->resize(wanted_size);
        }
    }
}

void Widget::add_clips(AddClipFn add_clip) {
    // If there is a texture, add a clip
    if (m_surface) {
        auto tex_size = m_surface->size();
        Recti source_rect = { 0, 0, tex_size.x, tex_size.y };

        add_clip(source_rect, vector_static_cast<int>(bounds.top_left()), &m_surface->texture());
    }
}

void Widget::signal_layout_changed() {
    if (m_parent) {
        m_parent->signal_layout_changed();
    }
}

void Widget::create_texture_if_needed(CreateTextureFn fn) {
    if (!m_surface) {
        auto tex_size = vector_static_cast<int>(bounds.size());
        if (tex_size.x <= 0 || tex_size.y <= 0) {
            tex_size = { 1, 1 };
        }

        m_surface = std::make_unique<graphics::Surface>(fn(tex_size));
    }
}

Widget *Box::get_widget_at(const Vector2f &pos) {
    for (Widget *child : m_children) {
        if (child->bounds.contains(pos)) {
            return child->get_widget_at(pos);
        }
    }

    return this;
}

void Box::repaint(UIPainter &painter) {
    Widget::repaint(painter);

    graphics::Painter p(*m_surface);
    painter.begin(p);
    painter.draw_background();
    painter.end();

    // Draw the children
    for (Widget *child : m_children) {
        child->repaint(painter);
    }
}

void Box::create_texture_if_needed(CreateTextureFn fn) {
    m_create_texture_fn = fn;

    Widget::create_texture_if_needed(fn);

    for (Widget *child : m_children) {
        child->create_texture_if_needed(fn);
    }
}

void Box::add_child(Widget *child) {
    signal_layout_changed();

    m_children.push_back(child);

    child->set_parent(this);

    if (m_create_texture_fn) {
        child->create_texture_if_needed(m_create_texture_fn);
    }
}

void Box::remove_child(Widget *child) {
    m_children.remove(child);
}

void Box::add_clips(AddClipFn add_clip) {
    Widget::add_clips(add_clip);

    for (Widget *child : m_children) {
        child->add_clips(add_clip);
    }
}

Rectf Box::inner_bounds() const {
    return bounds;
}

void FlowBox::add_child(Widget *child) {
    Box::add_child(child);
}

void FlowBox::update_layout() {
    Widget::update_layout();

    auto bounds = inner_bounds();

    float combined_width = 0;
    float combined_height = 0;

    // Update the layout of all children
    for (Widget *child : m_children) {
        // Update the layout for each child
        child->layout.calculate_layout(child->calculated_layout, &calculated_layout);
    
        if (child->layout.position == LayoutPositioning::Absolute) {
            continue;
        }

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
            if (child->layout.position == LayoutPositioning::Absolute) {
                child->bounds =
                    {0, 0, child->calculated_layout.inner_size.x, child->calculated_layout.inner_size.y};
                child->update_layout();
                continue;
            }

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
            if (child->layout.position == LayoutPositioning::Absolute) {
                child->bounds =
                    {0, 0, child->calculated_layout.inner_size.x, child->calculated_layout.inner_size.y};
                child->update_layout();
                continue;
            }

            auto inner_size = child->calculated_layout.inner_size;

            float widget_x = x_pos + child->calculated_layout.left_padding;
            float widget_y = y_pos + child->calculated_layout.top_padding;

            Rectf bounds = {
                widget_x,
                widget_y,
                widget_x + inner_size.x,
                widget_y + inner_size.y
            };

            child->bounds = bounds;

            x_pos += child->calculated_layout.outer_size.x;

            child->update_layout();
        }
    }
}

GridBox::GridBox(std::vector<Row> grid)
    : m_grid(std::move(grid)) {}

void GridBox::add_item(Widget *widget, const Size &column_size) {
    assert(!m_grid.empty());

    m_grid.back().items.push_back({ widget, column_size });

    add_child(widget);
}

void GridBox::add_row(const Size &size) {
    m_grid.push_back({ size, {} });
}

void GridBox::update_layout() {
    Widget::update_layout();

    auto bounds = inner_bounds();

    // Add the widths and heights of the rows and columns,
    // and add up the 'factors' which are used to calculate what proportion of the available
    // space each row and column should take up
    
    // e.g. If there is an item with a factor of 1 and another of a factor of 2,
    // the first item will take up 1/3 of the space and the second will take up 2/3 of the space

    float combined_height = 0;
    float combined_y_factor = 0;

    std::vector<float> combined_widths;
    std::vector<float> combined_x_factors;

    for (auto &row : m_grid) {
        if (row.size.unit == LayoutUnit::LayoutFactor) {
            combined_y_factor += row.size.value;
        } else {
            combined_height += row.size.as_pixels(bounds.width(), calculated_layout);
        }

        float combined_width = 0;
        float combined_x_factor = 0;

        // Update the layout for each child whilst doing the calculations
        for (auto &item : row.items) {
            auto *child = item.widget;
            child->layout.calculate_layout(child->calculated_layout, &calculated_layout);

            if (item.column_size.unit == LayoutUnit::LayoutFactor) {
                combined_x_factor += item.column_size.value;
            } else {
                combined_width += item.column_size.as_pixels(bounds.width(), calculated_layout);
            }
        }

        combined_widths.push_back(combined_width);
        combined_x_factors.push_back(combined_x_factor);
    }

    float x_pos = bounds.x;
    float y_pos = bounds.y;

    // Now we decide how we want to lay out the children
    for (size_t y = 0; y < m_grid.size(); y++) {
        // Get the rows items and calculate its height in pixels
        auto &items = m_grid[y].items;
        auto row_size = m_grid[y].size;

        float row_height;
        if (row_size.unit == LayoutUnit::LayoutFactor) {
            // Calculate the height of the row based on the available space
            row_height = row_size.value / combined_y_factor * (bounds.height() - combined_height);
        } else {
            row_height = row_size.as_pixels(bounds.height(), calculated_layout);
        }

        float space = bounds.width() - combined_widths[y];
        float factor = combined_x_factors[y];

        for (auto &item : items) {
            float column_width = 0;
            if (item.column_size.unit == LayoutUnit::LayoutFactor) {
                // Calculate the width of the column based on the available space
                column_width = item.column_size.value / factor * space;
            } else {
                column_width = item.column_size.as_pixels(bounds.width(), calculated_layout);
            }

            auto *child = item.widget;
            const auto &child_layout = child->calculated_layout;

            float widget_x = x_pos + child_layout.left_padding;
            float widget_y = y_pos + child_layout.top_padding;

            float widget_x2 = x_pos + column_width - child_layout.left_padding - child_layout.right_padding;
            float widget_y2 = widget_y + row_height - child_layout.top_padding - child_layout.bottom_padding;

            Rectf bounds = {
                widget_x,
                widget_y,
                widget_x2,
                widget_y2
            };

            child->bounds = bounds;
            child->update_layout();

            x_pos += column_width;
        }

        x_pos = bounds.x;
        y_pos += row_height;
    }
}

}
