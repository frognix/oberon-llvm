#include "expression_nodes.hpp"

#include "internal_error.hpp"
#include "node_formatters.hpp"
#include "symbol_table.hpp"
#include "type.hpp"
#include "type_nodes.hpp"

using namespace nodes;

std::string ConstInteger::to_string() const {
    return fmt::format("{}", value);
}

Maybe<std::pair<SymbolGroup, TypePtr>> ConstInteger::get_type(Context&) const {
    return std::pair(SymbolGroup::CONST, make_base_type(BaseType::INTEGER));
}

Maybe<ExpressionPtr> ConstInteger::eval_constant(Context&) const {
    return make_expression<ConstInteger>(*this);
}

std::string ConstReal::to_string() const {
    return fmt::format("{}", value);
}

Maybe<std::pair<SymbolGroup, TypePtr>> ConstReal::get_type(Context&) const {
    return std::pair(SymbolGroup::CONST, make_base_type(BaseType::REAL));
}

Maybe<ExpressionPtr> ConstReal::eval_constant(Context&) const {
    return make_expression<ConstReal>(*this);
}



std::string Char::to_string() const {
    if (value < ' ' || value > '~')
        return fmt::format("{}X", (int)value);
    else
        return fmt::format("'{}'", value);
}

Maybe<std::pair<SymbolGroup, TypePtr>> Char::get_type(Context&) const {
    return std::pair(SymbolGroup::CONST, make_base_type(BaseType::CHAR));
}

Maybe<ExpressionPtr> Char::eval_constant(Context&) const {
    return make_expression<Char>(*this);
}

std::string String::to_string() const {
    return fmt::format("\"{}\"", fmt::join(value, ""));
}

Maybe<std::pair<SymbolGroup, TypePtr>> String::get_type(Context&) const {
    return std::pair(SymbolGroup::CONST, make_type<ConstStringType>(value.size()));
}

Maybe<ExpressionPtr> String::eval_constant(Context&) const {
    return make_expression<String>(*this);
}

std::string Nil::to_string() const {
    return "NIL";
}

Maybe<std::pair<SymbolGroup, TypePtr>> Nil::get_type(Context&) const {
    return std::pair(SymbolGroup::CONST, make_base_type(BaseType::NIL));
}

Maybe<ExpressionPtr> Nil::eval_constant(Context&) const {
    return make_expression<Nil>(*this);
}

std::string Boolean::to_string() const {
    return fmt::format("{}", value);
}

Maybe<std::pair<SymbolGroup, TypePtr>> Boolean::get_type(Context&) const {
    return std::pair(SymbolGroup::CONST, make_base_type(BaseType::BOOL));
}

Maybe<ExpressionPtr> Boolean::eval_constant(Context&) const {
    return make_expression<Boolean>(*this);
}

std::string Set::to_string() const {
    return fmt::format("{{{}}}", fmt::join(value, ", "));
}

Maybe<std::pair<SymbolGroup, TypePtr>> Set::get_type(Context&) const {
    return std::pair(SymbolGroup::CONST, make_base_type(BaseType::SET));
}

Maybe<ExpressionPtr> Set::eval_constant(Context&) const {
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
                auto typeSymbol = context.symbols.get_symbol(context.messages, *ident);
                if (!typeSymbol) return error;
                if (assignment_compatible_types(context, *symbol.type, *typeSymbol->type)) {
                    symbol.type = typeSymbol->type;
                } else {
                    context.messages.addErr(place, "'{}' is not an extension of '{}'", typeSymbol->type->to_string(), symbol.type->to_string());
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
            auto desigRep = DesignatorRepairer(std::get<QualIdent>(back_selecter), std::vector<Selector>{});
            if (!desigRep.repair(context))
                return berror;
            auto desig = desigRep.get();
            auto symbol = context.symbols.get_symbol(context.messages, desig.ident);
            if (desig.selector.size() == 0 && symbol->group == SymbolGroup::TYPE) {
                return bsuccess;
            } else {
                ident.selector.pop_back();
                value.params = {make_expression<ProcCall>(desig, std::optional<ExpList>{})};
            }
        }
    }
    return bsuccess;
}

