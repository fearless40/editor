#pragma once
namespace term::digits {

constexpr auto max_digit() { return 4; };

constexpr unsigned int number_chars_for_integer(unsigned int value) {
  if (value > 9999 || value < 0)
    return 0;

  if (value <= 9)
    return 1;
  else if (value <= 99)
    return 2;
  else if (value <= 999)
    return 3;
  else if (value <= 9999)
    return 4;
  else
    return 10;
};

// In place conversion of numbers to text in the given buffer.
// Very simple design and cannot handle anything longer than 4 digits
template <typename CPtr> constexpr CPtr to_chars(CPtr start, int value) {

  auto nbrChars = number_chars_for_integer(value);
  int i = nbrChars;
  do {
    start[--i] = '0' + (value % 10);
    value /= 10;
  } while (value != 0);
  return start + nbrChars;
}
} // namespace term::digits
