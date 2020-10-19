#include "code_stream.hpp"

bool CodeStream::open() noexcept {
    std::ifstream file(m_filename);
    if (!file.is_open()) return false;

    char symbol = file.get();
    m_data_structure.push_back(0);
    m_data.reserve(2*1024);
    while (symbol != -1) {
        m_data_structure.back()++;
        if (symbol == '\n') m_data_structure.push_back(0);
        m_data.push_back(symbol);
        symbol = file.get();
    }
    return true;
}

std::optional<char> CodeStream::get() noexcept {
    if (m_index >= m_data.size()) return {};
    auto val = m_data[m_index];
    m_index++;
    return val;
}

std::optional<std::string> CodeStream::get(size_t size) noexcept {
    if (m_index + size > m_data.size()) return {};

    auto val = m_data.substr(m_index, size);
    m_index += size;

    return val;
}

std::optional<char> CodeStream::peek() const noexcept {
    if (m_index >= m_data.size()) return {};
    return m_data[m_index];
}

std::string_view CodeStream::get_line(size_t num) const noexcept {
    CodePlace place;
    place.line = num;
    size_t index = place_to_index(place);
    size_t length = m_data_structure[num];
    return std::string_view(m_data).substr(index, length);
}

CodePlace CodeStream::index_to_place(size_t index) const noexcept {
    CodePlace place;
    place.file = m_filename;
    for (auto length : m_data_structure) {
        if (index > length) {
            place.line++;
            index -= length;
        } else {
            place.column = index;
            break;
        }
    }
    return place;
}

size_t CodeStream::place_to_index(CodePlace place) const noexcept {
    size_t index = 0;
    for (size_t i = 0; i < place.line; i++) {
        index += m_data_structure[i];
    }
    index += place.column;
    return index;
}
