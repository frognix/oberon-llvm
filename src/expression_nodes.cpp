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
    return make_expression<Number>(*this);
}

TypeResult Char::get_type(const SymbolTable&) const {
    return built_in_char();
}

ExprResult Char::eval(const SymbolTable&) const {
    return make_expression<Char>(*this);
}

TypeResult String::get_type(const SymbolTable&) const {
    auto ch = built_in_char();
    auto size = make_expression<Number>(std::variant<Real, Integer>(Integer(value.size())));
    return make_type<ArrayType>(std::vector{size}, ch);
}

ExprResult String::eval(const SymbolTable&) const {
    return make_expression<String>(*this);
}

TypeResult Nil::get_type(const SymbolTable&) const {
    return built_in_nil();
}

ExprResult Nil::eval(const SymbolTable&) const {
    return make_expression<Nil>(*this);
}

TypeResult Boolean::get_type(const SymbolTable&) const {
    return built_in_bool();
}

ExprResult Boolean::eval(const SymbolTable&) const {
    return make_expression<Boolean>(*this);
}

TypeResult Set::get_type(const SymbolTable&) const {
    return built_in_set();
}

ExprResult Set::eval(const SymbolTable&) const {
    return make_expression<Set>(*this);
}

inline SemResult<RecordType*> get_record_from_pointer(Type* some_type, const SymbolTable& table, CodePlace place) {
    Type* start_type = some_type;
    if (auto typeName = some_type->is<TypeName>(); typeName) {
        auto type = typeName->dereference(table);
        if (!type) return type.get_err();
        some_type = type.get_ok().get();
    }
    if (auto pointer = some_type->is<PointerType>(); pointer) {
        some_type = pointer->type.get();
        if (auto typeName = some_type->is<TypeName>(); typeName) {
            auto type = typeName->dereference(table);
            if (!type) return type.get_err();
            some_type = type.get_ok().get();
        }
    }
    auto type = dynamic_cast<RecordType*>(some_type);
    if (!type) return ErrorBuilder(place).format("Expected record, found {}", start_type->to_string()).build();
    return type;
}

SemResult<SymbolToken> ValidDesignator::get_symbol(const SymbolTable& table, CodePlace place) const {
    auto res = table.get_symbol(ident);
    if (!res) {
        return res.get_err();
    } else {
        auto error = ErrorBuilder(place);
        auto symbol = res.get_ok();
        for (auto& sel : selector) {
            auto symbolType = symbol.type;
            if (symbol.group == SymbolGroup::TYPE) {
                return error.format("Unexpected type name {}", symbol.name).build();
            }
            if (auto ident = std::get_if<Ident>(&sel); ident) {
                auto typeRes = get_record_from_pointer(symbol.type.get(), table, place);
                if (!typeRes) return typeRes.get_err();
                auto type = typeRes.get_ok();
                if (auto fType = type->has_field(*ident, table); fType) {
                    symbol.type = fType.get_ok();
                } else {
                    return error.format("Field {} not found in record {}", *ident, type->to_string()).build();
                }
            } else if (auto ident = std::get_if<ExpList>(&sel); ident) {
                auto ltype = dynamic_cast<ArrayType*>(symbol.type.get());
                if (auto ntype = ltype->drop_dimensions(ident->size()); ntype) {
                    symbol.type = ntype.get_ok();
                } else {
                    return ntype.get_err();
                }
            } else if (auto ident = std::get_if<char>(&sel); ident) {
                auto type = dynamic_cast<PointerType*>(symbol.type.get());
                if (type) {
                    symbol.type = type->type;
                } else {
                    return error.format("Expected pointer type, found {}", symbol.type->to_string()).build();
                }
            } else if (auto ident = std::get_if<QualIdent>(&sel); ident) {
                auto ltype = get_record_from_pointer(symbol.type.get(), table, place);
                if (!ltype) return ltype.get_err();
                if (auto rsymbol = table.get_symbol(*ident); rsymbol) {
                    if (rsymbol.get_ok().group == SymbolGroup::TYPE) {
                        if (ltype && table.type_extends_base(ltype.get_ok(), *ident)) {
                            symbol.type = rsymbol.get_ok().type;
                        }
                    } else {
                        return error.format("Expected type, found {}", rsymbol.get_ok().name).build();
                    }
                } else {
                    return rsymbol.get_err();
                }
            } else {
                return error.text("Internal compiler error in ProcCall::get_type").build();
            }
        }
        return symbol;
    }
}

SemResult<ValidDesignator> Designator::get(const SymbolTable& table) const {
    if (!ident.qual) {
        return ValidDesignator{ident, selector};
    } else {
        auto import = table.get_symbol(nodes::QualIdent{{}, *ident.qual}, true);
        if (!import) return import.get_err();
        if (import.get_ok().group == SymbolGroup::VAR || import.get_ok().group == SymbolGroup::CONST) {
            std::vector<Selector> new_selector{ident.ident};
            new_selector.insert(new_selector.end(), selector.begin(), selector.end());
            return ValidDesignator{QualIdent{{}, *ident.qual}, new_selector};
        } else if (import.get_ok().group == SymbolGroup::MODULE) {
            return ValidDesignator{ident, selector};
        } else {
            return ErrorBuilder(ident.qual->place).text("Expected variable or module name").build();
        }
    }
}

