#pragma once

#include "textbuffer.hpp"
#include <filesystem>
#include <memory>

struct Document {

  enum class Errors { no_error = 0, file_does_not_exist, other_error };

  TextBuffer m_buffer;
  std::filesystem::path m_file;

  bool m_is_file_empty{true};

  constexpr bool dirty() const { return m_buffer.dirty(); }

  Errors read_file(std::filesystem::path file);
};

struct DocumentManager {
  std::vector<std::unique_ptr<Document>> documents;

  Document *create_empty_document() {
    auto ptr = std::make_unique<Document>();
    documents.push_back(std::move(ptr));
    return documents.back().get();
  }

  Document *load(std::filesystem::path filepath) {
    if (auto ptr = std::make_unique<Document>();
        ptr->read_file(filepath) == Document::Errors::no_error) {
      documents.push_back(std::move(ptr));
      return documents.back().get();
    };

    return create_empty_document();
  }
};
