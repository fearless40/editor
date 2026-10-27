#pragma once

#include "textbufferview.hpp"
#include "xy.hpp"
#include <cstdint>
#include <span>

using geom::Row;
using geom::Col;
using RowOffset = geom::Height;
using ColOffset = geom::Width;
using geom::Height;
using geom::Width;


class TextBufferView;

struct ByteColorData
{
  unsigned int start;
  unsigned int end;
};


enum class OutputFormat : std::uint8_t { SizeOnly, ASCII, UTF8, UTF16, UTF32 };
struct Renderable_Input
{
  std::span<std::byte> text_buffer;
  std::span<ByteColorData> color_buffer;
  const TextBufferView &view;
  Row row;
  OutputFormat fmt;
};


struct Renderable_Return
{
  // New span points at what was written.
  void constexpr bytes_written(std::span<std::byte> initial, int bytes_written)
  { text_buffer = { initial.begin(), initial.begin() + bytes_written }; }
  std::span<std::byte> text_buffer;
  std::span<ByteColorData> color_buffer;
  ColOffset written;// Number of characters written. In ASCII one to one ration, in UTF8 a character can be represetned
                    // by multiple ways
};


struct ThemeIndex
{ unsigned int idx; };

struct ColorData
{
  Col start;
  ColOffset end;
  ThemeIndex index;
};
