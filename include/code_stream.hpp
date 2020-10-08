#pragma once

#include <optional>
#include <fstream>
#include <fmt/format.h>

class Range {
public:
    Range(size_t begin, size_t end) : m_start(begin), m_size(end-begin) {
        if (end < begin) throw std::runtime_error("Range start must me less or equal to end");
    }
    size_t begin() const { return m_start; }
    size_t end() const { return m_start + m_size; }
    size_t size() const { return m_size; }
    Range operator + (const Range& other) {
        if (other.begin() != end()) throw std::runtime_error("For operator + end of first range must be start of second");
        return Range(m_start, other.end());
    }
private:
    size_t m_start;
    size_t m_size;
};

struct CodePlace {
    CodePlace() : line(0), column(0) {}
    CodePlace(size_t l, size_t c) : line(l), column(c) {}
    void new_line() { line++; column = 0; }
    size_t line;
    size_t column;
};

class CodeStream {
    friend class BreakPoint;
public:
    CodeStream(std::string filename) : m_file(filename), m_filename(filename) {}
    bool is_open() const { return m_file.is_open(); }
    std::string_view get_filename() const { return m_filename; }
    CodePlace place() const { return m_place; }
    std::optional<char> get() {
        auto val = m_file.get();
        if (val == '\0' || val == -1) return {};
        if (val == '\n') {
            m_place.new_line();
        } else {
            m_place.column++;
        }

        return val;
    }
    std::optional<std::string> peek(Range range) {
        auto last_place = m_file.tellg();
        m_file.seekg(range.begin());
        std::string result;
        result.resize(range.size());
        m_file.read(result.data(), range.size());
        m_file.seekg(last_place);
        if (m_file.gcount() != range.size()) return {};
        else return result;
    }
    std::optional<std::string> get(size_t size) {
        std::string result;
        result.resize(size);
        m_file.read(result.data(), size);
        if (m_file.gcount() != size) return {};
        else return result;
    }
    std::optional<char> peek() {
        auto val = m_file.peek();
        if (val == '\0' || val == -1) return {};
        else return val;
    }
private:
    std::ifstream m_file;
    std::string m_filename;
    CodePlace m_place;
};
