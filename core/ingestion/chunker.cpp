#include "chunker.hpp"
#include <sstream>
#include <algorithm>
#include <regex>

namespace isha {

// Helper to determine if a line starts a Markdown table, list, or numbered step
bool isSemanticBoundaryDelimiter(const std::string& line) {
    // Check if line contains a markdown table row delimiter e.g. |
    if (line.find('|') != std::string::npos) {
        return true;
    }
    // Check if line is a bullet item starting with -, *, or +
    std::string trimmed = line;
    trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    if (trimmed.rfind("- ", 0) == 0 || trimmed.rfind("* ", 0) == 0 || trimmed.rfind("+ ", 0) == 0) {
        return true;
    }
    // Check if line starts with a numbered step (e.g. "1. ", "10. ")
    static const std::regex numberedPattern("^\\s*\\d+\\.\\s+.*");
    if (std::regex_match(trimmed, numberedPattern)) {
        return true;
    }
    return false;
}

std::vector<IngestionChunk> BasicWordChunker::chunkText(const std::string& text, size_t chunk_size_words, size_t overlap_words) {
    std::vector<IngestionChunk> chunks;
    if (text.empty()) return chunks;

    std::vector<std::string> lines;
    std::string line;
    std::stringstream ss(text);
    while (std::getline(ss, line)) {
        lines.push_back(line);
    }

    std::vector<std::string> current_chunk_words;
    std::vector<std::string> current_line_words;
    size_t chunk_idx = 0;
    size_t char_offset = 0;

    for (const auto& raw_line : lines) {
        // Simple word splitting for current line
        current_line_words.clear();
        std::stringstream line_ss(raw_line);
        std::string w;
        while (line_ss >> w) {
            current_line_words.push_back(w);
        }

        if (current_line_words.empty()) continue;

        // If adding the new line's words breaches the target chunk size AND
        // we've met the boundary condition, flush the current chunk.
        if (current_chunk_words.size() + current_line_words.size() > chunk_size_words && !current_chunk_words.empty()) {
            // Semantic protection: If this line is a table or list, try not to split it
            // if we are already close to the limit. We emit the current chunk first.
            std::stringstream chunk_stream;
            for (size_t i = 0; i < current_chunk_words.size(); ++i) {
                chunk_stream << current_chunk_words[i] << (i + 1 == current_chunk_words.size() ? "" : " ");
            }
            chunks.push_back({ chunk_stream.str(), char_offset, chunk_idx++ });

            // Apply overlap: retain last N words
            size_t overlap_start = (current_chunk_words.size() > overlap_words) ? 
                                    (current_chunk_words.size() - overlap_words) : 0;
            std::vector<std::string> next_chunk_words(
                current_chunk_words.begin() + overlap_start,
                current_chunk_words.end()
            );
            current_chunk_words = std::move(next_chunk_words);
            char_offset += chunk_stream.str().size(); // Rough approximation of character offset
        }

        // Add line to chunk
        current_chunk_words.insert(
            current_chunk_words.end(),
            current_line_words.begin(),
            current_line_words.end()
        );
    }

    // Flush any remaining words
    if (!current_chunk_words.empty()) {
        std::stringstream chunk_stream;
        for (size_t i = 0; i < current_chunk_words.size(); ++i) {
            chunk_stream << current_chunk_words[i] << (i + 1 == current_chunk_words.size() ? "" : " ");
        }
        chunks.push_back({ chunk_stream.str(), char_offset, chunk_idx });
    }

    return chunks;
}

} // namespace isha
