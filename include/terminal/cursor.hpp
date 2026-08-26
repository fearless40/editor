#pragma once
#include "commandbuffer.hpp"
#include "types.hpp"
#include <optional>
namespace term::cursor {

using namespace std::string_view_literals;

constexpr void reset_position(CommandBuffer &buff) {
  buff.CSI();
  buff.add('H');
};

constexpr void off(CommandBuffer &buff) {
  buff.CSI();
  buff.add("25?l"sv);
};

constexpr void on(CommandBuffer &buff) {
  buff.CSI();
  buff.add("?25h"sv);
}

constexpr void reset_position() { CommandBuffer::write("\e[H"sv); };

void position(CommandBuffer &buff, Row r, Col c);

std::optional<Position> get_position();

constexpr void position(unsigned int line, unsigned int col) {
  SizedCommandBuffer<32> command;
  command.CSI();
  command.add(line);
  command.add(';');
  command.add(col);
  command.add('H');
  command.submit();
}

constexpr void relative(unsigned int value, char motion) {
  SizedCommandBuffer<32> command;
  command.CSI();
  command.add(value);
  command.add(motion);
  command.submit();
}

constexpr void up(unsigned int amount) { relative(amount, 'A'); }

constexpr void down(unsigned int amount) { relative(amount, 'B'); }

constexpr void right(unsigned int amount) { relative(amount, 'C'); }

constexpr void left(unsigned int amount) { relative(amount, 'D'); }

constexpr void down_front(unsigned int amount) { relative(amount, 'E'); }

constexpr void up_front(unsigned int amount) { relative(amount, 'F'); }

constexpr void column(int amount) { relative(amount, 'G'); }

} // namespace term::cursor
