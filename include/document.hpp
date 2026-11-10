#pragma once

#include "soa.hpp"
#include "soamemorylayout.hpp"
#include "textbuffer.hpp"
#include <filesystem>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>

// Holds layers of text, with a single TextBuffer as the primary interface
struct Document
{

  enum class Errors { no_error = 0, file_does_not_exist, other_error };

  TextBuffer m_buffer;


  [[nodiscard]] constexpr bool dirty() const { return m_buffer.dirty(); }

  Errors read_file(std::filesystem::path file);
};

struct DocumentManager
{
  enum class DocumentType : uint8_t { system, file, ui };

  using document_t = std::unique_ptr<Document>;
  using soa_t = util::soa::SOA<util::soa::memory_layout::DynamicArray, document_t, DocumentType, std::string>;
  using iterator = typename soa_t::Iterator;
  soa_t m_documents;

  static constexpr bool system_documents(soa_t::reference_t tup) { return false; }

  [[nodiscard]] constexpr auto back() { return m_documents.back<document_t>().get(); }

  Document *create_empty_document()
  {
    auto ptr = std::make_unique<Document>();
    m_documents.push_back(std::move(ptr), DocumentType::file, "unamed");
    return m_documents.back<document_t>().get();
  }

  std::string_view get_name(const Document *doc)
  {
    for (auto docData : m_documents) {
      if (std::get<document_t &>(docData).get() == doc) { return std::get<std::string &>(docData); }
    }
    return "";
  }

  Document *create_system_document(const std::string_view name)
  {
    m_documents.push_back(std::make_unique<Document>(), DocumentType::system, std::string{ name });
    return m_documents.back<document_t>().get();
  }

  Document *get_system_document(std::string_view name)
  {
    for (auto item : m_documents | std::views::filter([](auto tup) {
           return std::get<DocumentType &>(tup) == DocumentType::system;
         })) {
      if (std::get<std::string &>(item) == name) { return std::get<document_t &>(item).get(); }
    }
    return nullptr;
  }

  Document *load(std::filesystem::path filepath)
  {
    if (auto ptr = std::make_unique<Document>(); ptr->read_file(filepath) == Document::Errors::no_error) {
      m_documents.push_back(std::move(ptr), DocumentType::file, filepath.filename());
      return m_documents.back<document_t>().get();
    };

    return create_empty_document();
  }
};
