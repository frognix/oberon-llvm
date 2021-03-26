#pragma once

#include <exception>
#include "libparser/format.hpp"
#include <iostream>
#include <string_view>

namespace internal {

class InternalCompilerError : public std::exception {
public:
    inline InternalCompilerError(std::string_view message) noexcept : std::exception{} {
        m_message = fmt::format("Internal compiler error: {}\n", message);
    }
    const char* what() const noexcept override { return m_message.c_str(); }

private:
    std::string m_message;
};

[[noreturn]] inline void compiler_error(std::string_view message) {
    throw InternalCompilerError(message);
}

} // namespace internal
