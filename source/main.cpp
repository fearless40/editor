// #include "version.hpp"
#include "commandbuffer.hpp"
#include "cursor.hpp"
#include "document.hpp"
#include "documentview.hpp"
#include "dynamiccommandbuffer.hpp"
#include "editortypes.hpp"
#include "keymap.hpp"
#include "render.hpp"
#include "term_control.hpp"
#include "textbuffer.hpp"
#include "types.hpp"
#include <string_view>


struct EditorGlobals
{
  term::Row rows{ 1 };
  term::Col cols{ 1 };
  bool quit_now{ false };
};

DocumentManager g_DocManager;
DocumentViewManager g_DocViews;

EditorGlobals editor_globals;
KeyMap key_map;

void render_fixedbufferview(term::CommandBuffer &buff, FixedWidthTextBuffer &fbuf)
{
  term::cursor::reset_position(buff);
  for (auto [index, sv] : fbuf.line_view_index()) {
    buff.add(sv);
    // buff.add('\n');

    term::cursor::position(buff, RowOffset{ static_cast<RowOffset::underlying_t>(index) }, ColOffset{ 0 });
  }
}

void render_view(term::CommandBuffer &buff,
  const TextBufferView &view,
  term::Row row_offset = term::Row{ 1 },
  term::Col col_offset = term::Col{ 1 })
{

  if (view.buffer().empty()) return;

  term::cursor::position(buff, row_offset, col_offset);

  const auto col_size = view.window_cols();
  const auto row_size = view.window_rows();
  for (const auto &[index, row] : view.buffer().rows | std::views::drop(view.row_scroll().underlying())
                                    | std::views::take(row_size.underlying()) | std::views::enumerate) {

    const auto col_scroll = view.col_scroll().underlying();

    if (col_scroll < std::ssize(row)) buff.add(row.subview(col_scroll, (col_size - Width{ 1 }).underlying()));
    // else
    // buff.add(row);
    auto const set_row_pos = origin(row_offset) + term::Height{ (term::Height::underlying_t)index + 1 };
    term::cursor::position(buff, set_row_pos, col_offset);
  }
}

void render_document_view(term::CommandBuffer &cmd, DocumentView &view)
{
  // Ignore Header, Footer, RightGutter for now
  //
  //
  render_fixedbufferview(cmd, view.left_gutter());
  render_view(cmd, view.m_view, view.m_x, view.m_y);
}

void refresh_screen()
{

  term::DynamicCommandBuffer buff;
  term::cursor::off(buff);
  term::cursor::reset_position(buff);
  term::clear_screen(buff);

  // auto &view = g_DocViews.current_view();
  //
  auto &doc = g_DocViews.current_document();

  // g_LeftGutter.update_width(view);
  // g_LeftGutter.raw_view(view);

  // render_fixedbufferview(buff, g_LeftGutter.buff);

  // render_view(buff, view, 0, g_LeftGutter.total_width());
  render_document_view(buff, doc);

  auto cur_pos = doc.cursor_position();

  term::cursor::reset_position(buff);

  buff.add("Cursor  R:");
  buff.add((unsigned int)cur_pos.second.underlying());
  buff.add(" C:");
  buff.add((unsigned int)cur_pos.first.underlying());
  // buff.add(" T:");
  // buff.add((unsigned int)view.buffer().last_index());
  // buff.add(" L:");
  // buff.add((unsigned int)view.buffer().line_length(view.cursor_row_file()));
  //
  term::cursor::position(buff, cur_pos.second, cur_pos.first);
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

  if (key_map.key_event(key, g_DocViews.current_view())) return true;

  if (key.key >= 32 and key.key <= 126 and key.position == term::KeyPosition::released) {
    g_DocViews.current_view().insert_char_at_cursor(key.key);
    return true;
  }

  return false;
}


