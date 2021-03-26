#pragma once

#include "code_iterator.hpp"
#include <vector>

class CodeStream {
public:
    CodeStream(std::string filename) noexcept
        : m_data(), m_data_structure(), m_filename(filename) {}

    bool is_open() const noexcept { return m_data.size() != 0; }
    std::string_view get_filename() const noexcept { return m_filename; }
    size_t line_length(size_t num) { return m_data_structure[num]; }
    bool open() noexcept {
        std::ifstream file(m_filename);
        if (!file.is_open())
            return false;

        char symbol = file.get();
        m_data_structure.push_back(0);
        m_data.reserve(2 * 1024);
        while (symbol != -1) {
            m_data_structure.back()++;
            if (symbol == '\n')
                m_data_structure.push_back(0);
            m_data.push_back(symbol);
            symbol = file.get();
        }
        return true;
    }
    std::string_view get_line(CodePlace place) const noexcept {
        auto point = get_code_point(place);
        size_t length = m_data_structure[point.line];
        return std::string_view(m_data).substr(place.get_index()-point.column, length);
    }

    CodePoint get_code_point(CodePlace place) const noexcept {
        size_t index = place.get_index();
        size_t line = 0;
        for (size_t i = 0; m_data_structure[i] < index; i++) {
            index -= m_data_structure[i];
            line++;
        }
        return CodePoint{line, index};
    }

    CodeIterator get_iterator() const { return CodeIterator(m_data); }
private:
    std::string m_data;
    std::vector<size_t> m_data_structure;
    std::string m_filename;
};
