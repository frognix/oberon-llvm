#pragma once

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fstream>
#include <optional>

class CodeStream;

struct CodePlace {
    CodePlace() : index(0) {} //Нужно только потому что в некоторых узлах он пуст
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

    bool open() noexcept;
    std::optional<char> get() noexcept;
    std::optional<std::string> get(size_t size) noexcept;
    std::optional<char> peek() const noexcept;
    std::string_view get_line(CodePlace num) const noexcept;
    CodePoint get_code_point(CodePlace place) const noexcept;

  private:
    std::string m_data;
    size_t m_index;
    size_t m_no_return_point;
    std::vector<size_t> m_data_structure;
    std::string m_filename;
};
