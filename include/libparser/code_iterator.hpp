#pragma once

#include "format.hpp"
#include <fstream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string_view>

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

class CodeIterator {
public:
    CodeIterator(std::string_view data) : m_data(data), m_index(0), m_no_return_point(0) {}
    CodePlace place() const noexcept { return CodePlace(m_index); }
    bool can_move_to(CodePlace place) noexcept { return place.get_index() >= m_no_return_point; }
    void move_to(CodePlace place) noexcept {
        if (place.get_index() < m_no_return_point) {
            fmt::print("Trying to return before no_return_point? No way!\n");
            std::terminate();
        }
        m_index = place.get_index();
    }
    void set_no_return_point() { m_no_return_point = m_index; }
    std::optional<char> get() noexcept {
        if (m_index >= m_data.size())
            return {};
        auto val = m_data[m_index];
        m_index++;
        return val;
    }

    std::optional<std::string_view> get(size_t size) noexcept {
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

    void drop() {
        if (m_index < m_data.size())
            m_index++;
    }

    void error_expected(std::string expected) {
        m_error.expected = expected;
        if (m_error.type == NONE) {
            m_error.index = m_index;
        }
    }

    void set_undroppable_error() {
        m_error.type = UNDROPPABLE;
    }

    bool has_undroppable_error() const {
        return m_error.type == UNDROPPABLE;
    }

    std::string format_error() const {
        auto str = fmt::format("Expected {}", m_error.expected);
        std::string result;
        for (auto elem : str) {
            if (elem == '{')
                result += "{{";
            else if (elem == '}')
                result += "}}";
            else
                result.push_back(elem);
        }
        return result;
    }
private:
    enum ErrorType {
        NONE, DROPPABLE, UNDROPPABLE,
    };
    struct {
        size_t index;
        std::string expected;
        ErrorType type = NONE;
    } m_error;
    std::string_view m_data;
    size_t m_index;
    size_t m_no_return_point;
};
