#pragma once

#include "plib/result.hpp"

#include "type.hpp"

class SemanticError {
public:
    CodePlace get_place() const { return place; }
private:
    SemanticError() {}
    friend class ErrorBuilder;
    CodePlace place;
    std::string error;
};

class ErrorBuilder {
public:
    ErrorBuilder(const nodes::Node* node, std::string info = "") : m_error(), m_info(info) {
        m_error.place = node->place;
    }
    SemanticError build() {
        m_error.error = fmt::format("{} {} ({})", m_error.place, m_error.error, m_info);
        return m_error;
    }
    ErrorBuilder& exfound(const nodes::Node* expected, const nodes::Node* found) {
        m_error.error += fmt::format("expected {}, found {}", expected->to_string(), found->to_string());
        return *this;
    }
private:
    SemanticError m_error;
    std::string m_info;
};

using SemResult = Result<nodes::TypePtr, SemanticError>;
