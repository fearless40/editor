#pragma once

#include "digitcount.hpp"
#include "editortypes.hpp"
class LineNumbers
{
  Width min_width{ 3 };
  Width max_width{ 5 };

public:
  // Code to show a raw view
  constexpr Renderable_Return operator()(const Renderable_Input &input) const
  {
    auto max_row = input.view.window_rows().underlying() + input.view.row_scroll().underlying();
    auto digitCount = util::fast_digit_count(max_row);
    if (input.fmt == OutputFormat::SizeOnly) { return { .char_written = ColOffset{ digitCount } }; }

    // This only ever outputs ASCII for now. Other formats pretty easy to add later

    auto start_line_nbr = input.view.row_scroll();
    auto end_line_nbr = input.view.window_rows() + start_line_nbr;
    auto request_width = std::clamp(digitCount, min_width.underlying(), max_width.underlying());

    auto iterator = input.text_buffer.begin();
    // Generate space
    std::fill(&*iterator, &*(iterator + request_width - digitCount), static_cast<std::byte>(' '));

    // Move the iterator
    iterator += request_width - digitCount;

    constexpr const auto BASE_10 = 10;

    // Do integer conversion in place
    auto conres = std::to_chars(reinterpret_cast<char *>(&*iterator),
      reinterpret_cast<char *>(&*input.text_buffer.end()),
      input.row.underlying(),
      BASE_10);

    if (conres) { iterator += digitCount; }

    return {
      // clang-format off
      .text_buffer = {iterator, input.text_buffer.end()},
      .color_buffer = input.color_buffer,
      .char_written = request_width,
      // clang-format on
    };
  }
};
