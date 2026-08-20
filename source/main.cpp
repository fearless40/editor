// #include "version.hpp"
#include "cursor.hpp"
#include "render.hpp"
#include "term_control.hpp"
#include "terminfo.hpp"
#include <iostream>
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

struct EditorGlobals {
  term::Row rows;
  term::Col cols;
};

EditorGlobals editor_globals;

void refresh_screen() {
  term::SizedCommandBuffer<255> buff;
  term::clear_screen(buff);
  for (int r = 0; r < std::to_underlying(editor_globals.rows); ++r) {
    buff.add("~\n");
  }

  term::cursor::reset_position(buff);
  buff.submit();
}

// Returns false to indicate quitting
bool process_key_presses(const term::KeyStatus &key) {
  if (key.key > 255)
    return true;
  if (key.position == term::KeyPosition::released && key.key == 'q' &&
      key.alt == true)
    return false;
  if (key.position == term::KeyPosition::released && key.key == 'm' &&
      key.alt == true) {
    auto posOp = term::info::get_cursor_position();
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

  if (key.position == term::KeyPosition::released)
    // Otherwise echo the key
    std::cout << (char)key.key;

  return true;
}

int main(int argv, char *argc[]) {

  // std::cout << "Hello from the battleship program!\n";
  // std::cout << "Version: " << Version::MAJOR_VERSION << "."
  //           << Version::MINOR_VERSION << '\n';

  // compositor_test();

  term::TermControl tc{};
  bool still_running = true;

  editor_globals.rows = term::Row{tc.height()};
  editor_globals.cols = term::Col{tc.width()};

  while (still_running) {
    tc.on_loop();
    if (tc.had_key_event()) {
      still_running = process_key_presses(tc.get_key_event());
    }
  }

  return 0;
}
