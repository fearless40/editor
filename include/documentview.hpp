#pragma once
#include "document.hpp"
#include "rect.hpp"
#include "textbufferview.hpp"
#include "xy.hpp"
#include <cstddef>
#include <ranges>

struct RightGutter
{
};

struct Header
{
};

struct Footer
{
};

struct FixedWidthTextBuffer
{
  using uint = unsigned int;
  std::vector<char> buff;
  geom::Width line_width{ 1 };
  geom::Height line_count{ 1 };

  constexpr auto line_view()
  {
    return std::views::chunk(buff, line_width.underlying())
           | std::views::transform([](auto chunk) { return std::string_view(chunk); });
  }

  constexpr auto line_view_index() { return std::views::enumerate(line_view()); }

  void ensure_size(geom::Width new_width, geom::Height nbr_lines)
  {
    auto max_size = new_width * nbr_lines;
    if (max_size.underlying() > buff.capacity()) { buff.reserve(max_size.underlying()); }
    line_width = new_width;
    line_count = nbr_lines;
  }

  // std::span<char> line(unsigned int linenbr)
  // {
  //   if (linenbr > line_count) { return { &buff[(line_count - 1) * line_width], line_width }; }
  //   return { &buff[linenbr * line_width], line_width };
  // }
};

struct LeftGutter
{
  geom::Width request_width{ 4 };
  geom::Width min_width{ 4 };
  geom::Width max_width{ 5 };
  FixedWidthTextBuffer buff;

  [[nodiscard]] constexpr geom::Width width() const { return request_width; }

  constexpr geom::Width update_width(const TextBufferView &view)
  {
    char temp[12];
    int start_line_nbr = view.row_scroll().to<int>();
    int end_line_nbr = view.window_rows().to<int>() + start_line_nbr;
    auto count = std::to_chars(temp, &temp[11], end_line_nbr, 10);
    auto nbr_chars = geom::Width::make(std::distance(temp, count.ptr));
    request_width = std::clamp(nbr_chars, min_width, max_width);

    buff.ensure_size(request_width, geom::Height{ end_line_nbr - start_line_nbr });

    return request_width;
  };

  // Code to show a raw view
  FixedWidthTextBuffer &raw_view(const TextBufferView &view)
  {
    char temp[12];
    int start_line_nbr = view.row_scroll().to<int>();
    int end_line_nbr = view.window_rows().underlying() + start_line_nbr;
    std::size_t last_char_size = 0;
    buff.buff.clear();
    for (auto i = start_line_nbr; i < end_line_nbr; ++i) {
      // Could be slow using push_back for everything however will use it for
      // now;
      auto conres = std::to_chars(temp, &temp[11], i, 10);
      auto nbrChar = std::distance(temp, conres.ptr);
      for (std::size_t spaceIndex = 0; spaceIndex < request_width.underlying() - nbrChar; ++spaceIndex) {
        buff.buff.push_back(' ');
      }
      for (auto c : std::span<char>(temp, nbrChar)) { buff.buff.push_back(c); }
    }

    return buff;
  }
};

using Rect = geom::TypedRect<geom::Width, geom::Height>;


struct DocumentView
{
  using Height = geom::Height;
  using Width = geom::Width;
  using X = geom::X;
  using Y = geom::Y;

  Header m_header;
  LeftGutter m_left;
  RightGutter m_right;
  Footer m_foot;

  X m_x{ 0 };
  Y m_y{ 0 };
  Width m_w{ 1 };
  Height m_h{ 1 };


  geom::Height m_header_border{ 0 };
  geom::Width m_left_gutter_border{ 1 };
  geom::Width m_right_gutter_border{ 1 };
  geom::Height m_footer_border{ 0 };

  bool m_header_visible{ false };
  bool m_left_gutter_visible{ false };
  bool m_right_gutter_visible{ false };
  bool m_footer_visible{ false };


  // Ows the view but views are moveable and can be allocated within the
  // vector
  TextBufferView m_view;

  // Non owning pointer
  Document *document;

  explicit DocumentView(Document *doc) : document(doc), m_view(doc->m_buffer) {}

  void set_position(X x, Y y, Width w, Height h)
  // pre(w > 1) pre(h > 1) pre(x >= 0) pre(y >= 0)
  {
    m_x = x;
    m_y = y;
    m_w = w;
    m_h = h;
    m_view.set_window(h, w - m_left.width() - m_left_gutter_border);
  }

  [[nodiscard]] constexpr std::pair<X, Y> cursor_position() const
  {
    auto cx = X{ m_view.cursor_row_screen().to<X::underlying_t>() + m_x.underlying() + m_left_gutter_border.underlying()
                 + m_left.width().underlying() };
    auto cy = Y{ m_y.underlying() + m_view.cursor_col_screen().to<Y::underlying_t>() };
    return { cx, cy };
  }

  [[nodiscard]] constexpr auto left_gutter() -> FixedWidthTextBuffer &
  {
    m_left.update_width(m_view);
    return m_left.raw_view(m_view);
  }
};

struct DocumentViewManager
{
  std::vector<DocumentView> views;
  std::size_t active_view_index{ 0 };

  void set_active_view(std::size_t index)
  {
    if (index < views.size()) active_view_index = index;
  }

  void create_view(Document *doc, std::size_t row_width, std::size_t col_width)
  {
    views.emplace_back(doc);
    auto &d = views.back();
    d.m_view.set_window(geom::Height::make(row_width), geom::Width::make(col_width));
    active_view_index = std::distance(views.begin(), views.end() - 1);
  }

  constexpr TextBufferView &current_view() { return views[active_view_index].m_view; }

  constexpr DocumentView &current_document() { return views[active_view_index]; }
};
