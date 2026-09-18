#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

struct TextBuffer {
  std::vector<std::string> rows;
  std::size_t number_rows() const { return rows.size() - 1; }
  constexpr bool empty() const { return rows.empty(); }

  constexpr bool is_valid_row(std::size_t row) const {
    return row >= 0 and row < number_rows();
  }

  constexpr std::size_t line_length(unsigned row) const {
    if (row >= rows.size())
      return 0;
    return rows[row].length();
  }

  void remove_row(std::size_t row) {
    if (!is_valid_row(row))
      return;
    rows.erase(rows.begin() + row);
  }

  void insert_row(std::string_view data, std::size_t location) {
    if (!is_valid_row(location)) {
      append_row(data);
    } else {
      auto it = rows.begin() + location;
      rows.insert(it, std::move(std::string(data)));
    }
  }

  void modify_row(std::string_view data, std::size_t row) {
    if (!is_valid_row(row))

      return;
    rows[row] = std::string{data};
  }

  void append_row(std::string_view data) { rows.emplace_back(data); }

  void append_row(std::string_view data, std::size_t row) {
    if (!is_valid_row(row))
      return;
    auto &string = rows[row];
    string.append(data);
  }
  const std::optional<std::string_view> get_row(std::size_t row) const {
    if (!is_valid_row(row))
      return {};
    return {std::string_view{rows[row]}};
  }

  void insert_char(std::size_t row, std::size_t col, char value) {
    if (number_rows() == 0) {
      append_row(std::string_view{&value, 1});
      return;
    }

    if (!is_valid_row(row))
      return;
    if (row == number_rows()) {
      rows.push_back(std::string{value});
      return;
    }

    auto &string = rows[row];
    string.insert(col, 1, value);
  }

  void remove_char(std::size_t row, std::size_t col) {
    if (!is_valid_row(row) && col >= line_length(row))
      return;

    auto &string = rows[row];
    string.erase(col, 1);
  }
};
