#include "parsers.hpp"
#include <cctype>
#include <charconv>
#include <cstring>

// Extracts digits while it reads a digit or encounters a sentinal value
// Leaves the value on the sentinal or nondigit
template <typename Iterator, typename Sentinal>
static int digits(Iterator &it, const Sentinal S) {
  auto start = it;
  while (it != S and isdigit(*it)) {
    ++it;
  };

  int value{-1};
  std::from_chars(&*start, &*it, value);
  return value;
}

template <typename Iterator, typename Sentinal>
static void next_match(char letter, Iterator &start, const Sentinal S) {
  while (start != S) {
    if (*start == letter)
      return;
    ++start;
  }
}

// Moves the iterator the to next valid character
template <typename Iterator, typename Sentinal>
static bool validate_escape(Iterator &start, const Sentinal S) {
  if (start != S and *start != '\e')
    return false;
  if (start != S and *++start != '[')
    return false;
  if (++start != S)
    return true;

  return false;
}

namespace term::parsers {

KeyStatus key(const std::string_view buffer) noexcept {

  // Key protocol looks like
  // CSI unicode-key-code:alternate-key-codes ; modifiers:event-type ;
  // text-as-codepoints u
  KeyStatus k;
  std::memset((void *)&k, 0, sizeof(KeyStatus));
  k.position = KeyPosition::pressed;

  // Verfiy buffer is big enough
  if (buffer.size() < 3)
    return {};

  auto start = buffer.begin();

  if (!validate_escape(start, buffer.end()))
    return k;

  k.key = digits(start, buffer.end());

  next_match(';', start, buffer.end());

  if (*start != ';')
    return k;

  int modifier = digits(++start, buffer.end()) - 1;

  if (modifier > 0) {

    if (modifier & 0b1)
      k.shift = true;
    if (modifier & 0b10)
      k.alt = true;
    if (modifier & 0b100)
      k.ctl = true;
    if (modifier & 0b1000)
      k.super = true;
  }

  if (*start != ':') {
    k.position = KeyPosition::pressed;

    return k;
  }

  ++start;
  switch (*start) {
  case '1':
    k.position = KeyPosition::pressed;
    break;
  case '2':
    k.position = KeyPosition::repeat;
    break;
  case '3':
    k.position = KeyPosition::released;
    break;
  };

  return k;
}

MouseStatus mouse(const std::string_view buffer) noexcept {

  MouseStatus ms;
  std::memset(&ms, 0, sizeof(MouseStatus));

  auto start = buffer.begin();
  if (!validate_escape(start, buffer.end()))
    return ms;

  if (*++start != '<')
    return ms;

  auto btn = digits(++start, buffer.end());
  if (*start != ';')
    return ms;
  auto row = digits(++start, buffer.end());
  if (*start != ';')
    return ms;
  auto col = digits(++start, buffer.end());
  char state = *start;

  ms.button = static_cast<MouseStatus::Button>(btn & 0b11000011);
  ms.row = row - 1;
  ms.col = col - 1;
  ms.press = state == 'M';
  ms.release = state == 'm';
  ms.shift = btn & 0b100;
  ms.alt = btn & 0b1000;
  ms.ctrl = btn & 0b10000;

  return ms;
}

std::optional<Position>
cursor_position(const std::string_view buffer) noexcept {

  auto start = buffer.begin();
  if (!validate_escape(start, buffer.end()))
    return {};

  auto row = Row{digits(start, buffer.end())};
  // Verify ;
  if (*start != ';')
    return {};

  auto col = Col{digits(++start, buffer.end())};

  return {Position{row, col}};
}
} // namespace term::parsers
