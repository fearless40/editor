#include "render.hpp"
#include "utility.hpp"

namespace term {

void set_cursor_position(Row row, Col col) {
  char buf[16] = "\e[";
  auto next = util::to_chars(&buf[3], std::to_underlying(row));
  next = util::to_chars(next, std::to_underlying(col));
  *++next = 'H';
  *++next = '\0';
  std::puts(buf);
}
} // namespace term
