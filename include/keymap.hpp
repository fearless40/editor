#pragma once

#include "enum.hpp"
#include "types.hpp"
#include <algorithm>
#include <functional>
#include <utility>

struct TextBufferView;

enum class SpecialKeys : std::uint8_t {
  alt = 0b1,
  ctrl = 0b10,
  shift = 0b100,
  super = 0b1000,
  left = 0b10000,
  right = 0b100000
};

MAKE_ENUM_FLAG(SpecialKeys)

class KeyMap
{
public:
  enum struct EventContinue { consume, resume };
  enum struct Repeatability { single, repeat };

  using keyFN = std::copyable_function<EventContinue(const term::KeyStatus &key, TextBufferView &view)>;

private:
  struct Mapping
  {
    int keyCode;
    Repeatability status;
    keyFN fn;
  };

  struct key_map
  {
    unsigned int key;
    unsigned int index;
  };

  [[nodiscard]] static constexpr unsigned int make_key_integer(int keycode, SpecialKeys flags)
  { return (keycode & 0x00FFFFFF) | ((std::to_underlying(flags) & 0xFF) << 24); }

  [[nodiscard]] static constexpr SpecialKeys from_key_status(term::KeyStatus status)
  {
    using util::flags::operator|;
    SpecialKeys k{ 0 };
    if (status.alt) k = k | SpecialKeys::alt;
    if (status.ctl) k = k | SpecialKeys::ctrl;
    if (status.shift) k = k | SpecialKeys::shift;
    if (status.super) k = k | SpecialKeys::super;
    return k;
  }

  std::vector<key_map> continous_map;
  std::vector<key_map> release_map;
  std::vector<keyFN> functions;
  keyFN m_else;

  template<typename Self> constexpr auto &select_map(this Self &&self, term::KeyPosition pos)
  {
    return pos == term::KeyPosition::pressed ? std::forward<Self>(self).continous_map
                                             : std::forward<Self>(self).release_map;
  }
  template<typename Self> constexpr auto &select_map(this Self &&self, Repeatability pos)
  {
    return pos == Repeatability::repeat ? std::forward<Self>(self).continous_map : std::forward<Self>(self).release_map;
  }

  unsigned int create_fn_index(keyFN &&fn)
  {
    functions.emplace_back(std::forward<keyFN>(fn));
    auto last = functions.end() - 1;
    if (last == functions.begin()) return 0;
    return static_cast<unsigned int>(std::distance(functions.begin(), last));
  }

public:
  bool key_event(const term::KeyStatus &key, TextBufferView &view)
  {
    auto keyID = make_key_integer(key.key, from_key_status(key));
    auto &vec = select_map(key.position);
    for (const auto &map : vec) {
      if (map.key == keyID) {
        if (functions[map.index](key, view) == EventContinue::consume) return true;
      }
    }

    if (m_else) {
      auto con = m_else(key, view);
      if (con == EventContinue::consume) return true;
    }

    return false;
  }

private:
  void _add_key(keyFN &&keyfn, Repeatability repeat, unsigned int key_integer)
  {
    // Allow overwriting existing keys

    auto &vec = select_map(repeat);
    auto fit = std::ranges::find(vec, key_integer, &key_map::key);
    if (fit != vec.end()) {
      functions[std::distance(vec.begin(), fit)] = std::forward<keyFN>(keyfn);
    } else {
      auto fn_index = create_fn_index(std::forward<keyFN>(keyfn));
      auto &vec = select_map(repeat);
      vec.emplace_back(key_integer, fn_index);
    }
  }

public:
  void add_key(term::KeyCodes code, keyFN &&keyfn, SpecialKeys flags, Repeatability repeat = Repeatability::single)
  { _add_key(std::forward<keyFN>(keyfn), repeat, make_key_integer(std::to_underlying(code), flags)); }

  void add_key(char ascii, keyFN &&keyfn, SpecialKeys flags, Repeatability repeat = Repeatability::single)
  { _add_key(std::forward<keyFN>(keyfn), repeat, make_key_integer(0xFF & ascii, flags)); }

  void on_nomatch(keyFN &&keyfn) { m_else = std::forward<keyFN>(keyfn); }
};