std::string ProcCall::to_string() const {
    auto params = data.unsafe_get().params;
    auto ident = data.unsafe_get().ident.unsafe_get();
    if (data.unsafe_get().params)
        return fmt::format("{}({})", ident.to_string(), fmt::join(*params, ", "));
    else
        return fmt::format("{}", ident.to_string());
}

Maybe<std::pair<SymbolGroup, TypePtr>> ProcCall::get_info(Context& context) const {
    if (!data.repair(context)) return error;
    auto ident = data.get().ident.get();
    auto params = data.get().params;
    auto symbolRes = ident.get_symbol(context, place);
    if (!symbolRes)
        return error;
    auto symbol = *symbolRes;
    if (!params) return std::pair(symbol.group, symbol.type);

    auto funcType = dynamic_cast<ProcedureType*>(symbol.type.get());
    if (!funcType) {
        context.messages.addErr(place, "Expected procedure type, found {}", symbol.type.get()->to_string());
        return error;
    }
    if (params->size() != funcType->params.params.size()) {
        context.messages.addErr(place, "Expected {} parameters, found {}", funcType->params.params.size(), params->size());
        return error;
    }
    auto compatible_types = true;
    for (size_t i = 0; i < funcType->params.params.size(); i++) {
        auto var = funcType->params.params[i];
        auto expr = (*params)[i];
        auto exprRes = expr->get_type(context);
        if (!exprRes) return error;
        auto [group, exprType] = *exprRes;
        if (var.var && group != SymbolGroup::VAR) {
            context.messages.addErr(expr->place, "Expected variable");
            compatible_types = false;
        }
        if (!assignment_compatible_types(context, *var.type, *exprType) && !array_compatible(context, *exprType, *var.type)) {
            context.messages.addErr(place, "Can't match actual and formal parameter: {} and {}", exprType->to_string(), var.type->to_string());
            compatible_types = false;
        }
    }
    if (!compatible_types) {
        return error;
    }
    TypePtr return_type;
    if (funcType->params.rettype) return_type = *funcType->params.rettype;
    return std::pair(SymbolGroup::CONST, return_type);
}

Maybe<std::pair<SymbolGroup, TypePtr>> ProcCall::get_type(Context& context) const {
    auto res = get_info(context);
    if (!res)
        return error;
    auto [group, type] = *res;
    if (type == nullptr) {
        context.messages.addErr(place, "This procedure call has no type");
        return error;
    } else {
        return *res;
    }
}

