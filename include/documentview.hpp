#pragma once
#include "document.hpp"
#include "textbufferview.hpp"

struct DocumentView {

  // Ows the view but views are moveable and can be allocated within the vector
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

  TextBufferView &current_TextBufferView() {
    return views[active_view_index].view;
  }
};
