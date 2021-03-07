#include "expression_nodes.hpp"

#include "symbol_table.hpp"
#include "node_formatters.hpp"

using namespace nodes;

std::string Number::to_string() const {
    return std::visit([](auto&& arg) { return fmt::format("{}", arg); }, value);
}

TypeResult Number::get_type(const SymbolTable&) const {
    if (std::holds_alternative<Real>(value)) {
        return make_base_type(BaseType::REAL);
    } else {
        return make_base_type(BaseType::INTEGER);
    }
}

ExprResult Number::eval(const SymbolTable&) const {
    return make_expression<Number>(*this);
}

std::string Char::to_string() const {
    if (value < ' ' || value > '~')
        return fmt::format("{}X", (int)value);
    else
        return fmt::format("'{}'", value);
}

TypeResult Char::get_type(const SymbolTable&) const {
    return make_base_type(BaseType::CHAR);
}

ExprResult Char::eval(const SymbolTable&) const {
    return make_expression<Char>(*this);
}

std::string String::to_string() const {
    return fmt::format("\"{}\"", fmt::join(value, ""));
}

TypeResult String::get_type(const SymbolTable&) const {
    auto ch = make_base_type(BaseType::CHAR);
    auto size = make_expression<Number>(std::variant<Real, Integer>(Integer(value.size())));
    return make_type<ArrayType>(std::vector{size}, ch);
}

ExprResult String::eval(const SymbolTable&) const {
    return make_expression<String>(*this);
}

std::string Nil::to_string() const {
    return "NIL";
}

TypeResult Nil::get_type(const SymbolTable&) const {
    return make_base_type(BaseType::NIL);
}

ExprResult Nil::eval(const SymbolTable&) const {
    return make_expression<Nil>(*this);
}

std::string Boolean::to_string() const {
    return fmt::format("{}", value);
}

TypeResult Boolean::get_type(const SymbolTable&) const {
    return make_base_type(BaseType::BOOL);
}

ExprResult Boolean::eval(const SymbolTable&) const {
    return make_expression<Boolean>(*this);
}

std::string Set::to_string() const {
    return fmt::format("{{{}}}", fmt::join(value, ", "));
}

TypeResult Set::get_type(const SymbolTable&) const {
    return make_base_type(BaseType::SET);
}

ExprResult Set::eval(const SymbolTable&) const {
    return make_expression<Set>(*this);
}

Set::Set(std::optional<std::vector<SetElement>> v) {
    if (v)
        value = *v;
}

inline SemResult<RecordType*> get_record_from_pointer(Type* some_type, const SymbolTable& table, CodePlace place) {
    Type* start_type = some_type;
    if (auto typeName = some_type->is<TypeName>(); typeName) {
        auto type = typeName->dereference(table);
        if (!type)
            return type.get_err();
        some_type = type.get_ok().get();
    }
    if (auto pointer = some_type->is<PointerType>(); pointer) {
        some_type = pointer->type.get();
        if (auto typeName = some_type->is<TypeName>(); typeName) {
            auto type = typeName->dereference(table);
            if (!type)
                return type.get_err();
            some_type = type.get_ok().get();
        }
    }
    auto type = dynamic_cast<RecordType*>(some_type);
    if (!type)
        return ErrorBuilder(place).format("Expected record, found {}", start_type->to_string()).build();
    return type;
}

