#pragma once

#include "soa.hpp"
#include "soamemorylayout.hpp"
#include "textbuffer.hpp"
#include <filesystem>
#include <memory>
#include <ranges>

struct Document
{

  enum class Errors { no_error = 0, file_does_not_exist, other_error };

  TextBuffer m_buffer;
  std::filesystem::path m_file;

  bool m_is_file_empty{ true };

  [[nodiscard]] constexpr bool dirty() const { return m_buffer.dirty(); }

  Errors read_file(std::filesystem::path file);
};

struct DocumentManager
{
  enum class DocumentType : uint8_t { system, file, ui };
  enum class DocumentID : int {};

  using document_t = std::unique_ptr<Document>;
  using soa_t = util::soa::SOA<util::soa::memory_layout::DynamicArray, document_t, DocumentType, DocumentID>;
  using iterator = typename soa_t::Iterator;
  soa_t m_documents;

  static constexpr bool system_documents(soa_t::reference_t tup) {}

  Document *create_empty_document()
  {
    auto ptr = std::make_unique<Document>();
    m_documents.push_back(std::move(ptr), DocumentType::file, DocumentID{ -1 });
    return m_documents.back<document_t>().get();
  }

  Document *create_system_document(int requested_id)
  {
    m_documents.push_back(std::make_unique<Document>(), DocumentType::system, DocumentID{ requested_id });
    return m_documents.back<document_t>().get();
  }

  Document *get_system_document(int requested_id)
  {
    for (auto item : m_documents | std::views::filter([](auto tup) {
           return std::get<DocumentType &>(tup) == DocumentType::system;
         })) {
      if (std::get<DocumentID &>(item) == DocumentID{ requested_id }) { return std::get<document_t &>(item).get(); }
    }
    return nullptr;
  }

  Document *load(std::filesystem::path filepath)
  {
    if (auto ptr = std::make_unique<Document>(); ptr->read_file(filepath) == Document::Errors::no_error) {
      m_documents.push_back(std::move(ptr), DocumentType::file, DocumentID{ -1 });
      return m_documents.back<document_t>().get();
    };

    return create_empty_document();
  }
};
