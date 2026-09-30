// #include "version.hpp"
#include "cursor.hpp"
#include "dynamiccommandbuffer.hpp"
#include "enum.hpp"
#include "render.hpp"
#include "term_control.hpp"
#include "textbuffer.hpp"
#include "textbufferview.hpp"
#include "types.hpp"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <ios>
#include <iostream>
#include <iterator>
#include <memory>
#include <ranges>
#include <string_view>
#include <utility>
#include <vector>

struct Document {
  enum class Errors { no_error = 0, file_does_not_exist, other_error };

  TextBuffer m_buffer;
  std::filesystem::path m_file;

  bool m_is_file_empty{true};

  constexpr bool dirty() const { return m_buffer.dirty(); }

  Errors read_file(std::filesystem::path file) {

    std::fstream f{file, std::ios_base::in};
    if (!f.is_open())
      return Errors::file_does_not_exist;

    std::string line;
    while (!f.eof()) {
      std::getline(f, line);
      m_buffer.rows.emplace_back(std::move(line));
    }
    m_file = file;
    m_is_file_empty = false;

    m_buffer.clear_dirty();
    return Errors::no_error;
  }
};

struct DocumentView {

  // Ows the view but views are moveable and can be allocated within the vector
  TextBufferView view;

  // Non owning pointer
  Document *document;

  DocumentView(Document *doc) : document(doc), view(doc->m_buffer) {}
};

struct EditorGlobals {
  term::Row rows;
  term::Col cols;
  term::Row cr;
  term::Col cc;
  TextBuffer text;
  TextBufferView view{text};
  bool quit_now{false};
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

struct DocumentManager {
  std::vector<std::unique_ptr<Document>> documents;
  // Invariant assertion there is never a non active view. A view is always
  // created at startup

  Document *create_empty_document() {
    auto ptr = std::make_unique<Document>();
    documents.push_back(std::move(ptr));
    return documents.back().get();
  }

  Document *load(std::filesystem::path filepath) {
    if (auto ptr = std::make_unique<Document>();
        ptr->read_file(filepath) == Document::Errors::no_error) {
      documents.push_back(std::move(ptr));
      return documents.back().get();
    };

    return create_empty_document();
  }
};

DocumentManager g_DocManager;
DocumentViewManager g_DocViews;

enum class SpecialKeys : std::uint8_t {
  alt = 0b1,
  ctrl = 0b10,
  shift = 0b100,
  super = 0b1000,
  left = 0b10000,
  right = 0b100000
};

MAKE_ENUM_FLAG(SpecialKeys)

class KeyMapping {
public:
  enum struct EventContinue { consume, resume };
  enum struct Repeatability { single, repeat };

  using keyFN = std::function_ref<EventContinue(const term::KeyStatus &key,
                                                TextBufferView &view)>;

private:
  struct Mapping {
    int keyCode;
    Repeatability status;
    keyFN fn;
  };

  struct key_map {
    unsigned int key;
    unsigned int index;
  };

  constexpr unsigned int make_key_integer(int keycode,
                                          SpecialKeys flags) const {
    return (keycode & 0x00FFFFFF) | ((std::to_underlying(flags) & 0xFF) << 24);
  }

  constexpr SpecialKeys from_key_status(term::KeyStatus status) const {
    using util::flags::operator|;
    SpecialKeys k{0};
    if (status.alt)
      k = k | SpecialKeys::alt;
    if (status.ctl)
      k = k | SpecialKeys::ctrl;
    if (status.shift)
      k = k | SpecialKeys::shift;
    if (status.super)
      k = k | SpecialKeys::super;
    return k;
  }

  std::vector<key_map> continous_map;
  std::vector<key_map> release_map;
  std::vector<keyFN> functions;

  template <typename Self>
  constexpr auto &select_map(this Self &&self, term::KeyPosition pos) {
    return pos == term::KeyPosition::pressed
               ? std::forward<Self>(self).continous_map
               : std::forward<Self>(self).release_map;
  }
  template <typename Self>
  constexpr auto &select_map(this Self &&self, Repeatability pos) {
    return pos == Repeatability::repeat ? std::forward<Self>(self).continous_map
                                        : std::forward<Self>(self).release_map;
  }

  unsigned int create_fn_index(keyFN &&fn) {
    functions.emplace_back(std::forward<keyFN>(fn));
    auto last = functions.end() - 1;
    if (last == functions.begin())
      return 0;
    return static_cast<unsigned int>(std::distance(functions.begin(), last));
  }

public:
  bool key_event(const term::KeyStatus &key, TextBufferView &view) const {
    auto keyID = make_key_integer(key.key, from_key_status(key));
    auto &vec = select_map(key.position);
    for (const auto &[index, map] : std::views::enumerate(vec)) {
      if (map.key == keyID) {
        if (functions[map.index](key, view) == EventContinue::consume)
          return true;
      }
    }

    return false;
  }

private:
  void _add_key(keyFN &&keyfn, Repeatability repeat, unsigned int key_integer) {
    auto fn_index = create_fn_index(std::forward<keyFN>(keyfn));
    auto &vec = select_map(repeat);
    vec.emplace_back(key_integer, fn_index);
  }

public:
  void add_key(term::KeyCodes code, keyFN &&keyfn, SpecialKeys flags,
               Repeatability repeat = Repeatability::single) {
    _add_key(std::forward<keyFN>(keyfn), repeat,
             make_key_integer(std::to_underlying(code), flags));
  }

  void add_key(char ascii, keyFN &&keyfn, SpecialKeys flags,
               Repeatability repeat = Repeatability::single) {
    _add_key(std::forward<keyFN>(keyfn), repeat,
             make_key_integer(0xFF & ascii, flags));
  }
};

EditorGlobals editor_globals;
KeyMapping key_map;

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
  buff.add((unsigned int)view.buffer().line_length(editor_globals.view.row()));

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

  // std::cout << "Hello from the battleship program!\n";
  // std::cout << "Version: " << Version::MAJOR_VERSION << "."
  //           << Version::MINOR_VERSION << '\n';

  // compositor_test();

  term::TermControl tc{};

  if (argv >= 2) {
    if (!g_DocManager.load(argc[1]))
      return -1;
  }

  g_DocViews.create_view(g_DocManager.documents.back().get(), tc.height() - 1,
                         tc.width());

  editor_globals.rows = term::Row{tc.height()};
  editor_globals.cols = term::Col{tc.width()};

#define key_once(key_code, code, specialkeys)                                  \
  key_map.add_key((key_code),                                                  \
                  [](const term::KeyStatus &key, TextBufferView &view) {       \
                    code;                                                      \
                    return KeyMapping::EventContinue::consume;                 \
                  },                                                           \
                  specialkeys, KeyMapping::Repeatability::single);

#define key_many(key_code, code, specialkeys)                                  \
  key_map.add_key((key_code),                                                  \
                  [](const term::KeyStatus &key, TextBufferView &view) {       \
                    code;                                                      \
                    return KeyMapping::EventContinue::consume;                 \
                  },                                                           \
                  specialkeys, KeyMapping::Repeatability::repeat);

  key_map.add_key(
      term::KeyCodes::HOME,
      [](const term::KeyStatus &key, TextBufferView &view) {
        view.line_home();
        return KeyMapping::EventContinue::consume;
      },
      SpecialKeys{0}, KeyMapping::Repeatability::single);

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
