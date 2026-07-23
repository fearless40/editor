#pragma once
namespace term::util {

// In place conversion of numbers to text in the given buffer.
// Very simple design and cannot handle anything longer than 4 digits
template <typename CPtr> constexpr CPtr to_chars(CPtr start, int value) {

  if (value > 9999 || value < 0)
    return start;

  int nbrChars = 4;

  if (value <= 9)
    nbrChars = 1;
  else if (value <= 99)
    nbrChars = 2;
  else if (value <= 999)
    nbrChars = 3;
  else if (value <= 9999)
    nbrChars = 4;

  int i = nbrChars;
  do {
    start[--i] = '0' + (value % 10);
    value /= 10;
  } while (value != 0);
  return start + nbrChars;
}
} // namespace term::util
