#include "expression_nodes.hpp"

#include "symbol_table.hpp"

using namespace nodes;

TypeResult Number::get_type(const SymbolTable&) const {
    if (std::holds_alternative<Real>(value)) {
        return built_in_real();
    } else {
        return built_in_int();
    }
}

TypeResult Char::get_type(const SymbolTable&) const {
    return built_in_char();
}

TypeResult String::get_type(const SymbolTable&) const {
    auto ch = built_in_char();
    auto size = make_optr<Expression, Number>(std::variant<Real, Integer>(Integer(value.size())));
    return make_optr<Type, ArrayType>(std::vector{size}, ch);
}

TypeResult Nil::get_type(const SymbolTable&) const {
    return built_in_nil();
}

TypeResult Boolean::get_type(const SymbolTable&) const {
    return built_in_bool();
}

TypeResult Set::get_type(const SymbolTable&) const {
    return built_in_set();
}

//! \todo
TypeResult ProcCall::get_type(const SymbolTable& table) const {
    return make_optr<Type, BuiltInType>(std::vector{'R', 'E', 'A', 'L'});
}

TypeResult Tilda::get_type(const SymbolTable& table) const {
    auto expr_type = expression->get_type(table);
    if (expr_type) {
        auto boolean = built_in_bool();
        if (expr_type.get_ok() == boolean) return boolean;
        else return TypeError(this, boolean, expr_type.get_ok());
    } else return expr_type;
}

///! \todo
TypeResult Term::get_type(const SymbolTable& table) const {
    return make_optr<Type, BuiltInType>(std::vector{'R', 'E', 'A', 'L'});
}
