#include "plib/code_stream.hpp"

bool CodeStream::open() noexcept {
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

std::optional<char> CodeStream::get() noexcept {
    if (m_index >= m_data.size())
        return {};
    auto val = m_data[m_index];
    m_index++;
    return val;
}

std::optional<std::string> CodeStream::get(size_t size) noexcept {
    if (m_index + size > m_data.size())
        return {};

    auto val = m_data.substr(m_index, size);
    m_index += size;

    return val;
}

std::optional<char> CodeStream::peek() const noexcept {
    if (m_index >= m_data.size())
        return {};
    return m_data[m_index];
}

std::string_view CodeStream::get_line(CodePlace place) const noexcept {
    auto point = get_code_point(place);
    size_t length = m_data_structure[point.line];
    return std::string_view(m_data).substr(place.get_index()-point.column, length);
}

CodePoint CodeStream::get_code_point(CodePlace place) const noexcept {
    auto index = place.get_index();
    size_t line = 0;
    for (int i = 0; m_data_structure[i] < index; i++) {
        index -= m_data_structure[i];
        line++;
    }
    return CodePoint{line, index};
}
