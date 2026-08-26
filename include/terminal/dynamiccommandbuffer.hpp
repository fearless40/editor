#pragma once
#include "commandbuffer.hpp"
#include <charconv>
#include <string>
#include <system_error>

namespace term {
struct DynamicCommandBuffer : CommandBuffer {
  std::string m_data{};

  constexpr std::string_view as_string_view() const override {
    return std::string_view{m_data};
  }

  constexpr char *data() override { return m_data.data(); }

  constexpr void clear() override { m_data.clear(); }

  constexpr std::size_t capacity() const override { return m_data.capacity(); }

  constexpr std::size_t size() const override { return m_data.size(); }

  // Auto submits on your behalf if buffer is full
  constexpr bool add(const std::string_view command) override {
    m_data.append(command);
    return true;
  }

  constexpr bool add(char c) override {
    m_data.push_back(c);
    return true;
  }

  constexpr bool add(unsigned int number) override {
    char buff[20];
    auto [ptr, ec] = std::to_chars(buff, buff + sizeof(buff), number);
    if (ec == std::errc())
      m_data.append(buff, ptr);
    return true;
  }

  void submit_and_clear() override {
    submit();
    clear();
  }
  void submit() const override { CommandBuffer::write(m_data.subview()); }
};
} // namespace term
