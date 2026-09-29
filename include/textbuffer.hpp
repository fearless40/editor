#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

struct TextBuffer {
  std::vector<std::string> rows;
  bool m_dirty{false};
  std::size_t last_index() const { return empty() ? 0 : rows.size() - 1; }
  constexpr bool empty() const { return rows.empty(); }
  constexpr std::size_t size() const { return rows.size(); }
  constexpr bool dirty() const { return m_dirty; }

  constexpr void update_dirty() { m_dirty = true; }
  constexpr void clear_dirty() { m_dirty = false; }

  constexpr bool is_valid_row(std::size_t row) const {
    return row >= 0 and row < size();
  }

  constexpr std::size_t line_length(unsigned row) const {
    if (row < rows.size())
      return rows[row].length();

    return 0;
  }

  void remove_row(std::size_t row) {
    if (!is_valid_row(row))
      return;
    rows.erase(rows.begin() + row);
  }

  void insert_row(const std::string_view data, std::size_t location) {
    if (!is_valid_row(location)) {
      append_row(data);
    } else {
      auto it = rows.begin() + location;
      std::string copy{data};
      rows.insert(it, std::move(copy));
    }
    update_dirty();
  }

  void modify_row(std::string_view data, std::size_t row) {
    if (!is_valid_row(row))

      return;
    rows[row] = std::string{data};
    update_dirty();
  }

  void append_row(const std::string_view data) {
    rows.emplace_back(data);
    update_dirty();
  }

  void append_row(const std::string_view data, std::size_t row) {
    if (!is_valid_row(row))
      return;
    auto &string = rows[row];
    string.append(data);
    update_dirty();
  }
  const std::optional<std::string_view> get_row(std::size_t row) const {
    if (!is_valid_row(row))
      return {};
    return {std::string_view{rows[row]}};
  }

  void insert_char(std::size_t row, std::size_t col, char value) {
    if (empty() || row >= size()) {
      append_row(std::string_view{&value, 1});
      update_dirty();
      return;
    }

    if (!is_valid_row(row))
      return;

    auto &string = rows[row];
    if (col >= string.size())
      string.append(&value, 1);
    else
      string.insert(col, 1, value);
    update_dirty();
  }

  void remove_char(std::size_t row, std::size_t col) {
    if (!is_valid_row(row) || col >= line_length(row))
      return;

    auto &string = rows[row];
    string.erase(col, 1);
    update_dirty();
  }
};
