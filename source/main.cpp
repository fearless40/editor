// #include "version.hpp"
#include "commandbuffer.hpp"
#include "cursor.hpp"
#include "document.hpp"
#include "documentview.hpp"
#include "documentviews.hpp"
#include "dynamiccommandbuffer.hpp"
#include "editortypes.hpp"
#include "keymap.hpp"
#include "render.hpp"
#include "term_control.hpp"
#include "textbuffer.hpp"
#include "textbufferview.hpp"
#include "types.hpp"
#include <iterator>
#include <string_view>
#include <utility>


struct EditorGlobals
{
  term::Row rows{ 1 };
  term::Col cols{ 1 };
  bool quit_now{ false };
  bool user_save_as_mode{ false };
};

DocumentManager g_DocManager;
DocumentViews g_DocViews;

EditorGlobals editor_globals;


void render_fixedbufferview(term::CommandBuffer &buff, FixedWidthTextBuffer &fbuf)
{
  term::cursor::reset_position(buff);
  for (auto [index, sv] : fbuf.line_view_index()) {
    buff.add(sv);
    // buff.add('\n');

    term::cursor::position(buff, Row{ static_cast<RowOffset::underlying_t>(index) }, Col{ 0 });
  }
}

void render_view(term::CommandBuffer &buff,
  const TextBufferView &view,
  term::Row row_offset = term::Row{ 1 },
  term::Col col_offset = term::Col{ 1 })
{

  if (view.buffer()->empty()) return;


  const auto col_size = view.window_cols();
  const auto row_size = view.window_rows();
  for (const auto &[index, row] : view.buffer()->rows | std::views::drop(view.row_scroll().underlying())
                                    | std::views::take(row_size.underlying()) | std::views::enumerate) {

    const auto col_scroll = view.col_scroll().underlying();
    const auto len = std::clamp((int)std::ssize(row) - col_scroll, 0, view.window_cols().underlying());


    auto const set_row_pos = row_offset.value + index;

    term::cursor::position(buff, term::Row{ (int)set_row_pos }, col_offset);
    if (std::ssize(row) > col_scroll) { buff.add(row.subview(col_scroll, len)); }
  }
}

void render_document_view(term::CommandBuffer &cmd, DocumentView &view)
{
  // Ignore Header, Footer, RightGutter for now
  //
  //
  if (view.m_left_gutter_visible) { render_fixedbufferview(cmd, view.left_gutter()); };
  render_view(cmd, view.m_view, view.m_y, view.get_buffer_col());
}

void render_status_line(term::CommandBuffer &buff)
{

  auto &doc = g_DocViews.current_document();
  auto cur_pos = doc.cursor_position();
  term::cursor::position(buff, origin(origin(editor_globals.rows)), Col{ 0 });

  buff.add("Cursor  R:");
  buff.add((unsigned int)cur_pos.first.underlying());
  buff.add(" ViewR: ");
  buff.add((unsigned int)doc.m_view.cursor_row_file().underlying());
  buff.add(" C:");
  buff.add((unsigned int)cur_pos.second.underlying());
  buff.add(" ViewC: ");
  buff.add((unsigned int)doc.m_view.cursor_col_file().underlying());
  buff.add(" RowScroll: ");
  buff.add((unsigned int)doc.m_view.row_scroll().underlying());
}

void refresh_screen()
{

  term::DynamicCommandBuffer buff;
  term::cursor::off(buff);
  term::cursor::reset_position(buff);
  term::clear_screen(buff);

  auto &doc = g_DocViews.current_document();

  render_document_view(buff, doc);

  if (editor_globals.user_save_as_mode) {

  } else {
    render_status_line(buff);
  }

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
void save_as_query(DocumentView &document) {}
void save_as_query_current_view()
{
  save_as_query(g_DocViews.current_document());
  editor_globals.user_save_as_mode = true;
};

void close_system_input_buffer(bool user_agreed)
{
  editor_globals.user_save_as_mode = false;
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

  g_DocManager.create_system_document(std::to_underlying(SystemDocuments::user_input));

  if (argv >= 2)
    g_DocManager.load(argc[1]);
  else
    g_DocManager.create_empty_document();

  g_DocViews.create_view(g_DocManager.documents.back().get(), tc.height() - 1, tc.width());

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

  auto defKeyMap = g_DocViews.add_keymap(key_map);
  {
    auto kmSpecial{ key_map };
    key_once(kmSpecial, term::KeyCodes::ENTER, ::close_system_input_buffer(true), SpecialKeys{ 0 });

    key_once(kmSpecial, term::KeyCodes::ESCAPE, ::close_system_input_buffer(false), SpecialKeys{ 0 });
    auto specialmap = g_DocViews.add_keymap(kmSpecial);
  }


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
