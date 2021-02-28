#pragma once

#include "plib/result.hpp"
#include "plib/code_stream.hpp"

#include "node.hpp"

class SemanticError {
public:
    CodePlace get_place() const { return place; }
    std::string get_string() const { return error; }
private:
    SemanticError() {}
    friend class ErrorBuilder;
    CodePlace place;
    std::string error;
};

class ErrorBuilder {
public:
    ErrorBuilder(CodePlace place) : m_error() {
        m_error.place = place;
    }
    SemanticError build() {
        m_error.error =fmt::format("{}", fmt::join(errors, ";\n"));
        return m_error;
    }
    ErrorBuilder& exfound(const nodes::Node* expected, const nodes::Node* found) {
        errors.push_back(fmt::format("expected {}, found {}", expected->to_string(), found->to_string()));
        return *this;
    }
    ErrorBuilder& text(std::string str) {
        errors.push_back(str);
        return *this;
    }
    template <class... Args>
    ErrorBuilder& format(const char* str, Args... args) {
        errors.push_back(fmt::format(str, args...));
        return *this;
    }
private:
    SemanticError m_error;
    std::vector<std::string> errors;
};

using Error = std::optional<SemanticError>;

template <class T>
using SemResult = Result<T, SemanticError>;

using ExprResult = SemResult<nodes::ExpressionPtr>;
using TypeResult = SemResult<nodes::TypePtr>;
// using ExprResult = SemResult<ExpressionPtr>;
