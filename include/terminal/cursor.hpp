#pragma once
#include "commandbuffer.hpp"
#include "types.hpp"
#include <cstdio>
#include <utility>
namespace term::cursor {

inline void reset_position(CommandBuffer &buff) {
  buff.CSI();
  buff.add('H');
};

inline void reset_position() { std::fputs("\e[H", stdout); };

void position(CommandBuffer &buff, Row r, Col c);

std::pair<Row, Col> get_position();

} // namespace term::cursor
