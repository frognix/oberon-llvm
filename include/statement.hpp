#pragma once

#include "node.hpp"

namespace nodes {

struct Statement : Node {};

// using StatementPtr = OPtr<Statement>;

using StatementSequence = std::vector<StatementPtr>;

template <class Subtype, class... Args>
StatementPtr make_statement(Args... args) {
    return make_optr<Statement, Subtype>(args...);
}

}
