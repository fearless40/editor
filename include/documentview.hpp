#pragma once
#include "document.hpp"
#include "textbufferview.hpp"
#include <ranges>

struct RightGutter {};

struct Header {};

struct Footer {};

struct FixedWidthTextBuffer {
  std::vector<char> buff;
  unsigned int line_width{1};
  unsigned int line_count{1};

  constexpr auto line_view() {
    return std::views::chunk(buff, line_width) |
           std::views::transform(
               [](auto chunk) { return std::string_view(chunk); });
  }

  constexpr auto line_view_index() {
    return std::views::enumerate(line_view());
  }

  void ensure_size(unsigned int new_width, unsigned int nbr_lines) {
    std::size_t max_size = new_width * nbr_lines;
    if (max_size > buff.capacity()) {
      buff.reserve(max_size);
    }
    line_width = new_width;
    line_count = nbr_lines;
  }

  std::span<char> line(unsigned int linenbr) {
    if (linenbr > line_count) {
      return {&buff[(line_count - 1) * line_width], line_width};
    }
    return {&buff[linenbr * line_width], line_width};
  }
};

struct LeftGutter {
  unsigned int request_width;
  unsigned int min_width{4};
  unsigned int max_width{5};
  unsigned int right_border_width;
  FixedWidthTextBuffer buff;

  constexpr unsigned int update_width(const TextBufferView &view) {
    char temp[12];
    int start_line_nbr = std::to_underlying(view.row_scroll());
    int end_line_nbr = std::to_underlying(view.window_rows()) + start_line_nbr;
    auto count = std::to_chars(temp, &temp[11], end_line_nbr, 10);
    auto nbr_chars = std::distance(temp, count.ptr);
    request_width = std::clamp((unsigned int)nbr_chars, min_width, max_width);

    buff.ensure_size(request_width, end_line_nbr - start_line_nbr);

    return request_width;
  };

  // Code to show a raw view
  const FixedWidthTextBuffer &raw_view(const TextBufferView &view) {
    char temp[12];
    int start_line_nbr = std::to_underlying(view.row_scroll());
    int end_line_nbr = std::to_underlying(view.window_rows()) + start_line_nbr;
    std::size_t last_char_size = 0;
    buff.buff.clear();
    for (auto i = start_line_nbr; i < end_line_nbr; ++i) {
      // Could be slow using push_back for everything however will use it for
      // now;
      auto conres = std::to_chars(temp, &temp[11], i, 10);
      auto nbrChar = std::distance(temp, conres.ptr);
      for (std::size_t spaceIndex = 0; spaceIndex < request_width - nbrChar;
           ++spaceIndex) {
        buff.buff.push_back(' ');
      }
      for (auto c : std::span<char>(temp, nbrChar)) {
        buff.buff.push_back(c);
      }
    }

    return buff;
  }

  // color_map colors();

  constexpr unsigned int total_width() const {
    return min_width + right_border_width;
  }
};

struct DocumentView {
  // Ows the view but views are moveable and can be allocated within the
  // vector
  TextBufferView view;

  // Non owning pointer
  Document *document;

  DocumentView(Document *doc) : document(doc), view(doc->m_buffer) {}
};

struct DocumentViewManager {
  std::vector<DocumentView> views;
  std::size_t active_view_index{0};

  void set_active_view(std::size_t index) {
    if (index < views.size())
      active_view_index = index;
  }

  void create_view(Document *doc, std::size_t row_width,
                   std::size_t col_width) {
    views.emplace_back(doc);
    auto &d = views.back();
    d.view.set_window(RowSize{row_width}, ColSize{col_width});
    active_view_index = std::distance(views.begin(), views.end() - 1);
  }

  constexpr TextBufferView &current_view() {
    return views[active_view_index].view;
  }

  constexpr DocumentView &current_document() {
    return views[active_view_index];
  }
};
