#include "expression_nodes.hpp"

#include "expression.hpp"
#include "internal_error.hpp"
#include "node_formatters.hpp"
#include "type.hpp"
#include "type_nodes.hpp"
#include "symbol_table.hpp"
#include <string>
#include <string_view>

using namespace nodes;

std::string ConstInteger::to_string() const {
    return fmt::format("{}", value);
}

Maybe<std::pair<SymbolGroup, TypePtr>> ConstInteger::get_type(Context&) const {
    return std::pair(SymbolGroup::CONST, make_base_type(BaseType::INTEGER));
}

Maybe<ValuePtr> ConstInteger::eval_constant(Context&) const {
    return make_value<ConstInteger>(*this);
}

Maybe<ValuePtr> incompatible_types(Context& context, OpType oper, const Value& left, const Value& right) {
    auto left_res = left.get_type(context);
    auto right_res = right.get_type(context);
    if (!left_res || !right_res) return error;
    auto [left_group, left_type] = left_res.value();
    auto [right_group, right_type] = right_res.value();
    context.messages.addErr(left.place, "Incompatible types for operator '{}' : {} and {}", optype_to_str(oper), left_type->to_string(), right_type->to_string());
    return error;
}

bool compare_values(OpType oper, auto left, auto right) {
    bool result;
    switch (oper) {
        case OP_EQ:   result = left == right; break;
        case OP_NEQ:  result = left != right; break;
        case OP_LT:   result = left <  right; break;
        case OP_LTE:  result = left <= right; break;
        case OP_GT:   result = left >  right; break;
        case OP_GTE:  result = left >= right; break;
        default: internal::compiler_error(__FUNCTION__);
    }
    return result;
}

Maybe<ValuePtr> ConstInteger::apply_operator(Context& context, OpType oper, const Value& other) const {
    if (auto pinteger = other.is<ConstInteger>(); pinteger) {
        if (OP_COMPARE & oper)
            return make_value<Boolean>(compare_values(oper, value, pinteger->value));
        Integer result;
        switch (oper) {
            case OP_ADD:  result = value +  pinteger->value; break;
            case OP_SUB:  result = value -  pinteger->value; break;
            case OP_MUL:  result = value *  pinteger->value; break;
            case OP_IDIV: result = value /  pinteger->value; break;
            case OP_MOD:  result = value %  pinteger->value; break;
            default: return incompatible_types(context, oper, *this, other);
        }
        return make_value<ConstInteger>(result);
    } else if (auto pset = other.is<ConstSet>(); pset) {
        if (oper != OP_IN) return incompatible_types(context, oper, *this, other);
        return make_value<Boolean>(pset->values.contains(value));
    } else {
        return incompatible_types(context, oper, *this, other);
    }
}

std::string ConstReal::to_string() const {
    return fmt::format("{}", value);
}

Maybe<std::pair<SymbolGroup, TypePtr>> ConstReal::get_type(Context&) const {
    return std::pair(SymbolGroup::CONST, make_base_type(BaseType::REAL));
}

Maybe<ValuePtr> ConstReal::eval_constant(Context&) const {
    return make_value<ConstReal>(*this);
}