int main(int argv, char *argc[])
{

  term::TermControl tc{};


  if (argv >= 2)
    g_DocManager.load(argc[1]);
  else
    g_DocManager.create_empty_document();

  g_DocViews.create_view(g_DocManager.documents.back().get(), tc.height() - 1, tc.width());

  editor_globals.rows = term::Row{ tc.height() };
  editor_globals.cols = term::Col{ tc.width() };

#define key_once(key_code, code, specialkeys)              \
  key_map.add_key((key_code),                              \
    [](const term::KeyStatus &key, TextBufferView &view) { \
      code;                                                \
      return KeyMap::EventContinue::consume;               \
    },                                                     \
    specialkeys,                                           \
    KeyMap::Repeatability::single);

#define key_many(key_code, code, specialkeys)              \
  key_map.add_key((key_code),                              \
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

  key_once(term::KeyCodes::END, view.line_end(), SpecialKeys{ 0 });
  key_once('c', refresh_screen(), SpecialKeys::alt);
  key_once('q', close_app(RequestReason::User), SpecialKeys::alt);
  key_many(term::KeyCodes::UP, view.up(1), SpecialKeys{ 0 });
  key_many(term::KeyCodes::DOWN, view.down(1), SpecialKeys{ 0 });
  key_many(term::KeyCodes::LEFT, view.left(1), SpecialKeys{ 0 });
  key_many(term::KeyCodes::RIGHT, view.right(1), SpecialKeys{ 0 });
  key_many(term::KeyCodes::DELETE, view.delete_char_to_right(), SpecialKeys{ 0 });
  key_many(term::KeyCodes::BACKSPACE, view.delete_char_to_left(), SpecialKeys{ 0 });
  key_many(term::KeyCodes::ENTER, view.insert_enter(), SpecialKeys{ 0 });

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

// OLD CODE

// bool key_press_continous(const term::KeyStatus &key) {
//   if (key.position != term::KeyPosition::pressed)
//     return false;
//
//   if (key.key == std::to_underlying(term::KeyCodes::UP)) {
//     editor_globals.view.up(1);
//     return true;
//     // tem::cursor::up(1);
//   }
//   if (key.key == std::to_underlying(term::KeyCodes::LEFT)) {
//     // term::cursor::left(1);
//     editor_globals.view.left(1);
//     return true;
//   }
//   if (key.key == std::to_underlying(term::KeyCodes::DOWN)) {
//     editor_globals.view.down(1);
//     return true;
//     // term::cursor::down(1);
//   }
//   if (key.key == std::to_underlying(term::KeyCodes::RIGHT)) {
//     // term::cursor::right(1);
//     editor_globals.view.right(1);
//     return true;
//   }
//
//   if (key.key == std::to_underlying(term::KeyCodes::DELETE)) {
//     editor_globals.view.delete_char_to_right();
//     return true;
//   }
//
//   if (key.key == std::to_underlying(term::KeyCodes::BACKSPACE)) {
//     editor_globals.view.delete_char_to_left();
//     return true;
//   }
//
//   if (key.key == std::to_underlying(term::KeyCodes::ENTER)) {
//     editor_globals.view.insert_enter();
//     return true;
//   }
//   return false;
// }
//
//
//
// bool key_press_only_once(const term::KeyStatus &key) {
//   if (key.position != term::KeyPosition::released)
//     return false;
//
//   if (key.key == 'q' && key.alt == true) {
//     close_app(RequestReason::User);
//     return true;
//   }
//
//   // if (key.key == std::to_underlying(term::KeyCodes::HOME)) {
//   //   editor_globals.view.line_home();
//   //   refresh_screen();
//   //   return true;
//   // }
//
//   if (key.key == std::to_underlying(term::KeyCodes::END)) {
//     editor_globals.view.line_end();
//     return true;
//   }
//
//   if (key.key == 'c' && key.alt == true) {
//     refresh_screen();
//     return true;
//   }
//
//   return false;
// }
