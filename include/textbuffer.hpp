#pragma once

#include "typed_position.hpp"
#include "xy.hpp"
#include <cstddef>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <vector>


struct TextBuffer
{
  using Row = geom::Row;
  using Col = geom::Col;
  std::vector<std::string> rows;
  bool m_dirty{ false };
  [[nodiscard]] std::size_t last_index() const { return empty() ? 0 : rows.size() - 1; }
  [[nodiscard]] constexpr bool empty() const { return rows.empty(); }
  [[nodiscard]] constexpr auto size() const { return std::ssize(rows); }
  [[nodiscard]] constexpr bool dirty() const { return m_dirty; }

  constexpr void update_dirty() { m_dirty = true; }
  constexpr void clear_dirty() { m_dirty = false; }

  [[nodiscard]] constexpr bool is_valid_row(Row row) const { return row.underlying() < size(); }

  [[nodiscard]] constexpr auto line_length(Row row) const
  {
    if (is_valid_row(row)) { return std::ssize(rows[row.underlying()]); }

    return 0L;
  }

  void remove_row(geom::Row row)
  {
    if (!is_valid_row(row)) return;
    rows.erase(rows.begin() + row.underlying());
  }

  void insert_row(const std::string_view data, Row location)
  {
    if (!is_valid_row(location)) {
      append_row(data);
    } else {
      auto it = rows.begin() + location.underlying();
      std::string copy{ data };
      rows.insert(it, std::move(copy));
    }
    update_dirty();
  }

  void modify_row(std::string_view data, Row row)
  {
    if (!is_valid_row(row)) return;
    rows[static_cast<std::size_t>(row.underlying())] = std::string{ data };
    update_dirty();
  }

  void append_row(const std::string_view data)
  {
    rows.emplace_back(data);
    update_dirty();
  }

  void append_row(const std::string_view data, Row row)
  {
    if (!is_valid_row(row)) return;
    auto &string = rows[static_cast<std::size_t>(row.underlying())];
    string.append(data);
    update_dirty();
  }
  const std::optional<std::string_view> get_row(Row row) const
  {
    if (!is_valid_row(row)) return {};
    return { std::string_view{ rows[row.underlying()] } };
  }

  void insert_char(Row row, Col col, char value)
  {
    if (empty() || row.underlying() >= size()) {
      append_row(std::string_view{ &value, 1 });
      update_dirty();
      return;
    }

    if (!is_valid_row(row)) return;

    auto &string = rows[row.underlying()];
    if (col.underlying() >= string.size())
      string.append(&value, 1);
    else
      string.insert(col.underlying(), 1, value);
    update_dirty();
  }

  void remove_char(Row row, Col col)
  {
    if (!is_valid_row(row) || col.underlying() >= line_length(row)) return;


    auto &string = rows[row.underlying()];
    string.erase(col.underlying(), 1);
    update_dirty();
  }
};
