#pragma once

#include <algorithm>
#include <memory>
#include <set>

#include "code_stream.hpp"
#include "format.hpp"

struct ParseError {
    ParseError(const CodeStream& stream) : place(stream.place()) {}
    template <class T>
    ParseError(std::string e, T f, const CodeStream& stream)
        : expected({e}), found(fmt::format("{}", f)), place(stream.place()) {}
    std::string to_string() const {
        auto str = fmt::format("{} Expected ( {} ), found {}",
                               format_red(fmt::format("{}:{}:{}:", place.file, place.line, place.column)),
                               fmt::join(expected, " or "), found);
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
    ParseError operator||(const ParseError& other) {
        ParseError result = *this;
        for (auto elem : other.expected) {
            result.expected.insert(elem);
        }
        result.found = other.found;
        result.place = other.place;
        return result;
    }
    void set_undroppable() { undroppable = true; }
    bool is_undroppable() const { return undroppable; }

    std::set<std::string> expected;
    std::string found;
    CodePlace place;

  private:
    bool undroppable = false;
};
