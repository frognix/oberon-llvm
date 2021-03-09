#include "expression_nodes.hpp"

#include "node_formatters.hpp"
#include "symbol_table.hpp"

using namespace nodes;

std::string Number::to_string() const {
    return std::visit([](auto&& arg) { return fmt::format("{}", arg); }, value);
}

Maybe<TypePtr> Number::get_type(Context&) const {
    if (std::holds_alternative<Real>(value)) {
        return make_base_type(BaseType::REAL);
    } else {
        return make_base_type(BaseType::INTEGER);
    }
}

Maybe<ExpressionPtr> Number::eval(Context&) const {
    return make_expression<Number>(*this);
}

std::string Char::to_string() const {
    if (value < ' ' || value > '~')
        return fmt::format("{}X", (int)value);
    else
        return fmt::format("'{}'", value);
}

Maybe<TypePtr> Char::get_type(Context&) const {
    return make_base_type(BaseType::CHAR);
}

Maybe<ExpressionPtr> Char::eval(Context&) const {
    return make_expression<Char>(*this);
}

std::string String::to_string() const {
    return fmt::format("\"{}\"", fmt::join(value, ""));
}

Maybe<TypePtr> String::get_type(Context&) const {
    auto ch = make_base_type(BaseType::CHAR);
    auto size = make_expression<Number>(std::variant<Real, Integer>(Integer(value.size())));
    return make_type<ArrayType>(std::vector{size}, ch);
}

Maybe<ExpressionPtr> String::eval(Context&) const {
    return make_expression<String>(*this);
}

std::string Nil::to_string() const {
    return "NIL";
}

Maybe<TypePtr> Nil::get_type(Context&) const {
    return make_base_type(BaseType::NIL);
}

Maybe<ExpressionPtr> Nil::eval(Context&) const {
    return make_expression<Nil>(*this);
}

std::string Boolean::to_string() const {
    return fmt::format("{}", value);
}

Maybe<TypePtr> Boolean::get_type(Context&) const {
    return make_base_type(BaseType::BOOL);
}

Maybe<ExpressionPtr> Boolean::eval(Context&) const {
    return make_expression<Boolean>(*this);
}

std::string Set::to_string() const {
    return fmt::format("{{{}}}", fmt::join(value, ", "));
}

Maybe<TypePtr> Set::get_type(Context&) const {
    return make_base_type(BaseType::SET);
}

Maybe<ExpressionPtr> Set::eval(Context&) const {
    return make_expression<Set>(*this);
}

Set::Set(std::optional<std::vector<SetElement>> v) {
    if (v)
        value = *v;
}

inline Maybe<RecordType*> get_record_from_pointer(Type* some_type, Context& context, CodePlace place) {
    Type* start_type = some_type;
    if (auto typeName = some_type->is<TypeName>(); typeName) {
        auto type = typeName->dereference(context);
        if (!type)
            return error;
        some_type = type->get();
    }
    if (auto pointer = some_type->is<PointerType>(); pointer) {
        some_type = pointer->type.get();
        if (auto typeName = some_type->is<TypeName>(); typeName) {
            auto type = typeName->dereference(context);
            if (!type)
                return error;
            some_type = type->get();
        }
    }
    auto type = dynamic_cast<RecordType*>(some_type);
    if (!type) {
        context.messages.addErr(place, "Expected reord, found {}", start_type->to_string());
        return error;
    }
    return type;
}

