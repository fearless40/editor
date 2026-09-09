// #include "version.hpp"
#include "cursor.hpp"
#include "textbufferview.hpp"
#include "dynamiccommandbuffer.hpp"
#include "textbuffer.hpp"
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


struct EditorGlobals {
  term::Row rows;
  term::Col cols;
  term::Row cr;
  term::Col cc;
  TextBuffer text;
  TextBufferView view{text};
   bool quit_now {false}; 
};

EditorGlobals editor_globals;

void render_view( term::CommandBuffer & buff, const TextBufferView & view ) { 

if (view.buffer().empty()) 
   return; 

    const auto col_size = std::to_underlying(view.window_cols());
    const auto row_size = std::to_underlying(view.window_rows());
    for (const auto &[index, row] :
         std::views::enumerate(view.buffer().rows) |
             std::views::drop(std::to_underlying(view.row_scroll())) |
             std::views::take(row_size)) {

      if (std::to_underlying(view.col_scroll()) < row.length())
        buff.add(row.subview(std::to_underlying(view.col_scroll()), col_size - 1));
      buff.add('\n');
    }
} 


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


   render_view( buff, editor_globals.view );

  // term::cursor::position(
  //     buff, term::Row{(int)editor_globals.view.window_rows() - 1}, term::Col{0});

  // buff.add("Cursor  R:");
  // buff.add((unsigned int)view.cursor_row);
  // buff.add(" C:");
  // buff.add((unsigned int)view.cursor_col);
  //
  term::cursor::position(buff, editor_globals.view.crow(),
                         editor_globals.view.ccol());
  term::cursor::on(buff);
  buff.submit();
}

enum class RequestReason { 
   User, 
   AppError,
   OSRequest
};


void close_app( RequestReason ){
   editor_globals.quit_now = true; 
}
 
void key_press_continous( const term::KeyStatus &key) { 
  if (key.position != term::KeyPosition::pressed) 
      return ;

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

void key_press_only_once(const term::KeyStatus &key) { 
   if (key.position != term::KeyPosition::released) return ; 
   
  if (key.key == 'q' && key.alt == true) {
    close_app( RequestReason::User ); 
    return ;
   }

  if(key.key == std::to_underlying(term::KeyCodes::HOME)){ 
      editor_globals.view.line_home();
      refresh_screen();
      return;
}

if( key.key == std::to_underlying(term::KeyCodes::END)) { 
      editor_globals.view.line_end();
      refresh_screen();
      return;
}
  

  if (key.key == 'c' && key.alt == true) {
    refresh_screen();
    return ;
  }


  return;
}



// Returns false to indicate quitting
bool process_key_presses(const term::KeyStatus &key) {
 
  key_press_only_once(key); 
  


   key_press_continous(key);

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
  editor_globals.view.set_window(RowSize{tc.height()}, ColSize{ tc.width()});

  editor_globals.text.append_row("Hello text editor world"sv);

  while (!editor_globals.quit_now) {
    tc.on_loop();
    if (tc.had_key_event()) {
       process_key_presses(tc.get_key_event());
    }
  }

  return 0;
}
