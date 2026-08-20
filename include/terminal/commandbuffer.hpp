#pragma once

#include "utility.hpp"
#include <cstring>
#include <string_view>
#include <unistd.h>
namespace term {

struct CommandBuffer {
  virtual ~CommandBuffer() = default;
  virtual bool add(std::string_view command) = 0;
  virtual bool add(char) = 0;
  virtual bool add(unsigned int) = 0;
  virtual void submit_and_clear() = 0;
  virtual void submit() = 0;
  virtual void clear() = 0;
  constexpr bool CSI() { return add("\x1b["); }
};

template <unsigned int BUFFSIZE> struct SizedCommandBuffer : CommandBuffer {
  char data[BUFFSIZE];
  unsigned char write_position{0};

  constexpr void clear() { write_position = 0; }

  constexpr void adjust_positions(unsigned int len) { write_position += len; }

  // Auto submits on your behalf if buffer is full
  constexpr bool add(const std::string_view command) {
    if (command.length() + write_position >= BUFFSIZE) {
      submit_and_clear();

      if (command.length() >= BUFFSIZE) {
        write(STDOUT_FILENO, command.data(), command.length());
        return true;
      }
    }

    std::memcpy(&data[write_position], command.data(), command.length());
    adjust_positions(command.length());
    return true;
  }

  constexpr bool add(char c) {
    if (write_position + 1 >= BUFFSIZE)
      return false;
    data[write_position] = c;
    adjust_positions(1);
    return true;
  }

  constexpr bool add(unsigned int number) {
    if (write_position + 4 > BUFFSIZE) {
      return false;
    }

    auto endWrite = digits::to_chars(&data[write_position], number);

    auto length = static_cast<unsigned int>(
        std::distance(&data[write_position], endWrite));

    adjust_positions(length);

    return true;
  }

  void submit_and_clear() {
    submit();
    clear();
  }
  void submit() { write(STDOUT_FILENO, &data, write_position); }
};

} // namespace term
