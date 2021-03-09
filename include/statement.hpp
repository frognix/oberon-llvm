#pragma once

#include "node.hpp"

class SymbolTable;

namespace nodes {

struct Statement : Node {
    virtual bool check(Context&) const = 0;
    virtual ~Statement(){};
};

// using StatementPtr = OPtr<Statement>;

using StatementSequence = std::vector<StatementPtr>;

template <class Subtype, class... Args>
StatementPtr make_statement(Args... args) {
    return make_optr<Statement, Subtype>(args...);
}

} // namespace nodes
