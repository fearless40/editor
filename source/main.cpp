#include "commandbuffer.hpp"
#include "cursor.hpp"
#include "dimensions.hpp"
#include "document.hpp"
#include "documentview.hpp"
#include "documentviews.hpp"
#include "dynamiccommandbuffer.hpp"
#include "editortypes.hpp"
#include "fixedwidthtextbuffer.hpp"
#include "keymap.hpp"
#include "render.hpp"
#include "term_control.hpp"
#include "textbufferview.hpp"
#include "types.hpp"
#include <string_view>
#include <utility>


struct EditorGlobals
{
  term::Row rows{ 1 };
  term::Col cols{ 1 };
  bool quit_now{ false };
  bool user_save_as_mode{ false };

  DocumentViews::Iterator last_view;
  constexpr const static std::string_view text_input = "##system_input";
};

DocumentManager g_DocManager;
DocumentViews g_DocViews;

EditorGlobals editor_globals;

void render_view(term::CommandBuffer &buff,
  DocumentView &view,
  Row row_offset = term::Row{ 1 },
  Col col_offset = term::Col{ 1 })
{
  FixedWidthTextBuffer renderTo;
  renderTo.ensure_size(view.width(), 1);

  view.calculate_widths(geom::to_position(view.get_first_visible_row()));
  // const auto col_size = view.window_cols();
  // const auto row_size = view.window_rows();
  for (RowOffset row = view.get_first_visible_row(); row < view.get_last_visible_row(); ++row) {

    term::cursor::position(buff, row - view.get_first_visible_row() + row_offset, col_offset);
    auto sp = renderTo.span_at(0);
    Renderable_Input input = {
      .text_buffer = sp,
      .color_buffer = {},
      .view = view.view(),
      .row = geom::to_position(row),
      .fmt = OutputFormat::ASCII,
    };
    auto output = view.render(input);
    term::clear_current_line(buff);
    if (output.char_written > 0) {
      buff.add(std::string_view((char *)&*sp.begin(), (char *)&*output.text_buffer.begin()));
    }
  }
}


void render_document_view(term::CommandBuffer &cmd, DocumentView &view)
{ render_view(cmd, view, view.row(), view.col()); }

void render_status_line(term::CommandBuffer &buff)
{

  auto &doc = g_DocViews.current_document();
  auto cur_pos = doc.cursor_position();
  term::cursor::position(buff, geom::to_distance(editor_globals.rows) + term::Row{ 1 }, Col{ 0 });

  buff.add("Cursor  R:");
  buff.add((unsigned int)cur_pos.first.underlying());
  buff.add(" ViewR: ");
  // buff.add((unsigned int)doc.m_view.cursor_row_file().underlying());
  buff.add(" C:");
  buff.add((unsigned int)cur_pos.second.underlying());
  buff.add(" ViewC: ");
  // buff.add((unsigned int)doc.m_view.cursor_col_file().underlying());
  buff.add(" RowScroll: ");
  // buff.add((unsigned int)doc.m_view.row_scroll().underlying());
}

void render_save_as(term::CommandBuffer &buff) {}

void refresh_screen()
{

  term::DynamicCommandBuffer buff;
  term::cursor::off(buff);
  term::cursor::reset_position(buff);
  term::clear_screen(buff);

  for (auto &view : g_DocViews.get_visible_views()) { render_document_view(buff, view); }

  if (editor_globals.user_save_as_mode) {
    render_save_as(buff);
  } else {
    render_status_line(buff);
  }

  auto &doc = g_DocViews.current_document();
  auto cur_pos = doc.cursor_position();

  term::cursor::position(buff, cur_pos.first, cur_pos.second);
  term::cursor::on(buff);
  buff.submit();
}

enum class RequestReason : std::uint8_t { User, AppError, OSRequest };

void close_app(RequestReason)
{
  editor_globals.quit_now = true;
}

;

// Returns false to indicate quitting
bool process_key_presses(const term::KeyStatus &key)
{
  auto &ckeymap = g_DocViews.current_keymap();
  return ckeymap.key_event(key, g_DocViews.current_view());
}
void save_as_query(DocumentView &document)
{
  editor_globals.last_view = g_DocViews.current_document_iterator();
  auto *doc = g_DocManager.get_system_document(EditorGlobals::text_input);
  if (doc == nullptr) return;
  auto &buff = doc->m_buffer;
  buff.clear();
  auto name = g_DocManager.get_name(document.document());
  buff.append_row(name);
  auto system_input = g_DocViews.view_name_get(EditorGlobals::text_input);
  g_DocViews.show_view(system_input);
  g_DocViews.set_active_view(system_input);
}
void save_as_query_current_view()
{
  save_as_query(g_DocViews.current_document());
  editor_globals.user_save_as_mode = true;
};

