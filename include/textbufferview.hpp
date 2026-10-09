#pragma once
#include "textbuffer.hpp"
#include "xy.hpp"
#include <algorithm>
#include <iterator>
#include <string_view>

class TextBufferView
{

  using Row = geom::Row;
  using Col = geom::Col;
  using RowOffset = geom::Height;
  using ColOffset = geom::Width;
  using Height = geom::Height;
  using Width = geom::Width;
  TextBuffer &view;

  Height m_screen_rows{ 0 };
  Width m_screen_cols{ 0 };

  // Cursors are the actual value into the file not a visual value
  Row m_cursor_row{ 0 };
  Col m_cursor_col{ 0 };

  // Rowoffset is a value into the file
  RowOffset m_row_offset{ 0 };
  ColOffset m_col_offset{ 0 };

  bool view_scrolled_rows{ true };
  bool view_scrolled_cols{ true };

public:
  explicit TextBufferView(TextBuffer &buffer) : view(buffer) {};

  void set_view(TextBuffer &buffer) { view = buffer; }

  constexpr void set_window(Height rows, Width cols)
  {
    m_screen_rows = rows;
    m_screen_cols = cols;
  }

  [[nodiscard]] bool did_view_scroll_rows() const { return view_scrolled_rows; }

  [[nodiscard]] bool did_view_scroll_cols() const { return view_scrolled_cols; }

  [[nodiscard]] constexpr Row cursor_row_file() const { return m_cursor_row; }

  [[nodiscard]] constexpr Col cursor_col_file() const { return m_cursor_col; }

  [[nodiscard]] constexpr Row cursor_row_screen() const { return m_cursor_row - m_row_offset; }

  [[nodiscard]] constexpr Col cursor_col_screen() const { return m_cursor_col - m_col_offset; }

  [[nodiscard]] constexpr Height window_rows() const { return m_screen_rows; }

  [[nodiscard]] constexpr Width window_cols() const { return m_screen_cols; }

  [[nodiscard]] constexpr TextBuffer &buffer() const { return view; }

  [[nodiscard]] constexpr RowOffset row_scroll() const { return m_row_offset; }
  [[nodiscard]] constexpr ColOffset col_scroll() const { return m_col_offset; }

  constexpr void insert_char_at_cursor(char c)
  {
    view.insert_char(m_cursor_row.underlying(), m_cursor_col.underlying(), c);
    ++m_cursor_col;
  }

  constexpr void insert_enter()
  {
    auto str_o = view.get_row(m_cursor_row.underlying());
    if (!str_o) { return; }

    auto &value = str_o.value();
    if (value.length() == 0) {
      view.insert_row("", ++m_cursor_row);
    } else {

      const auto end_post = std::min(std::ssize(value), (long)m_cursor_col.underlying());

      if (end_post == value.length()) {
        view.insert_row("", ++m_cursor_row);
      } else {
        const std::string_view subview = value.subview(end_post);
        view.insert_row(subview, m_cursor_row + RowOffset{ 1 });
        view.modify_row(value.subview(0, end_post), m_cursor_row);
      }
    }
    adjust_cursor_row(1);
    validate_cursor_position();
  }

  constexpr void delete_char_to_right()
  {
    right(1);
    delete_char_to_left();
  }

  constexpr void delete_char_to_left()
  {
    if (m_cursor_col == 0 and m_cursor_row == 0) return;

    if (m_cursor_col > 0) {
      view.remove_char(m_cursor_row, m_cursor_col - ColOffset{ 1 });
      --m_cursor_col;
    } else {

      auto str_o = view.get_row(m_cursor_row);
      if (!str_o) { return; }

      auto &string = str_o.value();

      // if (string.length() == 0) {
      //   view.remove_row(cursor_row);
      //   adjust_cursor_row(-1);
      // } else {
      m_cursor_col = static_cast<Col::underlying_t>(view.line_length(m_cursor_row - ColOffset{ 1 }));
      view.append_row(string, m_cursor_row - RowOffset{ 1 });
      view.remove_row(m_cursor_row);
      adjust_cursor_row(-1);
      // }
    }
    validate_cursor_position();
  }

  constexpr void up(RowOffset amt)
  {
    adjust_cursor_row(-amt);
    validate_cursor_position();
  }

  constexpr void down(RowOffset amt)
  {
    adjust_cursor_row(amt);

    validate_cursor_position();
  }

  constexpr void line_home()
  {
    m_cursor_col = Col{ 0 };
    validate_cursor_position();
  };
  constexpr void line_end()
  {
    m_cursor_col = static_cast<Col::underlying_t>(view.line_length(m_cursor_row));
    validate_cursor_position();
  }

  constexpr void left(ColOffset amt)
  {
    m_cursor_col = m_cursor_col - amt;
    if (m_cursor_col < 0) {
      adjust_cursor_row(-1);
      m_cursor_col = static_cast<Col::underlying_t>(view.line_length(m_cursor_row));
    }

    validate_cursor_position();
  };

  constexpr void right(ColOffset amt)
  {
    m_cursor_col = m_cursor_col + amt;
    if (auto len = static_cast<Col::underlying_t>(view.line_length(m_cursor_row)); m_cursor_col > len) {
      adjust_cursor_row(1);
      m_cursor_col = Col{ 0 };
    }

    // Todo: consider putting in wrapping so if you advance by 10 characters
    // and you have a line length of 5 cursor would be on character 5 of the
    // next line.

    validate_cursor_position();
  }

private:
  constexpr void adjust_cursor_row(RowOffset amount)
  {

    if (view.empty()) {
      m_cursor_row = 0;
      return;
    }

    m_cursor_row = std::clamp(amount + m_cursor_row, Row{ 0 }, Row{ static_cast<Row::underlying_t>(view.size()) - 1 });
  }
  constexpr void do_scroll()
  {

    auto cursor_row_distance_from_origin = m_cursor_row - Row{ 0 };
    if (cursor_row_distance_from_origin >= m_row_offset + m_screen_rows) {
      m_row_offset = cursor_row_distance_from_origin - m_screen_rows + RowOffset{ 1 };
      view_scrolled_rows = true;
    } else if (cursor_row_distance_from_origin < m_row_offset) {
      m_row_offset = cursor_row_distance_from_origin;
      view_scrolled_rows = true;
    } else {
      view_scrolled_rows = false;
    }

    auto cursor_col_distance_from_origin = m_cursor_col - Col{ 0 };
    if (cursor_col_distance_from_origin >= m_col_offset + m_screen_cols) {
      m_col_offset = cursor_col_distance_from_origin - m_screen_cols + ColOffset{ 1 };
      view_scrolled_cols = true;
    } else if (cursor_col_distance_from_origin < m_col_offset) {
      m_col_offset = cursor_col_distance_from_origin;
      view_scrolled_cols = true;
    } else {
      view_scrolled_cols = false;
    }
  }

  constexpr void validate_cursor_position()
  {
    m_cursor_col =
      std::clamp(m_cursor_col, Col{ 0 }, Col{ static_cast<Col::underlying_t>(view.line_length(m_cursor_row)) });
    do_scroll();
  }
};
