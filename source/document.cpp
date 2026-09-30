#include "document.hpp"
#include <fstream>

Document::Errors Document::read_file(std::filesystem::path file) {

  std::fstream f{file, std::ios_base::in};
  if (!f.is_open())
    return Errors::file_does_not_exist;

  std::string line;
  while (!f.eof()) {
    std::getline(f, line);
    m_buffer.rows.emplace_back(std::move(line));
  }
  m_file = file;
  m_is_file_empty = false;

  m_buffer.clear_dirty();
  return Errors::no_error;
}
