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

ExprResult Number::eval(const SymbolTable&) const {
    return ErrorBuilder(this, "Undefined").build();
}

TypeResult Char::get_type(const SymbolTable&) const {
    return built_in_char();
}

ExprResult Char::eval(const SymbolTable&) const {
    return ErrorBuilder(this, "Undefined").build();
}

TypeResult String::get_type(const SymbolTable&) const {
    auto ch = built_in_char();
    auto size = make_expression<Number>(std::variant<Real, Integer>(Integer(value.size())));
    return make_type<ArrayType>(std::vector{size}, ch);
}

ExprResult String::eval(const SymbolTable&) const {
    return ErrorBuilder(this, "Undefined").build();
}

TypeResult Nil::get_type(const SymbolTable&) const {
    return built_in_nil();
}

ExprResult Nil::eval(const SymbolTable&) const {
    return ErrorBuilder(this, "Undefined").build();
}

TypeResult Boolean::get_type(const SymbolTable&) const {
    return built_in_bool();
}

ExprResult Boolean::eval(const SymbolTable&) const {
    return ErrorBuilder(this, "Undefined").build();
}

TypeResult Set::get_type(const SymbolTable&) const {
    return built_in_set();
}

ExprResult Set::eval(const SymbolTable&) const {
    return ErrorBuilder(this, "Undefined").build();
}

//! \todo
TypeResult ProcCall::get_type(const SymbolTable& table) const {
    return make_type<BuiltInType>(std::vector{'R', 'E', 'A', 'L'});
}

ExprResult ProcCall::eval(const SymbolTable&) const {
    return ErrorBuilder((Expression*)this, "Undefined").build();
}

TypeResult Tilda::get_type(const SymbolTable& table) const {
    auto expr_type = expression->get_type(table);
    if (expr_type) {
        auto boolean = built_in_bool();
        if (expr_type.get_ok() == boolean) return boolean;
        else return ErrorBuilder(this).exfound(boolean.get(), expr_type.get_ok().get()).build();
    } else return expr_type;
}

ExprResult Tilda::eval(const SymbolTable&) const {
    return ErrorBuilder(this, "Undefined").build();
}

///! \todo
TypeResult Term::get_type(const SymbolTable& table) const {
    return make_type<BuiltInType>(std::vector{'R', 'E', 'A', 'L'});
}

ExprResult Term::eval(const SymbolTable&) const {
    return ErrorBuilder(this, "Undefined").build();
}
