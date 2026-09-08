#pragma once 

#include <vector>
#include <string>

struct TextBuffer {
  std::vector<std::string> rows;
  std::size_t number_rows() const { return rows.size(); }
  constexpr bool empty() const { return rows.empty(); }

  constexpr std::size_t line_length(unsigned row) const {
    if (row >= rows.size())
      return 0;
    return rows[row].length();
  }

  void append_row(std::string_view data) { rows.emplace_back(data); }
};
