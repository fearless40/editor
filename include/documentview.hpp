#pragma once
#include "digitcount.hpp"
#include "dimensions.hpp"
#include "document.hpp"
#include "editortypes.hpp"
#include "textbufferview.hpp"
#include "xy.hpp"
#include <algorithm>
#include <cstddef>
#include <functional>
#include <string_view>


struct FixedWidthTextBuffer
{
  using uint = unsigned int;
  std::vector<std::byte> buff;
  geom::Width line_width{ 1 };
  geom::Height line_count{ 1 };

  // constexpr auto line_view()
  // {
  //   return std::views::chunk(buff, line_width.underlying())
  //          | std::views::transform([](auto chunk) { return std::string_view(chunk); });
  // }
  //
  // constexpr auto line_view_index() { return std::views::enumerate(line_view()); }

  void ensure_size(Width new_width, Height nbr_lines)
  {
    auto max_size = new_width * nbr_lines;
    if (max_size.underlying() > buff.capacity()) { buff.reserve(max_size.underlying()); }
    line_width = new_width;
    line_count = nbr_lines;
  }

  std::string_view view_at(Row row)
  {
    auto *ptr = buff.data();
    // Move ptr by number of rows;
    ptr += static_cast<ptrdiff_t>(row.underlying() * line_width.underlying());
    return std::string_view{ (char *)ptr, (char *)ptr + line_width.underlying() };
  }

  std::span<std::byte> span_at(Row row)
  {
    auto ptrPostion = row.underlying() * line_width.underlying();
    return std::span<std::byte>{ buff.data() + ptrPostion, static_cast<std::size_t>(line_width.underlying()) };
  }
};

struct ByteColorData
{
  unsigned int start;
  unsigned int end;
};


enum class OutputFormat : uint8_t { SizeOnly, ASCII, UTF8, UTF16, UTF32 };
struct renderable_input
{
  std::span<std::byte> text_buffer;
  std::span<ByteColorData> color_buffer;
  const TextBufferView &view;
  Row row;
  OutputFormat fmt;
};


struct renderable_return
{
  // New span points at valid range.
  std::span<std::byte> text_buffer;
  std::span<ByteColorData> color_buffer;
  ColOffset written;// Number of characters written. In ASCII one to one ration, in UTF8 a character can be represetned
                    // by multiple ways
};


struct LeftGutter2
{
  geom::Width min_width{ 3 };
  geom::Width max_width{ 5 };
  geom::Width right_border{ 1 };


  // Code to show a raw view
  renderable_return operator()(renderable_input &input) const
  {
    static constexpr std::string_view empty{ "                       " };
    auto max_row = input.view.window_rows().underlying() + input.view.row_scroll().underlying();
    auto digitCount = util::fast_digit_count(max_row);
    if (input.fmt == OutputFormat::SizeOnly) { return { .written = digitCount + right_border.underlying() }; }

    // This only ever outputs ASCII for now. Other formats pretty easy to add later

    auto start_line_nbr = input.view.row_scroll();
    auto end_line_nbr = input.view.window_rows() + start_line_nbr;
    auto request_width = std::clamp(digitCount, min_width.underlying(), max_width.underlying());

    auto iterator = input.text_buffer.begin();
    std::size_t last_char_size = 0;
    // Generate space
    memcpy(&*iterator, empty.data(), request_width - digitCount);

    // Move the iterator
    iterator += request_width - digitCount;


    // Do integer conversion in place
    auto conres = std::to_chars((char *)&*(iterator), (char *)&*input.text_buffer.end(), input.row.underlying(), 10);

    if (conres) {
      iterator += digitCount;
      std::memcpy(&*iterator, empty.data(), right_border.underlying());
      iterator += right_border.underlying();
    }

    return {
      // clang-format off
      .text_buffer = { iterator, input.text_buffer.end() },
      .color_buffer = input.color_buffer,
      .written = ColOffset
      { static_cast<typename ColOffset::underlying_t>(std::distance(input.text_buffer.begin(), iterator)) }
      // clang-format on
    };
  }
};


struct ThemeIndex
{
  unsigned int idx;
};

struct ColorData
{
  Col start;
  ColOffset end;
  ThemeIndex index;
};


class DocumentView
{
public:
  using Height = geom::Height;
  using Width = geom::Width;
  using X = geom::X;
  using Y = geom::Y;

  using render_able_gutter = std::copyable_function<renderable_return(renderable_input &input)>;

private:
  X m_x{ 0 };
  Y m_y{ 0 };
  Width m_w{ 1 };
  Height m_h{ 1 };


  TextBufferView m_view;

  // Non owning pointer
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
  [[nodiscard]] constexpr auto view() noexcept { return m_view; }


  std::span<std::byte>
    render_row_absolute(Row row, std::span<std::byte> buffer, OutputFormat fmt = OutputFormat::ASCII) const
  {

    // Check if row < headers.height()
    // render(headers)

    // Check if row > footers.start()
    // render(footers)

    // Otherwise render row

    // leftgutters -> buffer -> rightgutters
    auto input = renderable_input{ .text_buffer = buffer, .color_buffer = {}, .view = m_view, .row = row, .fmt = fmt };
    for (const auto &render : m_left_gutters) {
      auto ret = render(input);
      input.text_buffer = ret.text_buffer;
      input.color_buffer = ret.color_buffer;
    }

    // Determine number of characters written and adjust interal view size only for ASCII for now
    m_gutter_cache.left_gutter_size = ColOffset{ (int)std::distance(buffer.begin(), input.text_buffer.begin()) };
    // m_gutter_cache.right_gutter_size = std::distance(buffer.begin(), input.text_buffer.begin());

    // Going to ignore right gutters for now

    const auto row_string = m_view.get_row(row);
    const auto col_scroll = m_view.col_scroll();
    const auto len =
      std::clamp(ColOffset{ (int)std::ssize(row_string) } - col_scroll, ColOffset{ 0 }, m_view.window_cols());
    if (std::ssize(row_string) > col_scroll.underlying()) {
      // Write characters to the buffer
      if (fmt == OutputFormat::ASCII) {
        memcpy(input.text_buffer.data(),
          row_string.subview(col_scroll.underlying(), len.underlying()).data(),
          len.underlying());
      };
    }
  }

  FixedWidthTextBuffer &flatten(FixedWidthTextBuffer &buffer) {}

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
