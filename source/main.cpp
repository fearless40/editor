// #include "version.hpp"
#include "cursor.hpp"
#include "dynamiccommandbuffer.hpp"
#include "render.hpp"
#include "term_control.hpp"
#include "types.hpp"
#include <cstddef>
#include <fstream>
#include <ios>
#include <iostream>
#include <ranges>
#include <string_view>
#include <utility>
#include <vector>
void begin_game();

void soa_test() {
  // util::soa::SOA<util::soa::DynamicArray, char, int> soa;
  // {
  //   util::soa::SOA<util::soa::memory_layout::FixedArray<100>, char, int> soa;
  //
  //   soa.push_back('a', 1);
  //   soa.push_back('b', 2);
  //   soa.push_back('c', 3);
  //   soa.push_back('d', 4);
  //
  //   std::println("Capacity: {}, Size: {}", soa.capacity(), soa.size());
  // }
  // {
  //   util::soa::SOA<util::soa::memory_layout::DynamicArray, char, int> soa;
  //
  //   soa.push_back('a', 1);
  //   soa.push_back('b', 2);
  //   soa.push_back('c', 3);
  //   soa.push_back('d', 4);
  //
  //   for (int i = 1; i < 250; ++i) {
  //     soa.push_back('a', i);
  //   }
  //
  //   soa.remove(soa.end() - 1);
  //
  //   std::println("Capacity: {}, Size: {}", soa.capacity(), soa.size());
  // }
}

void image_test() {
  //
  //
  // term::TermControl tc{};
  // using ColorOnly = term::details::Pixel_<term::Color, term::ASCII>;
  // term::Image<ColorOnly> picture(10, 10);
  //
  // for (unsigned int x = 0; x < picture.width(); ++x) {
  //   for (unsigned int y = 0; y < picture.height(); ++y) {
  //     ColorOnly p;
  //     p.red = y * 10;
  //     p.green = x * 10;
  //     p.blue = x * y;
  //     p.value = 'a' + x;
  //
  //     picture.set_pixel(x, y, p);
  //   }
  // }
  //
  // unsigned char buffer[4096];
  //
  // term::details::render_to_buffer(picture, buffer);
  //
  // std::cout << buffer; //<< std::endl;
  //
  // while (1) {
  //   tc.on_loop();
  //
  //   // sleep(1);
  // }
}

struct TextBuffer {
  std::vector<std::string> rows;
  std::size_t number_rows() const { return rows.size(); }
  constexpr bool empty() const { return rows.empty(); }

  void append_row(std::string_view data) { rows.emplace_back(data); }
};

struct TextBufferView {
  TextBuffer &view;

  int screen_rows{0};
  int screen_cols{0};
  int cursor_row{0};
  int cursor_col{0};
  int row_offset{0};
  int col_offset{0};

  constexpr term::Row crow() const {
    return term::Row{(cursor_row - row_offset) + 1};
  }

  constexpr term::Col ccol() const {
    return term::Col{(cursor_col - col_offset) + 1};
  }

  constexpr void up(int amt) {
    cursor_row = std::max(0, cursor_row - amt);
    if (cursor_row < row_offset)
      row_offset = cursor_row;
  }

  constexpr void down(int amt) {
    cursor_row =
        std::min(static_cast<int>(view.number_rows()), cursor_row + amt);
    if (cursor_row >= row_offset + screen_rows)
      row_offset = cursor_row - screen_rows + 1;
  }

  constexpr void left(int amt) {
    cursor_col = std::max(0, cursor_col - amt);
    if (cursor_col < col_offset)
      col_offset = cursor_col;
  };

  constexpr void right(int amt) {
    cursor_col =
        std::min(static_cast<int>(view.number_rows()), cursor_col + amt);
    if (cursor_col >= col_offset + screen_cols)
      col_offset = cursor_col - screen_cols + 1;
  }
};

struct EditorGlobals {
  term::Row rows;
  term::Col cols;
  term::Row cr;
  term::Col cc;
  TextBuffer text;
  TextBufferView view{text, 0, 0, 0, 0, 0, 0};
};

EditorGlobals editor_globals;

