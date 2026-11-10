#pragma once
#include "dimensions.hpp"
#include "document.hpp"
#include "editortypes.hpp"
#include "textbufferview.hpp"
#include <algorithm>
#include <functional>
#include <string_view>

class DocumentView
{
public:
  using render_able_gutter = std::copyable_function<Renderable_Return(const Renderable_Input &input) const>;

private:
  Col m_col{ 0 };
  Row m_row{ 0 };
  Width m_w{ 1 };
  Height m_h{ 1 };

  TextBufferView m_view;
  Document *m_document;
  std::vector<render_able_gutter> m_left_gutter;
  std::vector<render_able_gutter> m_right_gutter;

  struct Gutters_Cache_Char
  {
    ColOffset left_gutter_size{ 0 };
    ColOffset right_gutter_size{ 0 };
  };

  mutable Gutters_Cache_Char m_gutter_cache;

public:
  constexpr DocumentView(DocumentView &&) = default;
  constexpr DocumentView(const DocumentView &) = delete;
  constexpr DocumentView &operator=(const DocumentView &) = delete;
  constexpr DocumentView &operator=(DocumentView &&) = default;

  explicit DocumentView(Document *doc) : m_document(doc), m_view(&doc->m_buffer) {}

  [[nodiscard]] constexpr Row row() const noexcept { return m_row; }
  [[nodiscard]] constexpr Col col() const noexcept { return m_col; }
  [[nodiscard]] constexpr const Document *document() const noexcept { return m_document; }
  [[nodiscard]] constexpr RowOffset get_first_visible_row() const noexcept { return m_view.row_scroll(); }
  [[nodiscard]] constexpr RowOffset get_last_visible_row() const noexcept
  { return m_view.row_scroll() + m_view.window_rows(); }
  [[nodiscard]] constexpr Width width() const noexcept { return m_w; }
  [[nodiscard]] constexpr Height height() const noexcept { return m_h; }
  [[nodiscard]] constexpr auto &view() noexcept { return m_view; }

  [[nodiscard]] constexpr const auto &view() const noexcept { return m_view; }
  [[nodiscard]] constexpr const ColOffset cursor_col() const
  { return geom::to_distance(m_view.cursor_col_screen()) + m_gutter_cache.left_gutter_size; }


  void constexpr add_left_gutter(render_able_gutter &&gutter)
  { m_left_gutter.emplace_back(std::forward<render_able_gutter>(gutter)); }


  void constexpr calculate_widths(Row row)
  {
    Renderable_Input input = { .view = view(), .row = row, .fmt = OutputFormat::SizeOnly };
    ColOffset left_width{ 0 };
    ColOffset right_width{ 0 };
    for (const auto &rend : m_left_gutter) {
      auto out = rend(input);
      left_width += out.char_written;
    }

    for (const auto &rend : m_right_gutter) {
      auto out = rend(input);
      right_width += out.char_written;
    }

    m_gutter_cache.left_gutter_size = left_width;
    m_gutter_cache.right_gutter_size = right_width;

    auto flex_width = m_w - left_width - right_width;
    m_view.set_window(m_h, flex_width);
  }

  Renderable_Return render(const Renderable_Input &input_data) const
  {
    auto input = input_data;

    ColOffset char_written{ 0 };
    Renderable_Return outpt{ .text_buffer = {}, .color_buffer = {}, .char_written = 0 };
    for (auto const &rend : m_left_gutter) {
      auto outpt = rend(input);
      char_written += outpt.char_written;
      input.update(outpt);
    }

    {
      auto outpt = render_row_of_document(input);
      char_written += outpt.char_written;
      input.update(outpt);
    }


    for (auto const &rend : m_right_gutter) {
      auto outpt = rend(input);
      char_written += outpt.char_written;

      input.update(outpt);
    }

    return { .text_buffer = input.text_buffer, .color_buffer = input.color_buffer, .char_written = char_written };
  }


  // [[nodiscard]] constexpr Col get_buffer_row() const noexcept { return origin(m_left_gutter_border +
  // m_left.width());
  // };

  void set_position(Col x, Row y, Width w, Height h)
  // pre(w > 1) pre(h > 1) pre(x >= 0) pre(y >= 0)
  {
    m_col = x;
    m_row = y;
    m_w = w;
    m_h = h;
    m_view.set_window(h, w);
  }

  [[nodiscard]] constexpr std::pair<Row, Col> cursor_position() const

  {
    using geom::to_distance, geom::to_distance;
    auto col = to_distance(m_view.cursor_col_screen()) + to_distance(m_col) + m_gutter_cache.left_gutter_size;
    auto row = to_distance(m_row) + to_distance(m_view.cursor_row_screen());
    return { to_position(row), to_position(col) };
  }

  static Renderable_Return render_row_of_document(Renderable_Input &input)
  {

    const auto row_string = input.view.get_row(input.row);
    const auto col_scroll = input.view.col_scroll();
    const auto len = std::clamp(
      ColOffset{ static_cast<int>(std::ssize(row_string)) } - col_scroll, ColOffset{ 0 }, input.view.window_cols());
    switch (input.fmt) {
      using enum OutputFormat;
    case SizeOnly:
      return { .char_written = len };
    case ASCII:
      // Write characters to the buffer

      if (std::ssize(row_string) > col_scroll.underlying()) {
        memcpy(input.text_buffer.data(),
          row_string.subview(col_scroll.underlying(), len.underlying()).data(),
          len.underlying());
        return {
          .text_buffer = { input.text_buffer.begin() + len.underlying(), input.text_buffer.end() },
          .color_buffer = input.color_buffer,
          .char_written = len,
        };
      }
      // Fall through if not writing anything

    default:
      return { .text_buffer = input.text_buffer, .color_buffer = input.color_buffer, .char_written = 0 };
    }
  };
};
