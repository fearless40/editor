#pragma once
#include "textbuffer.hpp"

enum class RowSize : std::size_t {};
enum class ColSize : std::size_t {};

class TextBufferView {
  TextBuffer &view;

  std::size_t screen_rows{0};
  std::size_t screen_cols{0};
  long cursor_row{0};
  long cursor_col{0};
  std::size_t row_offset{0};
  std::size_t col_offset{0};
  bool view_scrolled_rows{true};
  bool view_scrolled_cols{true};

public:
  TextBufferView(TextBuffer &buffer) : view(buffer) {};

  constexpr void set_window(RowSize rows, ColSize cols) {
    screen_rows = std::to_underlying(rows);
    screen_cols = std::to_underlying(cols);
  }

  bool did_view_scroll_rows() const { return view_scrolled_rows; }

  bool did_view_scroll_cols() const { return view_scrolled_cols; }

  constexpr long crow() const { return (long)(cursor_row - row_offset); }

  constexpr long ccol() const { return (long)(cursor_col - col_offset); }

  constexpr RowSize window_rows() const { return RowSize{screen_rows}; }

  constexpr ColSize window_cols() const { return ColSize{screen_cols}; }

  constexpr TextBuffer buffer() const { return view; }

  constexpr RowSize row_scroll() const { return RowSize{row_offset}; }
  constexpr ColSize col_scroll() const { return ColSize{col_offset}; }

  constexpr void insert_char_at_cursor(char c) {
    view.insert_char(cursor_row, cursor_col, c);
    cursor_col++;
  }

  constexpr void insert_enter() {
    if (cursor_col == view.line_length(cursor_row)) {
      view.append_row("");
    } else {
      auto str_o = view.get_row(cursor_row);
      if (!str_o)
        return;

      auto subview = str_o.value().subview(cursor_col);
      view.insert_row(subview, cursor_row + 1);
      view.modify_row(str_o.value().subview(0, cursor_col - 1), cursor_row);
      adjust_cursor_row(1);
    }
  }

  constexpr void delete_char_to_right() {

    if (cursor_col == view.line_length(cursor_row) and
        cursor_row != view.number_rows()) {
      auto str = view.get_row(cursor_row + 1);
      if (!str)
        return;
      auto end_col = view.line_length(cursor_row);
      view.append_row(str.value(), cursor_row);
      view.remove_row(cursor_row + 1);

      cursor_col = end_col;
    } else
      view.remove_char(cursor_row, cursor_col);

    validate_cursor_position();
  }

  constexpr void delete_char_to_left() {
    if (cursor_col == 0) {
      auto str = view.get_row(cursor_row);
      if (!str)
        return;
      auto end_col = view.line_length(cursor_row - 1);
      view.append_row(str.value(), cursor_row - 1);
      view.remove_row(cursor_row);
      adjust_cursor_row(-1);
      cursor_col = end_col;
    } else {
      view.remove_char(cursor_row, cursor_col - 1);
      cursor_col--;
    }
    validate_cursor_position();
  }

  constexpr void up(unsigned int amt) {
    adjust_cursor_row(-(long)amt);
    validate_cursor_position();
  }

  constexpr void down(unsigned int amt) {
    adjust_cursor_row((long)(amt));

    validate_cursor_position();
  }

  constexpr void line_home() {
    cursor_col = 0;
    validate_cursor_position();
  };
  constexpr void line_end() {
    cursor_col = view.line_length(cursor_row);
    validate_cursor_position();
  }

  constexpr void left(unsigned int amt) {
    cursor_col -= (long)amt;
    if (cursor_col < 0) {
      adjust_cursor_row(-1);
      cursor_col = view.line_length(cursor_row);
    }
    validate_cursor_position();
  };

  constexpr void right(unsigned int amt) {
    cursor_col += (long)amt;
    if (auto len = view.line_length(cursor_row); cursor_col > len) {
      adjust_cursor_row(1);
      cursor_col = 0;
    }

    // Todo: consider putting in wrapping so if you advance by 10 characters
    // and you have a line length of 5 cursor would be on character 5 of the
    // next line.

    validate_cursor_position();
  }

private:
  constexpr void adjust_cursor_row(long amount) {
    cursor_row =
        std::max(0l, std::min(cursor_row + amount, (long)view.number_rows()));
  }
  constexpr void do_scroll() {

    if (cursor_row >= row_offset + screen_rows) {
      row_offset = cursor_row - screen_rows + 1;
      view_scrolled_rows = true;
    } else if (cursor_row < row_offset) {
      row_offset = cursor_row;
      view_scrolled_rows = true;
    } else
      view_scrolled_rows = false;

    if (cursor_col >= col_offset + screen_cols) {
      col_offset = cursor_col - screen_cols + 1;
      view_scrolled_cols = true;
    } else if (cursor_col < col_offset) {
      col_offset = cursor_col;
      view_scrolled_cols = true;
    } else
      view_scrolled_cols = false;
  }

  constexpr void validate_cursor_position() {
    if (cursor_col < 0)
      cursor_col = 0;

    if (auto len = view.line_length(cursor_row); cursor_col >= len) {
      cursor_col = len;
    }

    do_scroll();
  }
};
