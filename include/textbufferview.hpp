#pragma once
#include "textbuffer.hpp"
#include "typed_scalar.hpp"
#include "xy.hpp"
#include <algorithm>
#include <cstddef>
#include <string_view>

enum class RowSize : std::size_t {};
enum class ColSize : std::size_t {};

namespace detail {
struct Cursor_Row_Tag
{
};
struct Cursor_Col_Tag
{
};
};// namespace detail

using CursorRow = geom::TypedScalar<std::size_t, detail::Cursor_Row_Tag>;
using CursorCol = geom::TypedScalar<std::size_t, detail::Cursor_Col_Tag>;

class TextBufferView
{
  using Height = geom::Height;
  using Width = geom::Width;
  TextBuffer &view;

  Height m_screen_rows{ 0 };
  Width m_screen_cols{ 0 };

  // Cursors are the actual value into the file not a visual value
  CursorRow m_cursor_row{ 0 };
  CursorCol m_cursor_col{ 0 };

  // Rowoffset is a value into the file
  CursorRow row_offset{ 0 };
  CursorCol col_offset{ 0 };

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

  [[nodiscard]] constexpr CursorRow cursor_row_file() const { return m_cursor_row; }

  [[nodiscard]] constexpr CursorCol cursor_col_file() const { return m_cursor_col; }

  [[nodiscard]] constexpr CursorRow cursor_row_screen() const { return (m_cursor_row - row_offset); }

  [[nodiscard]] constexpr CursorCol cursor_col_screen() const { return (m_cursor_col - col_offset); }

  [[nodiscard]] constexpr Height window_rows() const { return m_screen_rows; }

  [[nodiscard]] constexpr Width window_cols() const { return m_screen_cols; }

  [[nodiscard]] constexpr TextBuffer &buffer() const { return view; }

  [[nodiscard]] constexpr CursorRow row_scroll() const { return row_offset; }
  [[nodiscard]] constexpr CursorCol col_scroll() const { return col_offset; }

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
      view.insert_row("", m_cursor_row + 1);
    } else {

      const auto end_post = std::min(value.length(), m_cursor_col.underlying());

      if (end_post == value.length()) {
        view.insert_row("", m_cursor_row + 1);
      } else {
        const std::string_view subview = value.subview(end_post);
        view.insert_row(subview, m_cursor_row + 1);
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
      view.remove_char(m_cursor_row, m_cursor_col - 1);
      --m_cursor_col;
    } else {

      auto str_o = view.get_row(m_cursor_row);
      if (!str_o) { return; }

      auto &string = str_o.value();

      // if (string.length() == 0) {
      //   view.remove_row(cursor_row);
      //   adjust_cursor_row(-1);
      // } else {
      m_cursor_col = CursorCol::make(view.line_length(m_cursor_row - 1));
      view.append_row(string, m_cursor_row - 1);
      view.remove_row(m_cursor_row);
      adjust_cursor_row(-1);
      // }
    }
    validate_cursor_position();
  }

  constexpr void up(unsigned int amt)
  {
    adjust_cursor_row(-(long)amt);
    validate_cursor_position();
  }

  constexpr void down(unsigned int amt)
  {
    adjust_cursor_row((long)(amt));

    validate_cursor_position();
  }

  constexpr void line_home()
  {
    m_cursor_col = CursorCol{ 0 };
    validate_cursor_position();
  };
  constexpr void line_end()
  {
    m_cursor_col = CursorCol::make(view.line_length(m_cursor_row));
    validate_cursor_position();
  }

  constexpr void left(unsigned int amt)
  {
    auto ccol = m_cursor_col.to<long>();
    ccol -= amt;
    if (ccol < 0) {
      adjust_cursor_row(-1);
      m_cursor_col = CursorCol{ view.line_length(m_cursor_row) };
    } else {
      m_cursor_col = CursorCol::make(ccol);
    }
    validate_cursor_position();
  };

  constexpr void right(unsigned int amt)
  {
    m_cursor_col += CursorCol::make(amt);
    if (auto len = view.line_length(m_cursor_row); m_cursor_col > len) {
      adjust_cursor_row(1);
      m_cursor_col = CursorCol{ 0 };
    }

    // Todo: consider putting in wrapping so if you advance by 10 characters
    // and you have a line length of 5 cursor would be on character 5 of the
    // next line.

    validate_cursor_position();
  }

private:
  constexpr void adjust_cursor_row(long amount)
  {

    if (view.empty()) {
      m_cursor_row = CursorRow{ 0 };
      return;
    }

    auto nrow = std::clamp(amount + m_cursor_row.to<long>(), 0L, static_cast<long>(view.size()) - 1);
    m_cursor_row = CursorRow::make(nrow);
  }
  constexpr void do_scroll()
  {

    if (m_cursor_row >= row_offset + m_screen_rows.underlying()) {
      row_offset = CursorRow::make(m_cursor_row - m_screen_rows.underlying() + 1);
      view_scrolled_rows = true;
    } else if (m_cursor_row < row_offset) {
      row_offset = m_cursor_row;
      view_scrolled_rows = true;
    } else
      view_scrolled_rows = false;

    if (m_cursor_col >= col_offset + m_screen_cols.underlying()) {
      col_offset = CursorCol::make(m_cursor_col - m_screen_cols.underlying() + 1);
      view_scrolled_cols = true;
    } else if (m_cursor_col < col_offset) {
      col_offset = m_cursor_col;
      view_scrolled_cols = true;
    } else
      view_scrolled_cols = false;
  }

  constexpr void validate_cursor_position()
  {
    auto ncur = std::clamp(m_cursor_col.to<long>(), 0L, (long)view.line_length(m_cursor_row));
    m_cursor_col = CursorCol::make(ncur);

    do_scroll();
  }
};
