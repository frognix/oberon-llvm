#include "expression_nodes.hpp"

#include "symbol_table.hpp"

using namespace nodes;

SemResult Number::get_type(const SymbolTable&) const {
    if (std::holds_alternative<Real>(value)) {
        return built_in_real();
    } else {
        return built_in_int();
    }
}

SemResult Number::eval(const SymbolTable&) const {
    return ErrorBuilder(this, "Undefined").build();
}

SemResult Char::get_type(const SymbolTable&) const {
    return built_in_char();
}

SemResult Char::eval(const SymbolTable&) const {
    return ErrorBuilder(this, "Undefined").build();
}

SemResult String::get_type(const SymbolTable&) const {
    auto ch = built_in_char();
    auto size = make_expression<Number>(std::variant<Real, Integer>(Integer(value.size())));
    return make_type<ArrayType>(std::vector{size}, ch);
}

SemResult String::eval(const SymbolTable&) const {
    return ErrorBuilder(this, "Undefined").build();
}

SemResult Nil::get_type(const SymbolTable&) const {
    return built_in_nil();
}

SemResult Nil::eval(const SymbolTable&) const {
    return ErrorBuilder(this, "Undefined").build();
}

SemResult Boolean::get_type(const SymbolTable&) const {
    return built_in_bool();
}

SemResult Boolean::eval(const SymbolTable&) const {
    return ErrorBuilder(this, "Undefined").build();
}

SemResult Set::get_type(const SymbolTable&) const {
    return built_in_set();
}

SemResult Set::eval(const SymbolTable&) const {
    return ErrorBuilder(this, "Undefined").build();
}

//! \todo
SemResult ProcCall::get_type(const SymbolTable& table) const {
    return make_type<BuiltInType>(std::vector{'R', 'E', 'A', 'L'});
}

SemResult ProcCall::eval(const SymbolTable&) const {
    return ErrorBuilder((Expression*)this, "Undefined").build();
}

SemResult Tilda::get_type(const SymbolTable& table) const {
    auto expr_type = expression->get_type(table);
    if (expr_type) {
        auto boolean = built_in_bool();
        if (expr_type.get_ok() == boolean) return boolean;
        else return ErrorBuilder(this).exfound(boolean.get(), expr_type.get_ok().get()).build();
    } else return expr_type;
}

SemResult Tilda::eval(const SymbolTable&) const {
    return ErrorBuilder(this, "Undefined").build();
}

///! \todo
SemResult Term::get_type(const SymbolTable& table) const {
    return make_type<BuiltInType>(std::vector{'R', 'E', 'A', 'L'});
}

SemResult Term::eval(const SymbolTable&) const {
    return ErrorBuilder(this, "Undefined").build();
}