Maybe<ExpressionPtr> ProcCall::eval_constant(Context& context) const {
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

Maybe<std::pair<SymbolGroup, TypePtr>> Tilda::get_type(Context& context) const {
    auto exprRes = expression->get_type(context);
    if (!exprRes) {
        return error;
    }
    auto [group, type] = *exprRes;
    if (auto base = type->is<BuiltInType>(); base && base->equal_to(BaseType::BOOL))
        return *exprRes;
    else {
        context.messages.addErr(place, "Expected Boolean, found {}", type->to_string());
        return error;
    }
}

Maybe<ExpressionPtr> Tilda::eval_constant(Context& context) const {
    auto res = expression->eval_constant(context);
    if (!res)
        return error;
    auto boolean = dynamic_cast<Boolean*>(res->get());
    if (!boolean) {
        context.messages.addErr(place, "Expected boolean");
        return error;
    }
    return make_expression<Boolean>(!boolean->value);
}

OpType ident_to_optype(const Ident& i) {
    if (i.equal_to("+")) return OpType::ADD;
    if (i.equal_to("-")) return OpType::SUB;
    if (i.equal_to("*")) return OpType::MUL;
    if (i.equal_to("/")) return OpType::RDIV;
    if (i.equal_to("DIV")) return OpType::IDIV;
    if (i.equal_to("MOD")) return OpType::MOD;
    if (i.equal_to("OR")) return OpType::OR;
    if (i.equal_to("&")) return OpType::AND;
    if (i.equal_to("=")) return OpType::EQ;
    if (i.equal_to("#")) return OpType::NEQ;
    if (i.equal_to("<")) return OpType::LT;
    if (i.equal_to("<=")) return OpType::LTE;
    if (i.equal_to(">")) return OpType::GT;
    if (i.equal_to(">=")) return OpType::GTE;
    if (i.equal_to("IN")) return OpType::IN;
    if (i.equal_to("IS")) return OpType::IS;
    internal::compiler_error(fmt::format("Unexpected operator: '{}'", i));
}

const char* optype_to_str(OpType type) {
    switch (type) {
        case OpType::ADD: return "+";
        case OpType::SUB: return "-";
        case OpType::MUL: return "*";
        case OpType::RDIV: return "/";
        case OpType::IDIV: return "DIV";
        case OpType::MOD: return "MOD";
        case OpType::OR: return "OR";
        case OpType::AND: return "&";
        case OpType::EQ: return "=";
        case OpType::NEQ: return "#";
        case OpType::LT: return "<";
        case OpType::LTE: return "<=";
        case OpType::GT: return ">";
        case OpType::GTE: return ">=";
        case OpType::IN: return "IN";
        case OpType::IS: return "IS";
        default: internal::compiler_error("Unexpected operator type");
    }
}

Operator::Operator(Ident v) : value(ident_to_optype(v)) {}
Operator::Operator(char v) : value(ident_to_optype(Ident({v}))) {}

inline bool same_or(Context& context, const Type& expr, const Type& type1, const Type& type2) {
    return same_types(context, expr, type1) || same_types(context, expr, type2);
}

inline bool same_or_base(Context& context, const Type& expr, BaseType type1, BaseType type2) {
    return  same_or(context, expr, BuiltInType(type1), BuiltInType(type2));
}

struct OpTableSubLine {
    std::vector<BaseType> first;
    std::vector<BaseType> second;
    BaseType result;
};

struct OpTableLine {
    std::vector<OpType> oper;
    std::vector<OpTableSubLine> sublines;
};

const std::vector<OpTableLine> optable {
    {{OpType::ADD, OpType::SUB, OpType::MUL}, {
            OpTableSubLine{{BaseType::INTEGER, BaseType::BYTE}, {BaseType::INTEGER, BaseType::BYTE}, BaseType::INTEGER},
            OpTableSubLine{{BaseType::REAL}, {BaseType::REAL}, BaseType::REAL}
        }},
    {{OpType::ADD, OpType::SUB, OpType::MUL, OpType::RDIV}, {
            OpTableSubLine{{BaseType::SET}, {BaseType::SET}, BaseType::SET}
        }},
    {{OpType::IDIV, OpType::MOD}, {
            OpTableSubLine{{BaseType::INTEGER, BaseType::BYTE}, {BaseType::INTEGER, BaseType::BYTE}, BaseType::INTEGER}
        }},
    {{OpType::OR, OpType::AND}, {
            OpTableSubLine{{BaseType::BOOL}, {BaseType::BOOL}, BaseType::BOOL}
        }},
    {{OpType::EQ, OpType::NEQ, OpType::LT, OpType::LTE, OpType::GT, OpType::GTE}, {
            OpTableSubLine{{BaseType::CHAR}, {BaseType::CHAR}, BaseType::BOOL},
            OpTableSubLine{{BaseType::INTEGER, BaseType::BYTE}, {BaseType::INTEGER, BaseType::BYTE}, BaseType::BOOL},
            OpTableSubLine{{BaseType::REAL}, {BaseType::REAL}, BaseType::BOOL}
        }},
    {{OpType::EQ, OpType::NEQ}, {
            OpTableSubLine{{BaseType::BOOL}, {BaseType::BOOL}, BaseType::BOOL},
            OpTableSubLine{{BaseType::SET}, {BaseType::SET}, BaseType::BOOL}
        }},
    {{OpType::IN}, {
            OpTableSubLine{{BaseType::INTEGER, BaseType::BYTE}, {BaseType::SET}, BaseType::BOOL}
        }}
};

Maybe<BaseType> nodes::expression_compatible(Context& context, CodePlace place, const Type& left, OpType oper, const Type& right) {
    auto lbase = left.is<BuiltInType>();
    auto rbase = right.is<BuiltInType>();
    if (lbase && rbase) {
        auto left = lbase->type;
        auto right = rbase->type;
        for (auto& line : optable) {
            bool oper_check = false;
            for (auto oper_var : line.oper) {
                if (oper == oper_var) {
                    oper_check = true;
                    break;
                }
            }
            if (!oper_check) continue;
            for (auto subline : line.sublines) {
                bool left_check = false;
                for (auto left_var : subline.first) {
                    if (left == left_var) {
                        left_check = true;
                        break;
                    }
                }
                bool right_check = false;
                for (auto right_var : subline.second) {
                    if (right == right_var) {
                        right_check = true;
                        break;
                    }
                }
                 if (left_check && right_check) return subline.result;
            }
        }
    }
    if (oper == OpType::EQ || oper == OpType::NEQ || oper == OpType::LT || oper == OpType::LTE || oper == OpType::GT || oper == OpType::GTE) {
        if ((same_types(context, left, BuiltInType(BaseType::CHAR)) || same_types(context, left, ConstStringType(1)))
            && (same_types(context, right, BuiltInType(BaseType::CHAR)) || same_types(context, right, ConstStringType(1))))
            return BaseType::BOOL;
    }
    if (oper == OpType::EQ || oper == OpType::NEQ) {
        if ((same_types(context, left, BuiltInType(BaseType::NIL)) || left.is<PointerType>())
            && (same_types(context, right, BuiltInType(BaseType::NIL)) || right.is<PointerType>()))
            return BaseType::BOOL;
        if ((same_types(context, left, BuiltInType(BaseType::NIL)) || left.is<ProcedureType>())
            && (same_types(context, right, BuiltInType(BaseType::NIL)) || right.is<ProcedureType>()))
        return BaseType::BOOL;
    }
    if (oper == OpType::IS) {
        if (auto rrecord = right.is<RecordType>(); rrecord) {
            auto lrecord = left.is<RecordType>();
            if (lrecord && rrecord->extends(context, *rrecord)) return BaseType::BOOL;
            auto lpointer = left.is<PointerType>();
            if (lpointer) {
                auto& type = lpointer->get_type(context);
                if (rrecord->extends(context, type)) return BaseType::BOOL;
            }
        }
    }
    context.messages.addErr(place, "Incompatible types for '{}' operator: {} and {}", optype_to_str(oper), left.to_string(), right.to_string());
    return error;
}

std::string Term::to_string() const {
    std::string res = "";
    if (sign)
        res += *sign;
    if (oper)
        return res + fmt::format("({} {} {})", first, optype_to_str(oper->value), *second);
    else
        return res + fmt::format("{}", first);
}

///! \todo
Maybe<std::pair<SymbolGroup, TypePtr>> Term::get_type(Context& context) const {
    if (!sign && !oper && !second) {
        return first->get_type(context);
    } else if (oper && second) {
        auto firstRes = first->get_type(context);
        if (!firstRes) return error;
        auto [firstGroup, firstType] = *firstRes;
        auto secondRes = second.value()->get_type(context);
        if (!secondRes) return error;
        auto [secondGroup, secondType] = *secondRes;
        if (auto type = expression_compatible(context, place, *firstType, oper->value, *secondType); type) {
            return std::pair(SymbolGroup::CONST, make_type<BuiltInType>(*type));
        } else {
            return error;
        }
    } else {
        internal::compiler_error("Invalid term");
    }
}

Maybe<ExpressionPtr> Term::eval_constant(Context& context) const {
    if (!sign && !oper && !second) {
        return first->eval_constant(context);
    } else {
        return make_expression<ConstInteger>(10);
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