void refresh_screen() {
  auto line1 = "Welcome to my stupid editor"sv;
  auto line2 = "by"sv;
  auto line3 = "Adam Spivack"sv;
  auto line4 = "Version 0.0"sv;

  auto left_padding = [&](std::size_t length) -> auto {
    auto col = std::to_underlying(editor_globals.cols);
    return (col - length) / 2;
  };

  term::DynamicCommandBuffer buff;
  term::cursor::off(buff);
  term::cursor::reset_position(buff);
  term::clear_screen(buff);

  if (!editor_globals.text.empty()) {
    const auto col_size = std::to_underlying(editor_globals.cols);
    const auto row_size = std::to_underlying(editor_globals.rows);
    for (const auto &[index, row] :
         std::views::enumerate(editor_globals.text.rows) |
             std::views::drop(editor_globals.view.row_offset) |
             std::views::take(row_size)) {

      if (editor_globals.view.col_offset < row.length())
        buff.add(row.subview(editor_globals.view.col_offset, col_size - 1));
      buff.add('\n');
    }
    // } else {
    //
    //   for (int r = 0; r < std::to_underlying(editor_globals.rows); ++r) {
    //     if (r == 3) {
    //       buff.m_data.append_range(
    //           std::ranges::views::repeat(' ',
    //           left_padding(line1.length())));
    //       buff.m_data.append(line1);
    //       buff.add('\n');
    //     } else if (r == 4) {
    //       buff.m_data.append_range(
    //           std::ranges::views::repeat(' ',
    //           left_padding(line2.length())));
    //       buff.m_data.append(line2);
    //       buff.add('\n');
    //     } else if (r == 5) {
    //       buff.m_data.append_range(
    //           std::ranges::views::repeat(' ',
    //           left_padding(line3.length())));
    //       buff.m_data.append(line3);
    //       buff.add('\n');
    //     } else if (r == 6) {
    //       buff.m_data.append_range(
    //           std::ranges::views::repeat(' ',
    //           left_padding(line4.length())));
    //       buff.m_data.append(line4);
    //       buff.add('\n');
    //     } else
    //       buff.add("~\n");
    //   }
  }

  term::cursor::position(buff, editor_globals.view.crow(),
                         editor_globals.view.ccol());
  term::cursor::on(buff);
  buff.submit();
}

// Returns false to indicate quitting
bool process_key_presses(const term::KeyStatus &key) {
  if (key.position == term::KeyPosition::released && key.key == 'q' &&
      key.alt == true)
    return false;
  if (key.position == term::KeyPosition::released && key.key == 'm' &&
      key.alt == true) {
    auto posOp = term::cursor::get_position();
    if (posOp) {
      auto pos = posOp.value();
      std::cout << "Cursor Pos: " << std::to_underlying(pos.first) << ":"
                << std::to_underlying(pos.second);
      return true;
    } else {
      std::cout << "Could not read cursor position.\n";
      return true;
    }
  }

  if (key.position == term::KeyPosition::released && key.key == 'c' &&
      key.alt == true) {
    refresh_screen();
    return true;
  }

  if (key.position == term::KeyPosition::pressed) {
    if (key.key == std::to_underlying(term::KeyCodes::UP)) {
      editor_globals.view.up(1);
      // term::cursor::up(1);
    }
    if (key.key == std::to_underlying(term::KeyCodes::LEFT)) {
      // term::cursor::left(1);
      editor_globals.view.left(1);
    }
    if (key.key == std::to_underlying(term::KeyCodes::DOWN)) {
      editor_globals.view.down(1);
      // term::cursor::down(1);
    }
    if (key.key == std::to_underlying(term::KeyCodes::RIGHT)) {
      // term::cursor::right(1);
      editor_globals.view.right(1);
    }
    // Otherwise echo the key
    refresh_screen();
  }

  return true;
}

bool open_file(const char *filename) {
  std::fstream f{filename, std::ios_base::in};
  if (!f.is_open())
    return false;

  std::string line;
  while (!f.eof()) {
    std::getline(f, line);
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
  editor_globals.view.screen_rows = tc.height();
  editor_globals.view.screen_cols = tc.width();

  editor_globals.text.append_row("Hello text editor world"sv);

  while (still_running) {
    tc.on_loop();
    if (tc.had_key_event()) {
      still_running = process_key_presses(tc.get_key_event());
    }
  }

  return 0;
}
