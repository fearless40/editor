// #include "version.hpp"
#include "cursor.hpp"
#include "document.hpp"
#include "documentview.hpp"
#include "dynamiccommandbuffer.hpp"
#include "enum.hpp"
#include "keymap.hpp"
#include "render.hpp"
#include "term_control.hpp"
#include "textbuffer.hpp"
#include "textbufferview.hpp"
#include "types.hpp"
#include <cstddef>
#include <filesystem>
#include <functional>
#include <iterator>
#include <ranges>
#include <string_view>
#include <utility>
#include <vector>

struct EditorGlobals {
  term::Row rows;
  term::Col cols;
  term::Row cr;
  term::Col cc;
  bool quit_now{false};
};

DocumentManager g_DocManager;
DocumentViewManager g_DocViews;

EditorGlobals editor_globals;
KeyMap key_map;

void render_view(term::CommandBuffer &buff, const TextBufferView &view) {

  if (view.buffer().empty())
    return;

  const auto col_size = std::to_underlying(view.window_cols());
  const auto row_size = std::to_underlying(view.window_rows());
  for (const auto &[index, row] :
       std::views::enumerate(view.buffer().rows) |
           std::views::drop(std::to_underlying(view.row_scroll())) |
           std::views::take(row_size)) {

    if (std::to_underlying(view.col_scroll()) < row.length())
      buff.add(
          row.subview(std::to_underlying(view.col_scroll()), col_size - 1));
    buff.add('\n');
  }
}

int center_left(std::size_t length) {
  auto col = std::to_underlying(editor_globals.cols);
  return (col - length) / 2;
}

void refresh_screen() {

  term::DynamicCommandBuffer buff;
  term::cursor::off(buff);
  term::cursor::reset_position(buff);
  term::clear_screen(buff);

  render_view(buff, g_DocViews.current_TextBufferView());

  auto &view = g_DocViews.current_TextBufferView();

  term::cursor::position(buff, term::Row{(int)view.window_rows() + 1},
                         term::Col{1});

  buff.add("Cursor  R:");
  buff.add((unsigned int)view.row());
  buff.add(" C:");
  buff.add((unsigned int)view.col());
  buff.add(" T:");
  buff.add((unsigned int)view.buffer().last_index());
  buff.add(" L:");
  buff.add((unsigned int)view.buffer().line_length(view.row()));

  term::cursor::position(
      buff, term::Row{(int)g_DocViews.current_TextBufferView().crow() + 1},
      term::Col{(int)(g_DocViews.current_TextBufferView().ccol()) + 1});
  term::cursor::on(buff);
  buff.submit();
}

enum class RequestReason { User, AppError, OSRequest };

void close_app(RequestReason) {
  editor_globals.quit_now = true;
}

;

// Returns false to indicate quitting
bool process_key_presses(const term::KeyStatus &key) {

  if (key_map.key_event(key, g_DocViews.current_TextBufferView()))
    return true;

  if (key.key >= 32 and key.key <= 126 and
      key.position == term::KeyPosition::released) {
    g_DocViews.current_TextBufferView().insert_char_at_cursor(key.key);
    return true;
  }

  return false;
}

int main(int argv, char *argc[]) {

  term::TermControl tc{};

  if (argv >= 2)
    g_DocManager.load(argc[1]);
  else
    g_DocManager.create_empty_document();

  g_DocViews.create_view(g_DocManager.documents.back().get(), tc.height() - 1,
                         tc.width());

  editor_globals.rows = term::Row{tc.height()};
  editor_globals.cols = term::Col{tc.width()};

#define key_once(key_code, code, specialkeys)                                  \
  key_map.add_key((key_code),                                                  \
                  [](const term::KeyStatus &key, TextBufferView &view) {       \
                    code;                                                      \
                    return KeyMap::EventContinue::consume;                     \
                  },                                                           \
                  specialkeys, KeyMap::Repeatability::single);

#define key_many(key_code, code, specialkeys)                                  \
  key_map.add_key((key_code),                                                  \
                  [](const term::KeyStatus &key, TextBufferView &view) {       \
                    code;                                                      \
                    return KeyMap::EventContinue::consume;                     \
                  },                                                           \
                  specialkeys, KeyMap::Repeatability::repeat);

  key_map.add_key(
      term::KeyCodes::HOME,
      [](const term::KeyStatus &key, TextBufferView &view) {
        view.line_home();
        return KeyMap::EventContinue::consume;
      },
      SpecialKeys{0}, KeyMap::Repeatability::single);

  key_once(term::KeyCodes::END, view.line_end(), SpecialKeys{0});
  key_once('c', refresh_screen(), SpecialKeys::alt);
  key_once('q', close_app(RequestReason::User), SpecialKeys::alt);
  key_many(term::KeyCodes::UP, view.up(1), SpecialKeys{0});
  key_many(term::KeyCodes::DOWN, view.down(1), SpecialKeys{0});
  key_many(term::KeyCodes::LEFT, view.left(1), SpecialKeys{0});
  key_many(term::KeyCodes::RIGHT, view.right(1), SpecialKeys{0});
  key_many(term::KeyCodes::DELETE, view.delete_char_to_right(), SpecialKeys{0});
  key_many(term::KeyCodes::BACKSPACE, view.delete_char_to_left(),
           SpecialKeys{0});
  key_many(term::KeyCodes::ENTER, view.insert_enter(), SpecialKeys{0});

#undef key_once
#undef key_many

  refresh_screen();

  while (!editor_globals.quit_now) {
    tc.on_loop();
    if (tc.had_key_event()) {
      auto key_evt = tc.get_key_event();
      if (process_key_presses(key_evt))
        refresh_screen();
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