void close_system_input_buffer(bool user_agreed)
{
  editor_globals.user_save_as_mode = false;
  auto system_input = g_DocViews.view_name_get(EditorGlobals::text_input);
  g_DocViews.hide_view(system_input);
  g_DocViews.set_active_view(editor_globals.last_view);

  if (user_agreed) {
    // Then save the file ...
    return;
  }
}

enum class SystemDocuments : uint8_t { user_input, scratch_pad };

int main(int argv, char *argc[])
{

  term::TermControl tc{};

  KeyMap key_map;

  g_DocManager.create_system_document(EditorGlobals::text_input);

  if (argv >= 2)
    g_DocManager.load(argc[1]);
  else
    g_DocManager.create_empty_document();

  auto user_view = g_DocViews.create_view(g_DocManager.back(), tc.height() - 1, tc.width());
  auto *system_doc = g_DocManager.create_system_document(EditorGlobals::text_input);
  auto system_view = g_DocViews.create_view(system_doc, Height{ 1 }, tc.width());
  system_view.get<DocumentView &>().set_position(Col{ 0 }, Row{ tc.height() }, tc.width(), Height{ 1 });
  g_DocViews.view_name_set(system_view, EditorGlobals::text_input);
  g_DocViews.hide_view(system_view);

  g_DocViews.set_active_view(user_view);


  editor_globals.rows = term::Row{ tc.height() };
  editor_globals.cols = term::Col{ tc.width() };

#define key_once(km, key_code, code, specialkeys)          \
  km.add_key((key_code),                                   \
    [](const term::KeyStatus &key, TextBufferView &view) { \
      code;                                                \
      return KeyMap::EventContinue::consume;               \
    },                                                     \
    specialkeys,                                           \
    KeyMap::Repeatability::single);

#define key_many(km, key_code, code, specialkeys)          \
  km.add_key((key_code),                                   \
    [](const term::KeyStatus &key, TextBufferView &view) { \
      code;                                                \
      return KeyMap::EventContinue::consume;               \
    },                                                     \
    specialkeys,                                           \
    KeyMap::Repeatability::repeat);

  key_map.add_key(
    term::KeyCodes::HOME,
    [](const term::KeyStatus &key, TextBufferView &view) {
      view.line_home();
      return KeyMap::EventContinue::consume;
    },
    SpecialKeys{ 0 },
    KeyMap::Repeatability::single);

  key_once(key_map, 's', save_as_query_current_view(), SpecialKeys::alt);
  key_once(key_map, term::KeyCodes::END, view.line_end(), SpecialKeys{ 0 });
  key_once(key_map, 'c', refresh_screen(), SpecialKeys::alt);
  key_once(key_map, 'q', close_app(RequestReason::User), SpecialKeys::alt);
  key_many(key_map, term::KeyCodes::UP, view.move_cursor_vert(-1), SpecialKeys{ 0 });
  key_many(key_map, term::KeyCodes::DOWN, view.move_cursor_vert(1), SpecialKeys{ 0 });
  key_many(key_map, term::KeyCodes::LEFT, view.move_cursor_horz(-1), SpecialKeys{ 0 });
  key_many(key_map, term::KeyCodes::RIGHT, view.move_cursor_horz(1), SpecialKeys{ 0 });
  key_many(key_map, term::KeyCodes::DELETE, view.delete_char_to_right(), SpecialKeys{ 0 });
  key_many(key_map, term::KeyCodes::BACKSPACE, view.delete_char_to_left(), SpecialKeys{ 0 });
  key_many(key_map, term::KeyCodes::ENTER, view.insert_enter(), SpecialKeys{ 0 });

  key_map.on_nomatch([](const term::KeyStatus &key, TextBufferView &view) {
    if (key.key >= 32 and key.key <= 126 and key.position == term::KeyPosition::pressed) {
      view.insert_char_at_cursor(key.key);
      return KeyMap::EventContinue::consume;
    }
    return KeyMap::EventContinue::resume;
  });


  {
    auto kmSpecial{ key_map };
    key_once(kmSpecial, term::KeyCodes::ENTER, ::close_system_input_buffer(true), SpecialKeys{ 0 });

    key_once(kmSpecial, term::KeyCodes::ESCAPE, ::close_system_input_buffer(false), SpecialKeys{ 0 });
    auto specialmap = g_DocViews.add_keymap(kmSpecial);

    g_DocViews.assign_keymap_id_to_document(system_view, specialmap);
  }

  auto defKeyMap = g_DocViews.set_default_keymap(key_map);

#undef key_once
#undef key_many

  refresh_screen();

  while (!editor_globals.quit_now) {
    tc.on_loop();
    if (tc.had_key_event()) {
      auto key_evt = tc.get_key_event();
      if (process_key_presses(key_evt)) refresh_screen();
    }
  }
  return 0;
}