SemResult<SymbolToken> ProcCall::get_info(const SymbolTable& table) const {
    auto validIdentRes = ident.get(table);
    if (!validIdentRes) return validIdentRes.get_err();
    auto validIdent = validIdentRes.get_ok();
    //Проверка на правильный парсинг приведения типа
    //В некоторых случаях это может быть не приведение типа, а вызов функции
    auto new_params = params;
    if (validIdent.selector.size() > 0) {
        auto back_selecter = validIdent.selector.back();
        if (!params && std::holds_alternative<QualIdent>(back_selecter)) {
            auto ident = std::get<QualIdent>(back_selecter);
            auto symRes = table.get_symbol(ident);
            if (!symRes) return symRes.get_err();
            if (symRes.get_ok().group != SymbolGroup::TYPE)
                validIdent.selector.pop_back();
            new_params = {make_expression<ProcCall>(ident)};
        }
    }
    auto res = validIdent.get_symbol(table, place);
    auto error = ErrorBuilder(place);
    if (!res) return res.get_err();
    auto symbol = res.get_ok();
    if (!new_params) {
        return symbol;
    } else {
        auto funcType = dynamic_cast<ProcedureType*>(symbol.type.get());
        auto expression = new_params->begin();
        if (funcType) {
            for (auto& section : funcType->params.sections) {
                for (auto& param [[maybe_unused]] : section.idents) {
                    if (expression == new_params->end())
                        return error.format("Number of arguments does not match the number of formal parameters")
                            .build();
                    if (section.var) {
                        auto procCall = dynamic_cast<ProcCall*>((*expression).get());
                        auto errVar = error.format("Expected variable type {}", (*expression)->to_string()).build();
                        if (!procCall) return errVar;
                        auto info = procCall->get_info(table);
                        if (!info) return info.get_err();
                        if (info.get_ok().group != SymbolGroup::VAR) return errVar;
                    }
                    auto exprType = (*expression)->get_type(table);
                    if (!exprType)
                        return exprType.get_err();
                    if (*section.type != *exprType.get_ok()) {
                        auto etype = exprType.get_ok()->is<PointerType>();
                        auto stype = section.type->is<PointerType>();
                        TypeName const* ntype = nullptr;
                        if (stype) ntype = stype->type->is<TypeName>();
                        if (stype && etype && ntype) {
                            if (!table.type_extends_base(etype->type.get(), ntype->ident)) {
                                return error.format("Type {} does not extends type {}", etype->type, stype->type).build();
                            }
                        } else {
                            return error.format("Expected {}, found {}", section.type, exprType.get_ok()->to_string())
                                .build();
                        }
                    }
                    expression++;
                }
            }
            if (!funcType->params.rettype) {
                symbol.type = nullptr;
                // return error.format("This procedure call has no type").build();
                return symbol;
            } else {
                auto rettype = table.get_symbol(*funcType->params.rettype);
                if (!rettype)
                    return rettype.get_err();
                else {
                    symbol.type = rettype.get_ok().type;
                    symbol.group = SymbolGroup::CONST;
                    return symbol;
                }
            }
        } else {
            return error.format("Expected procedure type, found {}", symbol.type.get()->to_string()).build();
        }
    }
}

TypeResult ProcCall::get_type(const SymbolTable& table) const {
    auto symbol = get_info(table);
    if (!symbol) return symbol.get_err();
    if (symbol.get_ok().type == nullptr)
        return ErrorBuilder(((Expression*)this)->place).text("This procedure call has no type").build();
    else return symbol.get_ok().type;
}

ExprResult ProcCall::eval(const SymbolTable& table) const {
    auto validIdent = ident.get(table);
    if (!validIdent) return validIdent.get_err();
    if (!params && validIdent.get_ok().selector.size() == 0) {
        return table.get_value(validIdent.get_ok().ident);
    }
    return ErrorBuilder(place).format("Selection sequence cannot be constant: {}", this->to_string()).build();
}

TypeResult Tilda::get_type(const SymbolTable& table) const {
    auto expr_type = expression->get_type(table);
    if (expr_type) {
        auto boolean = built_in_bool();
        if (expr_type.get_ok() == boolean) return boolean;
        else return ErrorBuilder(this->place).exfound(boolean.get(), expr_type.get_ok().get()).build();
    } else return expr_type;
}

ExprResult Tilda::eval(const SymbolTable& table) const {
    auto res = expression->eval(table);
    if (!res) return res.get_err();
    auto boolean = dynamic_cast<Boolean*>(res.get_ok().get());
    if (!boolean) return ErrorBuilder(place).text("Expected boolean").build();
    return make_expression<Boolean>(!boolean->value);
}

///! \todo
TypeResult Term::get_type(const SymbolTable& table) const {
    if (!sign && !oper && !second) {
        return first->get_type(table);
    } else {
         return make_type<BuiltInType>(str_to_ident("INTEGER"));
    }
}

ExprResult Term::eval(const SymbolTable& table) const {
    if (!sign && !oper && !second) {
        return first->eval(table);
    } else {
        return make_expression<Number>(Integer(10));
    }
}
