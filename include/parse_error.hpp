#pragma once

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <memory>
#include <set>
#include <algorithm>

#include "code_stream.hpp"

struct ParseError {
    ParseError(const CodeStream& stream)
        : place(stream.place()), file(stream.get_filename()) {}
    template <class T>
    ParseError(std::string e, T f, const CodeStream& stream)
        : expected({e}), found(fmt::format("{}", f)),
          place(stream.place()), file(stream.get_filename()) {}
    std::string to_string() const {
        auto str = fmt::format("{}:{}:{} Expected {}, found {}",
                               file, place.line, place.column,
                               fmt::join(expected, " or "), found);
        std::string result;
        for (auto elem : str) {
            if (elem == '{') result += "{{";
            else if (elem == '}') result += "}}";
            else result.push_back(elem);
        }
        return result;
    }
    ParseError operator || (const ParseError& other) {
        ParseError result = *this;
        for (auto elem : other.expected) {
            result.expected.insert(elem);
        }
        result.found = other.found;
        result.place = other.place;
        return result;
    }
    std::set<std::string> expected;
    std::string found;
    CodePlace place;
    std::string file;
};
