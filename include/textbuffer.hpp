#pragma once

#include <cstddef>
#include <string>
#include <vector>

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

  void insert_char(std::size_t row, std::size_t col, char value) {
    if (row < 0 || row > number_rows())
      return;
    if (row == number_rows()) {
      rows.push_back(std::string{value});
      return;
    }

    auto &string = rows[row];
    string.insert(col, 1, value);
  }
};
