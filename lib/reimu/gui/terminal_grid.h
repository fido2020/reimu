#pragma once

#include <reimu/core/optional.h>
#include <reimu/graphics/painter.h>

#include <assert.h>
#include <stdint.h>

#include <vector>

namespace reimu::term {

struct CellFlags {
    // Invert the colors of the cell
    bool inverted : 1 = false;
    // Cell is part of a wrapped line
    bool wrapped : 1 = false;
};

class Cell {
public:
    Cell() : m_ch(UINT32_MAX), m_fg_color(0), m_bg_color(0) {}

    Cell(uint32_t ch, uint32_t fg_color, uint32_t bg_color)
        : m_ch(ch), m_fg_color(fg_color), m_bg_color(bg_color) {}

    uint32_t get_ch() const { return m_ch; }
    uint32_t get_fg_color() const { return m_fg_color; }
    uint32_t get_bg_color() const { return m_bg_color; }

    CellFlags get_flags() const { return m_flags; }
    CellFlags set_flags(CellFlags flags) { m_flags = flags; }

private:
    uint32_t m_ch;
    uint32_t m_fg_color;
    uint32_t m_bg_color;

    CellFlags m_flags;
};

class Grid {
public:
    Grid(int num_visible_rows, int row_size)
        : m_num_visible_rows(num_visible_rows), m_row_size(row_size) {

        m_cells.resize(num_visible_rows);
        for (int i = 0; i < num_visible_rows; i++) {
            m_cells[i].resize(row_size);
        }
    }

    Optional<Cell> get_cell_at(int row, int col) const {
        if (row < m_num_visible_rows && col < m_cells[row].size()) {
            return OPT_SOME(m_cells[row][col]);
        }

        return OPT_NONE;
    }

    void put_cell_at_cursor(Cell cell) {
        m_cells[m_cur_y][m_cur_x] = std::move(cell);
        
        m_cur_x++;
        if (m_cur_x >= m_row_size) {
            // TODO: terminal wrapping here
            // (set wrapped flag to true on the last cell probably?)

            m_cur_x = 0;
            next_row();
        }
    }

    void move_cursor(int x, int y) {
        m_cur_x += x;
        m_cur_y += y;

        m_cur_x = std::clamp(m_cur_x, 0, m_row_size - 1);
        m_cur_y = std::clamp(m_cur_y, 0, m_num_visible_rows - 1);
    }

    void set_cursor(int x, int y) {
        x = std::clamp(x, 0, m_row_size - 1);
        y = std::clamp(y, 0, m_num_visible_rows - 1);

        m_cur_x = x;
        m_cur_y = y;
    }

    Vector2i get_cursor() const {
        return {m_cur_x, m_cur_y};
    }

    void erase_display(const Vector2i &start, const Vector2i &end) {
        for (int i = start.y; i <= end.y; i++) {
            if (i == start.y) {
                erase_line(i, start.x, m_row_size - 1);
            } else if (i == end.y - 1) {
                erase_line(i, 0, end.x);
            } else {
                erase_line(i, 0, m_row_size - 1);
            }
        }
    }

    void erase_line(int row, int start, int end) {
        assert(row < m_num_visible_rows);
        assert(start <= end);
        assert(end < m_row_size && start >= 0);

        for (int i = start; i <= end; i++) {
            m_cells[row][i] = Cell();
        }
    }

    void resize(int x, int y) {
        m_num_visible_rows = y;
        m_row_size = x;

        if (m_cells.size() < y) {
            m_cells.resize(y);
        }

        for (int i = 0; i < y; i++) {
            // TODO: wrap lines here instead of cutting stuff off
            m_cells[i].resize(x);
        }

        if (m_cur_x >= x) {
            m_cur_x = x - 1;
        }

        if (m_cur_y >= y) {
            m_cur_y = y - 1;
        }
    }

    template<typename DrawCellFn>
    void paint(DrawCellFn &draw_cell_fn) {
        assert(m_num_visible_rows <= m_cells.size());

        for (int i = 0; i < m_cells.size(); i++) {
            for (int j = 0; j < m_row_size; j++) {
                const auto &cell = m_cells[i][j];

                draw_cell_fn(cell.get_ch(), i, j, cell.get_fg_color(), cell.get_bg_color());
            }
        }

        // Draw cursor
        draw_cell_fn(' ', m_cur_y, m_cur_x, 0x000000, 0xFFFFFF);
    }

    void carriage_return() {
        m_cur_x = 0;
    }

    void next_row() {
        assert(m_cur_y <= m_num_visible_rows - 1);

        if (m_cur_y < m_num_visible_rows - 1) {
            m_cur_y++;
        } else {
            if (m_cells.size() >= m_num_visible_rows) {
                for (int i = 0; i < m_num_visible_rows - 1; i++) {
                    m_cells[i] = std::move(m_cells[i + 1]);
                }

                m_cells[m_num_visible_rows - 1].clear();
                m_cells[m_num_visible_rows - 1].resize(m_row_size);
            } else {
                m_cells.push_back(std::vector<Cell>(m_row_size));
            }
        }
    }

    int get_num_visible_rows() const { return m_num_visible_rows; }
    int get_row_size() const { return m_row_size; }

private:
    std::vector<std::vector<Cell>> m_cells;

    int m_cur_x = 0;
    int m_cur_y = 0;

    // The number of rows that are visible on the screen
    int m_num_visible_rows;
    // Size of each row in cells (we do not allow rows to go off the screen)
    int m_row_size;
};

}
