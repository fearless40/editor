// #include "version.hpp"
#include "cursor.hpp"
#include "dynamiccommandbuffer.hpp"
#include "render.hpp"
#include "term_control.hpp"
#include "textbuffer.hpp"
#include "textbufferview.hpp"
#include "types.hpp"
#include <cstddef>
#include <fstream>
#include <functional>
#include <ios>
#include <iostream>
#include <ranges>
#include <string_view>
#include <utility>
#include <vector>

struct EditorGlobals {
  term::Row rows;
  term::Col cols;
  term::Row cr;
  term::Col cc;
  TextBuffer text;
  TextBufferView view{text};
  bool quit_now{false};
};

struct KeyMapping {
  EditorGlobals &globals;
  enum struct EventContinue { consume, resume };
  enum struct Repeatablitiy { single, repeat };

  using keyFN = std::function_ref<EventContinue(const term::KeyStatus &key,
                                                EditorGlobals &global)>;

  struct Mapping {
    int keyCode;
    Repeatablitiy status;
    keyFN fn;
  };

  std::vector<Mapping> keymap;

  bool key_event(const term::KeyStatus &key) const {
    for (auto &m : keymap) {
      if (m.keyCode == key.key) {
        if (key.position == term::KeyPosition::pressed and
            m.status == Repeatablitiy::repeat) {
          if (m.fn(key, globals) == EventContinue::consume)
            return true;
        } else if (key.position == term::KeyPosition::released and
                   m.status == Repeatablitiy::single) {
          if (m.fn(key, globals) == EventContinue::consume)
            return true;
        }
      }
    }
    return false;
  }

  void add_key(term::KeyCodes code, Repeatablitiy repeat, keyFN &&keyfn) {
    keymap.emplace_back(std::to_underlying(code), repeat, keyfn);
  }
};

EditorGlobals editor_globals;
KeyMapping key_map{editor_globals};

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

  render_view(buff, editor_globals.view);

  term::cursor::position(buff,
                         term::Row{(int)editor_globals.view.window_rows() + 1},
                         term::Col{1});

  buff.add("Cursor  R:");
  buff.add((unsigned int)editor_globals.view.row());
  buff.add(" C:");
  buff.add((unsigned int)editor_globals.view.col());
  buff.add(" T:");
  buff.add((unsigned int)editor_globals.text.number_rows());
  buff.add(" L:");
  buff.add(
      (unsigned int)editor_globals.text.line_length(editor_globals.view.row()));

  term::cursor::position(buff, term::Row{(int)editor_globals.view.crow() + 1},
                         term::Col{(int)(editor_globals.view.ccol()) + 1});
  term::cursor::on(buff);
  buff.submit();
}

enum class RequestReason { User, AppError, OSRequest };

void close_app(RequestReason) {
  editor_globals.quit_now = true;
}

;

bool key_press_continous(const term::KeyStatus &key) {
  if (key.position != term::KeyPosition::pressed)
    return false;

  if (key.key == std::to_underlying(term::KeyCodes::UP)) {
    editor_globals.view.up(1);
    return true;
    // tem::cursor::up(1);
  }
  if (key.key == std::to_underlying(term::KeyCodes::LEFT)) {
    // term::cursor::left(1);
    editor_globals.view.left(1);
    return true;
  }
  if (key.key == std::to_underlying(term::KeyCodes::DOWN)) {
    editor_globals.view.down(1);
    return true;
    // term::cursor::down(1);
  }
  if (key.key == std::to_underlying(term::KeyCodes::RIGHT)) {
    // term::cursor::right(1);
    editor_globals.view.right(1);
    return true;
  }

  if (key.key == std::to_underlying(term::KeyCodes::DELETE)) {
    editor_globals.view.delete_char_to_right();
    return true;
  }

  if (key.key == std::to_underlying(term::KeyCodes::BACKSPACE)) {
    editor_globals.view.delete_char_to_left();
    return true;
  }

  if (key.key == std::to_underlying(term::KeyCodes::ENTER)) {
    editor_globals.view.insert_enter();
    return true;
  }
  return false;
}

bool key_press_only_once(const term::KeyStatus &key) {
  if (key.position != term::KeyPosition::released)
    return false;

  if (key.key == 'q' && key.alt == true) {
    close_app(RequestReason::User);
    return true;
  }

  // if (key.key == std::to_underlying(term::KeyCodes::HOME)) {
  //   editor_globals.view.line_home();
  //   refresh_screen();
  //   return true;
  // }

  if (key.key == std::to_underlying(term::KeyCodes::END)) {
    editor_globals.view.line_end();
    refresh_screen();
    return true;
  }

  if (key.key == 'c' && key.alt == true) {
    refresh_screen();
    return true;
  }

  return false;
}

// Returns false to indicate quitting
bool process_key_presses(const term::KeyStatus &key) {

  if (key_press_only_once(key))
    return true;

  if (key_map.key_event(key))
    return true;

  if (key_press_continous(key))
    return true;

  if (key.key >= 32 and key.key <= 126 and
      key.position == term::KeyPosition::released) {
    editor_globals.view.insert_char_at_cursor(key.key);
    return true;
  }

  return false;
}

bool open_file(const char *filename) {
  std::fstream f{filename, std::ios_base::in};
  if (!f.is_open())
    return false;

  std::string line;
  while (!f.eof()) {
    std::getline(f, line);
    if (line.ends_with('\n') || line.ends_withorsize() - 1)
      ;
  }
  editor_globals.text.rows.emplace_back(std::move(line));
};
return true;
}

int main(int argv, char *argc[]) {

  // std::cout << "Hello from the battleship program!\n";
  // std::cout << "Version: " << Version::MAJOR_VERSION << "."
  //           << Version::MINOR_VERSION << '\n';

  // compositor_test();

  term::TermControl tc{};
  bool still_running = true;

  if (argv >= 2) {
    open_file(argc[1]);
  }

  editor_globals.rows = term::Row{tc.height()};
  editor_globals.cols = term::Col{tc.width()};
  editor_globals.view.set_window(RowSize{(std::size_t)tc.height() - 1},
                                 ColSize{(std::size_t)tc.width()});

  key_map.add_key(term::KeyCodes::HOME, KeyMapping::Repeatablitiy::single,
                  [](const term::KeyStatus &key, EditorGlobals &global) {
                    global.view.line_home();
                    return KeyMapping::EventContinue::consume;
                  });

  // editor_globals.text.append_row("Hello text editor world"sv);

  refresh_screen();

  while (!editor_globals.quit_now) {
    tc.on_loop();
    if (tc.had_key_event()) {
      if (process_key_presses(tc.get_key_event()) or
          key_map.key_event(tc.get_key_event()))
        refresh_screen();
    }
  }

  return 0;
}
