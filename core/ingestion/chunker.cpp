#include "chunker.hpp"
#include <sstream>
#include <algorithm>

namespace isha {

std::vector<IngestionChunk> BasicWordChunker::chunkText(const std::string& text, size_t chunk_size, size_t overlap) {
    std::vector<std::string> words;
    std::string word;
    std::stringstream ss(text);
    
    while (ss >> word) {
        words.push_back(word);
    }

    std::vector<IngestionChunk> chunks;
    if (words.empty()) return chunks;

    size_t step = (chunk_size > overlap) ? (chunk_size - overlap) : 1;
    size_t chunk_idx = 0;

    for (size_t i = 0; i < words.size(); i += step) {
        std::stringstream chunk_stream;
        size_t end_idx = std::min(i + chunk_size, words.size());
        
        for (size_t j = i; j < end_idx; ++j) {
            chunk_stream << words[j] << (j + 1 == end_idx ? "" : " ");
        }

        std::string chunk_text = chunk_stream.str();
        chunks.push_back({ chunk_text, i, chunk_idx++ });

        if (end_idx == words.size()) break;
    }

    return chunks;
}

} // namespace isha