SemResult<SymbolToken> Designator::get_symbol(const SymbolTable& table, CodePlace place) const {
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
                if (!typeRes)
                    return typeRes.get_err();
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
                if (!ltype)
                    return ltype.get_err();
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

std::optional<SemanticError> nodes::designator_repair(Designator& value, const SymbolTable& table) {
    if (value.ident.qual) {
        auto import = table.get_symbol(nodes::QualIdent{{}, *value.ident.qual}, true);
        if (!import)
            return import.get_err();
        if (import.get_ok().group == SymbolGroup::VAR || import.get_ok().group == SymbolGroup::CONST) {
            std::vector<Selector> new_selector{value.ident.ident};
            new_selector.insert(new_selector.end(), value.selector.begin(), value.selector.end());
            value.selector = new_selector;
            value.ident.ident = *value.ident.qual;
            value.ident.qual = {};
        } else if (import.get_ok().group != SymbolGroup::MODULE) {
            return ErrorBuilder(value.ident.qual->place).text("Expected variable or module name").build();
        }
    }
    return {};
}

std::optional<SemanticError> nodes::proccall_repair(ProcCallData& value, const SymbolTable& table) {
    auto err = value.ident.repair(table);
    if (err) return err;
    auto& ident = value.ident.get_mut();
    if (ident.selector.size() > 0 && !value.params) {
        auto back_selecter = ident.selector.back();
        if (!value.params && std::holds_alternative<QualIdent>(back_selecter)) {
            auto desig = DesignatorRepairer(std::get<QualIdent>(back_selecter), std::vector<Selector>{});
            auto err = desig.repair(table);
            if (!err) {
                ident.selector.pop_back();
                value.params = {make_expression<ProcCall>(desig.get(), std::optional<ExpList>{})};
            }
        }
    }
    return {};
}

std::string ProcCall::to_string() const {
    auto params = data.unsafe_get().params;
    auto ident = data.unsafe_get().ident.unsafe_get();
    if (data.unsafe_get().params)
        return fmt::format("{}({})", ident.to_string(), fmt::join(*params, ", "));
    else
        return fmt::format("{}", ident.to_string());
}

SemResult<SymbolToken> ProcCall::get_info(const SymbolTable& table) const {
    auto err = data.repair(table);
    if (err) return SemanticError{*err};
    auto ident = data.get().ident.get();
    auto params = data.get().params;
    auto res = ident.get_symbol(table, place);
    auto error = ErrorBuilder(place);
    if (!res)
        return res.get_err();
    auto symbol = res.get_ok();
    if (!params) {
        return symbol;
    } else {
        auto funcType = dynamic_cast<ProcedureType*>(symbol.type.get());
        auto expression = params->begin();
        if (funcType) {
            for (auto& section : funcType->params.sections) {
                for (auto& param [[maybe_unused]] : section.idents) {
                    if (expression == params->end())
                        return error.format("Number of arguments does not match the number of formal parameters")
                            .build();
                    if (section.var) {
                        auto procCall = dynamic_cast<ProcCall*>((*expression).get());
                        auto errVar = error.format("Expected variable type {}", (*expression)->to_string()).build();
                        if (!procCall)
                            return errVar;
                        auto info = procCall->get_info(table);
                        if (!info)
                            return info.get_err();
                        if (info.get_ok().group != SymbolGroup::VAR)
                            return errVar;
                    }
                    auto exprType = (*expression)->get_type(table);
                    if (!exprType)
                        return exprType.get_err();
                    if (*section.type != *exprType.get_ok()) {
                        auto etype = exprType.get_ok()->is<PointerType>();
                        auto stype = section.type->is<PointerType>();
                        TypeName const* ntype = nullptr;
                        if (stype)
                            ntype = stype->type->is<TypeName>();
                        if (stype && etype && ntype) {
                            if (!table.type_extends_base(etype->type.get(), ntype->ident)) {
                                return error.format("Type {} does not extends type {}", etype->type, stype->type)
                                    .build();
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
                auto type = (*funcType->params.rettype).get();
                if (auto isBaseType = type->is<BuiltInType>(); isBaseType) {
                    symbol.type = *funcType->params.rettype;
                } else if (auto isTypeName = type->is<TypeName>(); isTypeName) {
                    auto rettype = table.get_symbol(isTypeName->ident);
                    if (!rettype) return rettype.get_err();
                    symbol.type = rettype.get_ok().type;
                } else {
                    return error.format("Expected base type or type name, found {}", type->to_string()).build();
                }
                symbol.group = SymbolGroup::CONST;
                return symbol;
            }
        } else {
            return error.format("Expected procedure type, found {}", symbol.type.get()->to_string()).build();
        }
    }
}

TypeResult ProcCall::get_type(const SymbolTable& table) const {
    auto symbol = get_info(table);
    if (!symbol)
        return symbol.get_err();
    if (symbol.get_ok().type == nullptr)
        return ErrorBuilder(((Expression*)this)->place).text("This procedure call has no type").build();
    else
        return symbol.get_ok().type;
}

ExprResult ProcCall::eval(const SymbolTable& table) const {
    auto err = data.repair(table);
    if (err) return SemanticError{*err};
    auto ident = data.get().ident.get();
    if (!data.get().params && ident.selector.size() == 0) {
        return table.get_value(ident.ident);
    }
    return ErrorBuilder(place).format("Selection sequence cannot be constant: {}", this->to_string()).build();
}

std::string Tilda::to_string() const {
    return fmt::format("~{}", expression);
}

TypeResult Tilda::get_type(const SymbolTable& table) const {
    auto expr_type = expression->get_type(table);
    if (expr_type) {
        auto boolean = make_base_type(BaseType::BOOL);
        if (expr_type.get_ok() == boolean)
            return boolean;
        else
            return ErrorBuilder(this->place).exfound(boolean.get(), expr_type.get_ok().get()).build();
    } else
        return expr_type;
}

ExprResult Tilda::eval(const SymbolTable& table) const {
    auto res = expression->eval(table);
    if (!res)
        return res.get_err();
    auto boolean = dynamic_cast<Boolean*>(res.get_ok().get());
    if (!boolean)
        return ErrorBuilder(place).text("Expected boolean").build();
    return make_expression<Boolean>(!boolean->value);
}

std::string Term::to_string() const {
    std::string res = "";
    if (sign)
        res += *sign;
    if (oper)
        return res + fmt::format("({} {} {})", first, oper->value, *second);
    else
        return res + fmt::format("{}", first);
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

Term::Term(std::optional<char> s, ExpressionPtr f, std::optional<std::tuple<Operator, ExpressionPtr>> sec)
    : sign(s), first(f) {
    if (sec) {
        auto [op, se] = *sec;
        oper = op;
        second = se;
    }
}

Term::Term(ExpressionPtr f, std::optional<std::tuple<Operator, ExpressionPtr>> sec) : first(f) {
    if (sec) {
        auto [op, se] = *sec;
        oper = op;
        second = se;
    }
}
