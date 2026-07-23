#pragma once
#include "types.hpp"
#include <optional>
#include <string_view>

namespace term::parsers {

constexpr bool is_mouse_protocol(const std::string_view buffer) noexcept {
  // const std::uint16_t value = (std::uint16_t)(('\e' << 16) | ('[' << 8) |
  // '<'); return value ==
  //        ((*reinterpret_cast<const std::uint16_t *>(buffer.data())) &
  //        0xFFFFFF);

  if (buffer.size() <= 2)
    return false;
  auto start = buffer.begin();
  return *start == '\e' && *(start + 1) == '[' && *(start + 2) == '<';
}

term::MouseStatus mouse(const std::string_view buffer) noexcept;
term::KeyStatus key(const std::string_view buffer) noexcept;
std::optional<Position> cursor_position(const std::string_view buffer) noexcept;

} // namespace term::parsers