Maybe<SymbolToken> Designator::get_symbol(Context& context, CodePlace place) const {
    auto res = context.symbols.get_symbol(context.messages, ident);
    if (!res) {
        return error;
    } else {
        auto symbol = *res;
        for (auto& sel : selector) {
            auto symbolType = symbol.type;
            if (symbol.group == SymbolGroup::TYPE) {
                context.messages.addErr(place, "Unexpected type name {}", symbol.name);
                return error;
            }
            if (auto ident = std::get_if<Ident>(&sel); ident) {
                auto typeRes = get_record_from_pointer(symbol.type.get(), context, place);
                if (!typeRes)
                    return error;
                auto type = *typeRes;
                if (auto fType = type->has_field(*ident, context); fType) {
                    symbol.type = *fType;
                } else {
                    context.messages.addErr(place, "Field {} not found in record {}", *ident, type->to_string());
                    return error;
                }
            } else if (auto ident = std::get_if<ExpList>(&sel); ident) {
                auto ltype = dynamic_cast<ArrayType*>(symbol.type.get());
                if (auto ntype = ltype->drop_dimensions(ident->size(), context); ntype) {
                    symbol.type = *ntype;
                } else {
                    return error;
                }
            } else if (auto ident = std::get_if<char>(&sel); ident) {
                auto type = dynamic_cast<PointerType*>(symbol.type.get());
                if (type) {
                    symbol.type = type->type;
                } else {
                    context.messages.addErr(place, "Expected pointer type, found {}", symbol.type->to_string());
                    return error;
                }
            } else if (auto ident = std::get_if<QualIdent>(&sel); ident) {
                auto ltype = get_record_from_pointer(symbol.type.get(), context, place);
                if (!ltype)
                    return error;
                if (auto rsymbol = context.symbols.get_symbol(context.messages, *ident); rsymbol) {
                    if (rsymbol->group == SymbolGroup::TYPE) {
                        if (ltype && context.symbols.type_extends_base(*ltype, *ident)) {
                            symbol.type = rsymbol->type;
                        }
                    } else {
                        context.messages.addErr(place, "Expected type, found {}", rsymbol->name);
                        return error;
                    }
                } else {
                    return error;
                }
            } else {
                context.messages.addErr(place, "Internal compiler error in ProcCall::get_type");
                return error;
            }
        }
        return symbol;
    }
}

bool nodes::designator_repair(Designator& value, Context& context) {
    if (value.ident.qual) {
        auto import = context.symbols.get_symbol(context.messages, nodes::QualIdent{{}, *value.ident.qual}, true);
        if (!import)
            return berror;
        if (import->group == SymbolGroup::VAR || import->group == SymbolGroup::CONST) {
            std::vector<Selector> new_selector{value.ident.ident};
            new_selector.insert(new_selector.end(), value.selector.begin(), value.selector.end());
            value.selector = new_selector;
            value.ident.ident = *value.ident.qual;
            value.ident.qual = {};
        } else if (import->group != SymbolGroup::MODULE) {
            context.messages.addErr(value.ident.qual->place, "Expected variable or module name");
            return berror;
        }
    }
    return bsuccess;
}

bool nodes::proccall_repair(ProcCallData& value, Context& context) {
    if (!value.ident.repair(context))
        return berror;
    auto& ident = value.ident.get_mut();
    if (ident.selector.size() > 0 && !value.params) {
        auto back_selecter = ident.selector.back();
        if (!value.params && std::holds_alternative<QualIdent>(back_selecter)) {
            auto desig = DesignatorRepairer(std::get<QualIdent>(back_selecter), std::vector<Selector>{});
            if (!desig.repair(context))
                return berror;
            ident.selector.pop_back();
            value.params = {make_expression<ProcCall>(desig.get(), std::optional<ExpList>{})};
        }
    }
    return true;
}

std::string ProcCall::to_string() const {
    auto params = data.unsafe_get().params;
    auto ident = data.unsafe_get().ident.unsafe_get();
    if (data.unsafe_get().params)
        return fmt::format("{}({})", ident.to_string(), fmt::join(*params, ", "));
    else
        return fmt::format("{}", ident.to_string());
}

