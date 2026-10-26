#pragma once
#include "documentview.hpp"
#include "keymap.hpp"
#include "soa.hpp"
#include "soamemorylayout.hpp"
#include <cstddef>
#include <ranges>
#include <utility>

class DocumentViews
{
public:
  enum class KeymapIndex : unsigned long { def = 0 };
  enum class Visible : uint8_t { visible = 1, hidden = 2 };

  using soadata_t = util::soa::SOA<util::soa::memory_layout::DynamicArray, DocumentView, KeymapIndex, Visible>;
  using Iterator = typename soadata_t::Iterator;

private:
  struct NameToView
  {
    std::string name;
    Iterator view;
  };

  soadata_t m_views;
  std::vector<KeyMap> m_keymaps;

  std::vector<NameToView> m_view_names;
  // std::vector<DocumentView> views;
  Iterator m_active_view;


public:
  void set_active_view(Iterator index)
  {
    if (index.index() < m_views.size()) { m_active_view = index; }
  }

  std::size_t add_keymap(KeyMap &keymap)
  {
    m_keymaps.push_back(std::move(keymap));
    return std::distance(m_keymaps.begin(), m_keymaps.end() - 1);
  }

  void assign_keymap_id_to_document(Iterator doc, std::size_t keymapid)
  {
    if (doc != m_views.end() && keymapid < std::size(m_keymaps)) { doc.get<KeymapIndex>() = KeymapIndex{ keymapid }; }
  }

  Iterator create_view(Document *doc, Height row_height, Width col_width)
  {
    m_views.push_back(DocumentView{ doc }, KeymapIndex::def, Visible::visible);
    auto &d = m_views.back<DocumentView>();
    d.set_position(0, 0, col_width, row_height);
    m_active_view = m_views.end() - 1;
    return m_active_view;
  }

  void view_name_set(Iterator view, const std::string &name)
  {
    auto found = std::ranges::find(m_view_names, name, &NameToView::name);
    if (found != m_view_names.end()) { return; }

    m_view_names.push_back(NameToView{ .name = name, .view = view });
  }

  Iterator view_name_get(const std::string &name)
  {
    auto found = std::ranges::find(m_view_names, name, &NameToView::name);
    if (found == m_view_names.end()) { m_views.end(); }
    return found->view;
  }


  constexpr void hide_view(Iterator view)
  {
    if (view != m_views.end()) view.get<Visible>() = Visible::hidden;
  }

  constexpr void show_view(Iterator view)
  {
    if (view != m_views.end()) view.get<Visible>() = Visible::visible;
  }

  constexpr auto get_visible_views()
  {
    return std::views::filter(m_views, [](auto tup) { return std::get<Visible &>(tup) == Visible::visible; })
           | std::views::transform([](auto tup) -> DocumentView & { return std::get<DocumentView &>(tup); });
  }

  constexpr DocumentView &current_document() { return m_active_view.get<DocumentView>(); }
  constexpr TextBufferView &current_view() { current_document().view(); }
  constexpr KeyMap &current_keymap() { return m_keymaps[std::to_underlying(m_active_view.get<KeymapIndex>())]; }
};
