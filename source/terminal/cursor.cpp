#include "cursor.hpp"
#include "commandbuffer.hpp"

namespace term::cursor {

void position(term::CommandBuffer &buff, Row r, Col c) {

  buff.add("\e[");
  buff.add(static_cast<unsigned int>(std::to_underlying(r)));
  buff.add(';');
  buff.add(static_cast<unsigned int>(std::to_underlying(c)));
  buff.add('H');
}

Position get_position() { return {Row{0}, Col{0}}; }

} // namespace term::cursor
