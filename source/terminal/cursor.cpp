#include "cursor.hpp"
#include "commandbuffer.hpp"
#include "parsers.hpp"
#include <optional>

namespace term::cursor {

void position(term::CommandBuffer &buff, Row r, Col c) {

  buff.CSI();
  buff.add(static_cast<unsigned int>(std::to_underlying(r)));
  buff.add(';');
  buff.add(static_cast<unsigned int>(std::to_underlying(c)));
  buff.add('H');
}

std::optional<Position> get_position() {

  SizedCommandBuffer<8> command;
  SizedCommandBuffer<32> response;
  command.CSI();
  command.add("6n");

  if (!CommandBuffer::read_response(command, response))
    std::puts("Error could not read the cursor position.");

  return parsers::cursor_position(response.as_string_view());
}
} // namespace term::cursor