Maybe<ValuePtr> ConstReal::apply_operator(Context& context, OpType oper, const Value& other) const {
    if (auto preal = other.is<ConstReal>(); preal) {
        if (OP_COMPARE & oper)
            return make_value<Boolean>(compare_values(oper, value, preal->value));
        Real result;
        switch (oper) {
            case OP_ADD:  result = value + preal->value; break;
            case OP_SUB:  result = value - preal->value; break;
            case OP_MUL:  result = value * preal->value; break;
            case OP_RDIV: result = value / preal->value; break;
            default: return incompatible_types(context, oper, *this, other);
        }
        return make_value<ConstReal>(result);
    } else {
        return incompatible_types(context, oper, *this, other);
    }
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

Maybe<ValuePtr> Char::eval_constant(Context&) const {
    return make_value<Char>(*this);
}

Maybe<ValuePtr> Char::apply_operator(Context& context, OpType oper, const Value& other) const {
    if (!(OP_COMPARE & oper)) return incompatible_types(context, oper, *this, other);
    if (auto pchar = other.is<Char>(); pchar) {
        return make_value<Boolean>(compare_values(oper, value, pchar->value));
    } else if (auto pstring = other.is<String>(); pstring && pstring->value.size() == 1) {
        return make_value<Boolean>(compare_values(oper, value, pstring->value[0]));
    } else {
        return incompatible_types(context, oper, *this, other);
    }
}

std::string String::to_string() const {
    return fmt::format("\"{}\"", fmt::join(value, ""));
}

Maybe<std::pair<SymbolGroup, TypePtr>> String::get_type(Context&) const {
    return std::pair(SymbolGroup::CONST, make_type<ConstStringType>(value.size()));
}

Maybe<ValuePtr> String::eval_constant(Context&) const {
    return make_value<String>(*this);
}

Maybe<ValuePtr> String::apply_operator(Context& context, OpType oper, const Value& other) const {
    if (!(OP_COMPARE & oper)) return incompatible_types(context, oper, *this, other);
    if (auto pchar = other.is<Char>(); pchar && value.size() == 1) {
        return make_value<Boolean>(compare_values(oper, value[0], pchar->value));
    } else if (auto pstring = other.is<String>(); pstring) {
        return make_value<Boolean>(compare_values(oper, value, pstring->value));
    } else {
        return incompatible_types(context, oper, *this, other);
    }
}

std::string Nil::to_string() const {
    return "NIL";
}

Maybe<std::pair<SymbolGroup, TypePtr>> Nil::get_type(Context&) const {
    return std::pair(SymbolGroup::CONST, make_base_type(BaseType::NIL));
}

Maybe<ValuePtr> Nil::eval_constant(Context&) const {
    return make_value<Nil>(*this);
}

Maybe<ValuePtr> Nil::apply_operator(Context& context, OpType oper, const Value& other) const {
    if (auto pnil = other.is<Nil>(); pnil) {
        bool result;
        switch (oper) {
            case OP_EQ:  result = true;  break;
            case OP_NEQ: result = false; break;
            default: return incompatible_types(context, oper, *this, other);
        }
        return make_value<Boolean>(result);
    } else {
        return incompatible_types(context, oper, *this, other);
    }
}

std::string Boolean::to_string() const {
    return fmt::format("{}", value);
}

Maybe<std::pair<SymbolGroup, TypePtr>> Boolean::get_type(Context&) const {
    return std::pair(SymbolGroup::CONST, make_base_type(BaseType::BOOL));
}

Maybe<ValuePtr> Boolean::eval_constant(Context&) const {
    return make_value<Boolean>(*this);
}

Maybe<ValuePtr> Boolean::apply_operator(Context& context, OpType oper, const Value& other) const {
    if (auto pbool = other.is<Boolean>(); pbool) {
        bool result;
        switch (oper) {
            case OP_OR:  result = value || pbool->value; break;
            case OP_AND: result = value && pbool->value; break;
            case OP_EQ:  result = value == pbool->value; break;
            case OP_NEQ: result = value != pbool->value; break;
            default: return incompatible_types(context, oper, *this, other);
        }
        return make_value<Boolean>(result);
    } else {
        return incompatible_types(context, oper, *this, other);
    }
}

std::string ConstSet::to_string() const {
    return fmt::format("{{{}}}", fmt::join(values, ", "));
}

Maybe<std::pair<SymbolGroup, TypePtr>> ConstSet::get_type(Context&) const {
    return std::pair(SymbolGroup::CONST, make_base_type(BaseType::SET));
}

Maybe<ValuePtr> ConstSet::eval_constant(Context&) const {
    return make_value<ConstSet>(*this);
}

Maybe<ValuePtr> ConstSet::apply_operator(Context& context, OpType oper, const Value& other) const {
    if (auto pset = other.is<ConstSet>(); pset) {
        if (OP_COMPARE & oper) {
            bool result;
            switch (oper) {
                case OP_EQ: result = values == pset->values; break;
                case OP_NEQ: result = values != pset->values; break;
                default: return incompatible_types(context, oper, *this, other);
            }
            return make_value<Boolean>(result);
        } else {
             std::set<Integer> result;
             switch (oper) {
                 case OP_ADD:  std::set_union(values.begin(), values.end(),
                                              pset->values.begin(), pset->values.end(),
                                              std::inserter(result, result.begin()));
                 break;
                 case OP_SUB:  std::set_difference(values.begin(), values.end(),
                                                   pset->values.begin(), pset->values.end(),
                                                   std::inserter(result, result.begin()));
                 break;
                 case OP_MUL:  std::set_intersection(values.begin(), values.end(),
                                                     pset->values.begin(), pset->values.end(),
                                                     std::inserter(result, result.begin()));
                 break;
                 case OP_RDIV: std::set_symmetric_difference(values.begin(), values.end(),
                                                             pset->values.begin(), pset->values.end(),
                                                             std::inserter(result, result.begin()));
                 break;
                 default: return incompatible_types(context, oper, *this, other);
             }
             return make_value<ConstSet>(result);
        }
    } else {
        return incompatible_types(context, oper, *this, other);
    }
}

std::string Set::to_string() const {
    return fmt::format("{{{}}}", fmt::join(value, ", "));
}

Maybe<std::pair<SymbolGroup, TypePtr>> Set::get_type(Context&) const {
    return std::pair(SymbolGroup::CONST, make_base_type(BaseType::SET));
}

inline Maybe<ConstInteger> check_value(Context& context, const Expression& expr) {
    auto value_res = expr.eval_constant(context);
    if (!value_res) return error;
    auto value_int = value_res.value()->is<ConstInteger>();
    if (!value_int) {
        context.messages.addErr(expr.place, "Exprected integer value");
        return error;
    }
    return *value_int;
}

Maybe<ValuePtr> Set::eval_constant(Context& context) const {
    std::set<Integer> set;
    for (auto elem : value) {
        auto first_int = check_value(context, *elem.first);
        if (!first_int) return error;
        if (elem.second) {
            auto second_int = check_value(context, *elem.second.value());
            if (!second_int) return error;
            for (Integer i = first_int->value; i <= second_int->value; ++i) {
                set.insert(i);
            }
        } else {
            set.insert(first_int->value);
        }
    }
    return make_value<ConstSet>(set);
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
                if (symbol.type->assignment_compatible(context, *typeSymbol->type)) {
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
        if (!var.type->assignment_compatible(context, *exprType) && !ArrayType::compatible(context, *exprType, *var.type)) {
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

Maybe<ValuePtr> ProcCall::eval_constant(Context& context) const {
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

Maybe<ValuePtr> Tilda::eval_constant(Context& context) const {
    auto res = expression->eval_constant(context);
    if (!res)
        return error;
    auto boolean = dynamic_cast<Boolean*>(res->get());
    if (!boolean) {
        context.messages.addErr(place, "Expected boolean");
        return error;
    }
    return make_value<Boolean>(!boolean->value);
}

OpType ident_to_optype(std::string_view i) {
    if (i == "+") return OP_ADD;
    if (i == "-") return OP_SUB;
    if (i == "*") return OP_MUL;
    if (i == "/") return OP_RDIV;
    if (i == "DIV") return OP_IDIV;
    if (i == "MOD") return OP_MOD;
    if (i == "OR") return OP_OR;
    if (i == "&") return OP_AND;
    if (i == "=") return OP_EQ;
    if (i == "#") return OP_NEQ;
    if (i == "<") return OP_LT;
    if (i == "<=") return OP_LTE;
    if (i == ">") return OP_GT;
    if (i == ">=") return OP_GTE;
    if (i == "IN") return OP_IN;
    if (i == "IS") return OP_IS;
    internal::compiler_error(fmt::format("Unexpected operator: '{}'", i));
}

const char* nodes::optype_to_str(OpType type) {
    switch (type) {
        case OP_ADD: return "+";
        case OP_SUB: return "-";
        case OP_MUL: return "*";
        case OP_RDIV: return "/";
        case OP_IDIV: return "DIV";
        case OP_MOD: return "MOD";
        case OP_OR: return "OR";
        case OP_AND: return "&";
        case OP_EQ: return "=";
        case OP_NEQ: return "#";
        case OP_LT: return "<";
        case OP_LTE: return "<=";
        case OP_GT: return ">";
        case OP_GTE: return ">=";
        case OP_IN: return "IN";
        case OP_IS: return "IS";
        default: internal::compiler_error("Unexpected operator type");
    }
}

Operator::Operator(Ident v) : value(ident_to_optype({v.value.data(), v.value.size()})) {}
Operator::Operator(std::string_view v) : value(ident_to_optype(v)) {}

inline bool same_or(Context& context, const Type& expr, const Type& type1, const Type& type2) {
    return expr.same(context, type1) || expr.same(context, type2);
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
    {{OP_ADD, OP_SUB, OP_MUL}, {
            OpTableSubLine{{BaseType::INTEGER, BaseType::BYTE}, {BaseType::INTEGER, BaseType::BYTE}, BaseType::INTEGER},
            OpTableSubLine{{BaseType::REAL}, {BaseType::REAL}, BaseType::REAL}
        }},
    {{OP_ADD, OP_SUB, OP_MUL, OP_RDIV}, {
            OpTableSubLine{{BaseType::SET}, {BaseType::SET}, BaseType::SET}
        }},
    {{OP_IDIV, OP_MOD}, {
            OpTableSubLine{{BaseType::INTEGER, BaseType::BYTE}, {BaseType::INTEGER, BaseType::BYTE}, BaseType::INTEGER}
        }},
    {{OP_OR, OP_AND}, {
            OpTableSubLine{{BaseType::BOOL}, {BaseType::BOOL}, BaseType::BOOL}
        }},
    {{OP_EQ, OP_NEQ, OP_LT, OP_LTE, OP_GT, OP_GTE}, {
            OpTableSubLine{{BaseType::CHAR}, {BaseType::CHAR}, BaseType::BOOL},
            OpTableSubLine{{BaseType::INTEGER, BaseType::BYTE}, {BaseType::INTEGER, BaseType::BYTE}, BaseType::BOOL},
            OpTableSubLine{{BaseType::REAL}, {BaseType::REAL}, BaseType::BOOL}
        }},
    {{OP_EQ, OP_NEQ}, {
            OpTableSubLine{{BaseType::BOOL}, {BaseType::BOOL}, BaseType::BOOL},
            OpTableSubLine{{BaseType::SET}, {BaseType::SET}, BaseType::BOOL}
        }},
    {{OP_IN}, {
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
    if (oper == OP_EQ || oper == OP_NEQ || oper == OP_LT || oper == OP_LTE || oper == OP_GT || oper == OP_GTE) {
        if ((left.same(context, BuiltInType(BaseType::CHAR)) || left.same(context, ConstStringType(1)))
            && (right.same(context, BuiltInType(BaseType::CHAR)) || right.same(context, ConstStringType(1))))
            return BaseType::BOOL;
    }
    if (oper == OP_EQ || oper == OP_NEQ) {
        if ((left.same(context, BuiltInType(BaseType::NIL)) || left.is<PointerType>())
            && (right.same(context, BuiltInType(BaseType::NIL)) || right.is<PointerType>()))
            return BaseType::BOOL;
        if ((left.same(context, BuiltInType(BaseType::NIL)) || left.is<ProcedureType>())
            && (right.same(context, BuiltInType(BaseType::NIL)) || right.is<ProcedureType>()))
        return BaseType::BOOL;
    }
    if (oper == OP_IS) {
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

Maybe<ValuePtr> Term::eval_constant(Context& context) const {
    if (!sign && !oper && !second) {
        return first->eval_constant(context);
    } else {
        auto firstRes = first->eval_constant(context);
        if (!firstRes) return error;
        auto secondRes = second.value()->eval_constant(context);
        if (!secondRes) return error;
        return firstRes.value()->apply_operator(context, oper->value, *secondRes.value());
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
