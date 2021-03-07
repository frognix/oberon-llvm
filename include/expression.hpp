#pragma once

#include "node.hpp"
#include "type.hpp"
#include "semantic_error.hpp"
#include "repairer.hpp"

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

struct Designator {
    Designator() {}
    Designator(QualIdent i, std::vector<Selector> s) : ident(i), selector(s) {}
    std::string to_string() const {return fmt::format("{}{}", ident, fmt::join(selector, ""));}
    bool is_simple() const { return selector.empty() && !ident.qual; }
    SemResult<SymbolToken> get_symbol(const SymbolTable&, CodePlace) const;
    QualIdent ident;
    std::vector<Selector> selector;
};

std::optional<SemanticError> designator_repair(Designator& value, const SymbolTable& table);

using DesignatorRepairer = Repairer<Designator, SymbolTable, SemanticError, designator_repair>;

}
