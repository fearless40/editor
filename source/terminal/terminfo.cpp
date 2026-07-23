#include "terminfo.hpp"
#include "parsers.hpp"
#include <cstdio>
#include <iostream>
#include <unistd.h>

namespace term::info {

std::optional<Position> get_cursor_position() {

  std::puts("\e[6n");

  write(STDOUT_FILENO, "\e[6n", 4);
  unsigned int i = 0;

  char buf[32];
  std::cout << "Reading input:\n";
  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) {
      std::cout << "Could not read.";
      break;
    }
    if (buf[i] == 'R')
      break;
    ++i;
    std::cout << (char)buf[i] << " i=" << i << '\n';
  }

  const std::string_view buffer{buf, i};

  return parsers::cursor_position(buffer);
}
} // namespace term::info
