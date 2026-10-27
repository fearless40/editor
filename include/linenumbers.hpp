#pragma once

#include "digitcount.hpp"
#include "editortypes.hpp"
class LineNumbers
{
  Width min_width{ 3 };
  Width max_width{ 5 };
  Width right_border{ 1 };

public:
  // Code to show a raw view
  constexpr Renderable_Return operator()(const Renderable_Input &input) const
  {
    auto max_row = input.view.window_rows().underlying() + input.view.row_scroll().underlying();
    auto digitCount = util::fast_digit_count(max_row);
    if (input.fmt == OutputFormat::SizeOnly) { return { .written = ColOffset{ digitCount } + right_border }; }

    // This only ever outputs ASCII for now. Other formats pretty easy to add later

    auto start_line_nbr = input.view.row_scroll();
    auto end_line_nbr = input.view.window_rows() + start_line_nbr;
    auto request_width = std::clamp(digitCount, min_width.underlying(), max_width.underlying());

    auto iterator = input.text_buffer.begin();
    std::size_t last_char_size = 0;
    // Generate space
    std::fill(&*iterator, &*(iterator + request_width - digitCount), static_cast<std::byte>(' '));

    // Move the iterator
    iterator += request_width - digitCount;


    // Do integer conversion in place
    auto conres = std::to_chars(reinterpret_cast<char *>(&*(iterator)),
      reinterpret_cast<char *>(&*input.text_buffer.end()),
      input.row.underlying(),
      10);

    if (conres) {
      iterator += digitCount;
      std::fill(&*iterator, &*(iterator + right_border.underlying()), static_cast<std::byte>(' '));
      iterator += right_border.underlying();
    }

    return {
      // clang-format off
      .text_buffer = { input.text_buffer.begin(), iterator},
      .color_buffer = input.color_buffer,
      .written = ColOffset
      { static_cast<ColOffset::underlying_t>(std::distance(input.text_buffer.begin(), iterator)) }
      // clang-format on
    };
  }
};
