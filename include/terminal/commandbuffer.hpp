#pragma once

#include "utility.hpp"
#include <cstddef>
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
  virtual void submit() const = 0;
  virtual void clear() = 0;
  virtual std::string_view as_string_view() const = 0;
  virtual std::size_t size() const = 0;
  virtual std::size_t max_size() const = 0;
  constexpr bool CSI() { return add("\x1b["); }

  static constexpr void write(std::string_view data) {
    ::write(STDOUT_FILENO, data.data(), data.length());
  }

  // Will always clear the response buffer
  static constexpr bool read_response(const CommandBuffer &command,
                                      CommandBuffer &response) {

    command.submit();

    bool success{false};
    response.clear();
    char value;
    while (response.size() < response.max_size()) {
      if (::read(STDIN_FILENO, &value, 1) != 1)
        break;

      response.add(value);
    }
    if (response.size() == 0)
      return false;

    return true;
  }
};

template <unsigned char BUFFSIZE> struct SizedCommandBuffer : CommandBuffer {
  char data[BUFFSIZE];
  unsigned char write_position{0};

  constexpr std::string_view as_string_view() const override {
    return std::string_view{data, static_cast<std::size_t>(write_position) - 1};
  }

  constexpr void clear() override { write_position = 0; }

  constexpr std::size_t max_size() const override { return BUFFSIZE; }

  constexpr std::size_t size() const override { return write_position; }

  constexpr void adjust_positions(unsigned int len) { write_position += len; }

  // Auto submits on your behalf if buffer is full
  constexpr bool add(const std::string_view command) override {
    if (command.length() + write_position >= BUFFSIZE) {
      submit_and_clear();

      if (command.length() >= BUFFSIZE) {
        ::write(STDOUT_FILENO, command.data(), command.length());
        return true;
      }
    }

    std::memcpy(&data[write_position], command.data(), command.length());
    adjust_positions(command.length());
    return true;
  }

  constexpr bool add(char c) override {
    if (write_position + 1 >= BUFFSIZE)
      return false;
    data[write_position] = c;
    adjust_positions(1);
    return true;
  }

  constexpr bool add(unsigned int number) override {
    if (write_position + 4 > BUFFSIZE) {
      return false;
    }

    auto endWrite = digits::to_chars(&data[write_position], number);

    auto length = static_cast<unsigned int>(
        std::distance(&data[write_position], endWrite));

    adjust_positions(length);

    return true;
  }

  void submit_and_clear() override {
    submit();
    clear();
  }
  void submit() const override {
    ::write(STDOUT_FILENO, &data, write_position);
  }
};

} // namespace term
