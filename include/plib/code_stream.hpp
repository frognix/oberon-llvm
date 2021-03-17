#pragma once

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fstream>
#include <optional>

class CodeStream;

struct CodePlace {
    CodePlace() : index(0) {}
    CodePlace(size_t p) : index(p) {}
    inline size_t get_index() const { return index; }
private:
    size_t index;
};

struct CodePoint {
    size_t line;
    size_t column;
};

template <>
struct fmt::formatter<CodePoint> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(CodePoint const& point, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "{}:{}", point.line, point.column);
    }
};

class CodeStream {
    friend class BreakPoint;

  public:
    CodeStream(std::string filename) noexcept
        : m_data(), m_index(0), m_no_return_point(0), m_data_structure(), m_filename(filename) {}

    bool is_open() const noexcept { return m_data.size() != 0; }
    std::string_view get_filename() const noexcept { return m_filename; }
    CodePlace place() const noexcept { return CodePlace(m_index); }
    size_t index() const noexcept { return m_index; }
    bool can_move_to(size_t index) noexcept { return index >= m_no_return_point; }
    void move_to(size_t index) noexcept {
        if (index < m_no_return_point) {
            fmt::print("Trying to return before no_return_point? No way!\n");
            std::terminate();
        }
        m_index = index;
    }
    void set_no_return_point() { m_no_return_point = m_index; }
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

    std::optional<char> get() noexcept {
        if (m_index >= m_data.size())
            return {};
        auto val = m_data[m_index];
        m_index++;
        return val;
    }

    std::optional<std::string> get(size_t size) noexcept {
        if (m_index + size > m_data.size())
            return {};

        auto val = m_data.substr(m_index, size);
        m_index += size;

        return val;
    }

    std::optional<char> peek() const noexcept {
        if (m_index >= m_data.size())
            return {};
        return m_data[m_index];
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
  private:
    std::string m_data;
    size_t m_index;
    size_t m_no_return_point;
    std::vector<size_t> m_data_structure;
    std::string m_filename;
};