Maybe<SymbolToken> ProcCall::get_info(Context& context) const {
    if (!data.repair(context))
        return error;
    auto ident = data.get().ident.get();
    auto params = data.get().params;
    auto res = ident.get_symbol(context, place);
    if (!res)
        return error;
    auto symbol = *res;
    if (!params) {
        return symbol;
    } else {
        auto funcType = dynamic_cast<ProcedureType*>(symbol.type.get());
        auto expression = params->begin();
        if (funcType) {
            for (auto& section : funcType->params.sections) {
                for (auto& param [[maybe_unused]] : section.idents) {
                    if (expression == params->end()) {
                        context.messages.addErr(place,
                                                "Number of arguments does not match the number of formal parameters");
                        return error;
                    }
                    if (section.var) {
                        auto procCall = dynamic_cast<ProcCall*>((*expression).get());
                        auto errMsg = Message(MPriority::ERR, place,
                                              fmt::format("Expected variable type {}", (*expression)->to_string()));
                        if (!procCall) {
                            context.messages.addMessage(errMsg);
                            return error;
                        }
                        auto info = procCall->get_info(context);
                        if (!info)
                            return error;
                        if (info->group != SymbolGroup::VAR) {
                            context.messages.addMessage(errMsg);
                            return error;
                        }
                    }
                    auto exprType = (*expression)->get_type(context);
                    if (!exprType)
                        return error;
                    if (*section.type != **exprType) {
                        auto etype = exprType.value()->is<PointerType>();
                        auto stype = section.type->is<PointerType>();
                        TypeName const* ntype = nullptr;
                        if (stype)
                            ntype = stype->type->is<TypeName>();
                        if (stype && etype && ntype) {
                            if (!context.symbols.type_extends_base(etype->type.get(), ntype->ident)) {
                                context.messages.addErr(place, "Type {} does not extends type {}", etype->type,
                                                        stype->type);
                                return error;
                            }
                        } else {
                            context.messages.addErr(place, "Expected {}, found {}", section.type,
                                                    exprType.value()->to_string());
                            return error;
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
                    auto rettype = context.symbols.get_symbol(context.messages, isTypeName->ident);
                    if (!rettype)
                        return error;
                    symbol.type = rettype->type;
                } else {
                    context.messages.addErr(place, "Expected base type or type name, found {}", type->to_string());
                    return error;
                }
                symbol.group = SymbolGroup::CONST;
                return symbol;
            }
        } else {
            context.messages.addErr(place, "Expected procedure type, found {}", symbol.type.get()->to_string());
            return error;
        }
    }
}

Maybe<TypePtr> ProcCall::get_type(Context& context) const {
    auto symbol = get_info(context);
    if (!symbol)
        return error;
    if (symbol->type == nullptr) {
        context.messages.addErr(place, "This procedure call has no type");
        return error;
    } else {
        return symbol->type;
    }
}

Maybe<ExpressionPtr> ProcCall::eval(Context& context) const {
    if (!data.repair(context))
        return error;
    auto ident = data.get().ident.get();
    if (!data.get().params && ident.selector.size() == 0) {
        return context.symbols.get_value(context.messages, ident.ident);
    }
    context.messages.addErr(place, "Selection sequence cannot be constant: {}", this->to_string());
    return error;
}

std::string Tilda::to_string() const {
    return fmt::format("~{}", expression);
}

Maybe<TypePtr> Tilda::get_type(Context& context) const {
    auto expr_type = expression->get_type(context);
    if (expr_type) {
        auto boolean = make_base_type(BaseType::BOOL);
        if (*expr_type == boolean)
            return boolean;
        else {
            context.messages.addErr(place, "Expected {}, found {}", boolean->to_string(),
                                    expr_type.value()->to_string());
            return error;
        }
    } else
        return expr_type;
}

Maybe<ExpressionPtr> Tilda::eval(Context& context) const {
    auto res = expression->eval(context);
    if (!res)
        return error;
    auto boolean = dynamic_cast<Boolean*>(res->get());
    if (!boolean) {
        context.messages.addErr(place, "Expected boolean");
        return error;
    }
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
Maybe<TypePtr> Term::get_type(Context& context) const {
    if (!sign && !oper && !second) {
        return first->get_type(context);
    } else {
        return make_type<BuiltInType>(str_to_ident("INTEGER"));
    }
}

Maybe<ExpressionPtr> Term::eval(Context& context) const {
    if (!sign && !oper && !second) {
        return first->eval(context);
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
