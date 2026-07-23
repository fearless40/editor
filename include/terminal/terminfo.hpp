#pragma once
#include "types.hpp"
#include <optional>

namespace term::info {

std::optional<term::Position> get_cursor_position();

}
