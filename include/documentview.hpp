#pragma once
#include "digitcount.hpp"
#include "dimensions.hpp"
#include "document.hpp"
#include "editortypes.hpp"
#include "fixedwidthtextbuffer.hpp"
#include "textbufferview.hpp"
#include "xy.hpp"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <string_view>


class DocumentView
{
public:
  using Height = geom::Height;
  using Width = geom::Width;
  using X = geom::X;
  using Y = geom::Y;

  using render_able_gutter = std::copyable_function<Renderable_Return(const Renderable_Input &input) const>;

private:
  X m_x{ 0 };
  Y m_y{ 0 };
  Width m_w{ 1 };
  Height m_h{ 1 };

  TextBufferView m_view;
  Document *m_document;
  std::vector<render_able_gutter> m_left_gutters;

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

  [[nodiscard]] constexpr Col get_buffer_col() const noexcept { return origin(m_gutter_cache.left_gutter_size); };

  [[nodiscard]] constexpr RowOffset get_first_visible_row() const noexcept { return m_view.row_scroll(); }
  [[nodiscard]] constexpr RowOffset get_last_visible_row() const noexcept
  { return m_view.row_scroll() + m_view.window_rows(); }
  [[nodiscard]] constexpr Width width() const noexcept { return m_w; }
  [[nodiscard]] constexpr Height height() const noexcept { return m_h; }
  [[nodiscard]] constexpr auto &view() noexcept { return m_view; }

  void constexpr add_left_gutter(render_able_gutter &&gutter)
  { m_left_gutters.emplace_back(std::forward<render_able_gutter>(gutter)); }


  std::span<std::byte>
    render_row_absolute(Row row, std::span<std::byte> buffer, OutputFormat fmt = OutputFormat::ASCII) const
  {

    // Check if row < headers.height()
    // render(headers)

    // Check if row > footers.start()
    // render(footers)

    // Otherwise render row

    // leftgutters -> buffer -> rightgutters
    auto input = Renderable_Input{ .text_buffer = buffer, .color_buffer = {}, .view = m_view, .row = row, .fmt = fmt };
    auto sum_characters = ColOffset{ 0 };
    for (const auto &render : m_left_gutters) {
      auto ret = render(input);
      input.text_buffer = ret.text_buffer;
      input.color_buffer = ret.color_buffer;
      sum_characters += ret.written;
    }

    // Determine number of characters written and adjust interal view size only for ASCII for now
    m_gutter_cache.left_gutter_size =
      sum_characters;// ColOffset{ (int)std::distance(buffer.begin(), input.text_buffer.begin()) };
    // m_gutter_cache.right_gutter_size = std::distance(buffer.begin(), input.text_buffer.begin());

    // Going to ignore right gutters for now

    const auto row_string = m_view.get_row(row);
    const auto col_scroll = m_view.col_scroll();
    const auto len = std::clamp(
      ColOffset{ static_cast<int>(std::ssize(row_string)) } - col_scroll, ColOffset{ 0 }, m_view.window_cols());
    if (std::ssize(row_string) > col_scroll.underlying()) {
      // Write characters to the buffer
      if (fmt == OutputFormat::ASCII) {
        memcpy(input.text_buffer.data(),
          row_string.subview(col_scroll.underlying(), len.underlying()).data(),
          len.underlying());
      };
    }
    return { buffer.begin(), buffer.begin() + m_gutter_cache.left_gutter_size.underlying() + len.underlying() };
  }


  // [[nodiscard]] constexpr Col get_buffer_row() const noexcept { return origin(m_left_gutter_border +
  // m_left.width());
  // };

  void set_position(X x, Y y, Width w, Height h)
  // pre(w > 1) pre(h > 1) pre(x >= 0) pre(y >= 0)
  {
    m_x = x;
    m_y = y;
    m_w = w;
    m_h = h;
    m_view.set_window(h, w);
  }

  [[nodiscard]] constexpr std::pair<Row, Col> cursor_position() const

  {
    auto cx = geom::origin(m_view.cursor_col_screen()) + geom::origin(m_x) + m_gutter_cache.left_gutter_size;
    auto cy = geom::origin(m_y) + geom::origin(m_view.cursor_row_screen());
    return { geom::origin(cy), geom::origin(cx) };
  }
};

Renderable_Return render_row_of_document(Renderable_Input &input)
{
  const auto row_string = input.view.get_row(input.row);
  const auto col_scroll = input.view.col_scroll();
  const auto len = std::clamp(
    ColOffset{ static_cast<int>(std::ssize(row_string)) } - col_scroll, ColOffset{ 0 }, input.view.window_cols());
  if (std::ssize(row_string) > col_scroll.underlying()) {
    // Write characters to the buffer
    if (input.fmt == OutputFormat::ASCII) {
      memcpy(input.text_buffer.data(),
        row_string.subview(col_scroll.underlying(), len.underlying()).data(),
        len.underlying());
    };
  }
  return {
    .text_buffer = { input.text_buffer.begin(), input.text_buffer.begin() + len.underlying() },
    .color_buffer = input.color_buffer,
    .written = len,
  };
};
