#pragma once

#include "node.hpp"
#include "type.hpp"
#include "semantic_error.hpp"

class SymbolTable;
struct SymbolToken;

namespace nodes {

struct Expression : Node {
    virtual TypeResult get_type(const SymbolTable&) const = 0;
    virtual ExprResult eval(const SymbolTable&) const = 0;
    virtual ~Expression() = default;
};

template <class Subtype, class... Args>
ExpressionPtr make_expression(Args... args) {
    return make_optr<Expression, Subtype>(args...);
}

using ExpList = std::vector<ExpressionPtr>;

using Selector = std::variant<Ident, ExpList, char, QualIdent>;

struct ValidDesignator {
    QualIdent ident;
    std::vector<Selector> selector;
    SemResult<SymbolToken> get_symbol(const SymbolTable& table, CodePlace place) const;
};

struct Designator {
    Designator() {}
    Designator(QualIdent i, std::vector<Selector> s) : ident(i), selector(s) {}
    SemResult<ValidDesignator> get(const SymbolTable&) const;
    std::string to_string() const {return fmt::format("{}{}", ident, fmt::join(selector, ""));}
    bool is_simple() const { return selector.empty() && !ident.qual; }
private:
    QualIdent ident;
    std::vector<Selector> selector;
};

}
