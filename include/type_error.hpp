#pragma once

#include "plib/result.hpp"

#include "type.hpp"

class TypeError {
public:
    TypeError(const nodes::Node* node, nodes::TypePtr expected, nodes::TypePtr found, std::string info = "")
        : place(node->place) {
        error = fmt::format("{} expected {}, found {}. {}", place, expected->to_string(), found->to_string(), info);
    }
private:
    CodePlace place;
    std::string error;
};

using TypeResult = Result<nodes::TypePtr, TypeError>;
