#include "expression_nodes.hpp"

#include "expression.hpp"
#include "internal_error.hpp"
#include "node.hpp"
#include "node_formatters.hpp"
#include "semantic_context.hpp"
#include "symbol_token.hpp"
#include "type.hpp"
#include "type_nodes.hpp"
#include "symbol_table.hpp"
#include <bits/stdint-uintn.h>
#include <cmath>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <algorithm>
#include <ranges>

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
        return make_value<Boolean>(pset->values.test(value));
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
    std::vector<uint> set_values;
    for (uint i = 0; i < values.size(); ++i) {
        if (values.test(i)) set_values.push_back(i);
    }
    return fmt::format("{{{}}}", fmt::join(set_values, ", "));
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
            ConstSet::SetType result = values;
             switch (oper) {
                 case OP_ADD:  result |= pset->values; break;
                 case OP_SUB:  result |= ~pset->values; break;
                 case OP_MUL:  result &= pset->values; break;
                 case OP_RDIV: result ^= pset->values; break;
                 default: return incompatible_types(context, oper, *this, other);
             }
             return make_value<ConstSet>(result);
        }
    } else {
        return incompatible_types(context, oper, *this, other);
    }
}

constexpr const char* show_bptype(BPType type) {
    switch (type) {
        case BPType::ABS:    return "ABS";
        case BPType::ODD:    return "ODD";
        case BPType::LEN:    return "LEN";
        case BPType::LSL:    return "LSL";
        case BPType::ASR:    return "ASR";
        case BPType::ROR:    return "ROR";
        case BPType::FLOOR:  return "FLOOR";
        case BPType::FLT:    return "FLT";
        case BPType::ORD:    return "ORD";
        case BPType::CHR:    return "CHR";
        case BPType::INC:    return "INC";
        case BPType::DEC:    return "DEC";
        case BPType::INCL:   return "INCL";
        case BPType::EXCL:   return "EXCL";
        case BPType::NEW:    return "NEW";
        case BPType::ASSERT: return "ASSERT";
        case BPType::PACK:   return "PACK";
        case BPType::UNPK:   return "UNPK";
        default: internal::compiler_error(__FUNCTION__);
    }
}

BPType read_bptype(std::string_view type) {
    if (type == show_bptype(BPType::ABS))    return BPType::ABS;
    if (type == show_bptype(BPType::ODD))    return BPType::ODD;
    if (type == show_bptype(BPType::LEN))    return BPType::LEN;
    if (type == show_bptype(BPType::LSL))    return BPType::LSL;
    if (type == show_bptype(BPType::ASR))    return BPType::ASR;
    if (type == show_bptype(BPType::ROR))    return BPType::ROR;
    if (type == show_bptype(BPType::FLOOR))  return BPType::FLOOR;
    if (type == show_bptype(BPType::FLT))    return BPType::FLT;
    if (type == show_bptype(BPType::ORD))    return BPType::ORD;
    if (type == show_bptype(BPType::CHR))    return BPType::CHR;
    if (type == show_bptype(BPType::INC))    return BPType::INC;
    if (type == show_bptype(BPType::DEC))    return BPType::DEC;
    if (type == show_bptype(BPType::INCL))   return BPType::INCL;
    if (type == show_bptype(BPType::EXCL))   return BPType::EXCL;
    if (type == show_bptype(BPType::NEW))    return BPType::NEW;
    if (type == show_bptype(BPType::ASSERT)) return BPType::ASSERT;
    if (type == show_bptype(BPType::PACK))   return BPType::PACK;
    if (type == show_bptype(BPType::UNPK))   return BPType::UNPK;
    internal::compiler_error(__FUNCTION__);
}

std::string BaseProcedure::to_string() const {
    return fmt::format("{}({})", show_bptype(name), params);
}

inline bool expect_params_count(Context& context, const BaseProcedure& proc, std::initializer_list<size_t> count) {
    if (std::ranges::find(count, proc.params.size()) == count.end()) {
        std::vector count_vector(count);
        context.messages.addErr(proc.place, "Expected {} parameters, found {}", fmt::join(count_vector, " or "), proc.params.size());
        return berror;
    }
    return bsuccess;
}

inline bool expect_params_count(Context& context, const BaseProcedure& proc, size_t count) {
    return expect_params_count(context, proc, {count});
}

inline bool exprect_var(Context& context, const BaseProcedure& proc, std::vector<std::pair<SymbolGroup, TypePtr>> types, size_t index) {
    if (types[index].first != SymbolGroup::VAR) {
        context.messages.addErr(proc.params[index]->place, "Expected variable");
        return berror;
    }
    return bsuccess;
}

template <class... Types>
requires (std::convertible_to<Types, bool> && ...)
bool many_and(Types... args) {
    return (true && ... && args);
}

template <class... Type>
inline bool expect_type(Context& context, const BaseProcedure& proc, std::vector<std::pair<SymbolGroup, TypePtr>> types, size_t index, std::string_view descr) {
    if (many_and(!types[index].second->is<Type>()...)) {
        context.messages.addErr(proc.params[index]->place, "Expected {}", descr);
        return berror;
    }
    return bsuccess;
}

inline bool expect_base_type(Context& context, const BaseProcedure& proc,  std::vector<std::pair<SymbolGroup, TypePtr>> types, size_t index, std::initializer_list<BaseType> base_types) {
    if (auto type = types[index].second->is<BuiltInType>(); type) {
        if (std::ranges::find(base_types, type->type) != base_types.end()) return true;
    }
    auto type_strings = std::views::transform(base_types, basetype_to_str);
    context.messages.addErr(proc.params[index]->place, "Expected {}", fmt::join(type_strings, " or "));
    return berror;
}

inline bool expect_base_type(Context& context, const BaseProcedure& proc,  std::vector<std::pair<SymbolGroup, TypePtr>> types, size_t index, BaseType base_type) {
    return expect_base_type(context, proc, types, index, {base_type});
}

Maybe<std::pair<SymbolGroup, TypePtr>> BaseProcedure::get_type(Context& context) const {
    std::vector<std::pair<SymbolGroup, TypePtr>> params_types;
    bool param_error = false;
    for (auto param : params) {
        auto param_type = param->get_type(context);
        if (!param_type) param_error = true;
        if (!param_error) params_types.push_back(param_type.value());
    }
    if (param_error) return error;
    const std::vector<std::tuple<BPType, std::vector<BaseType>, BaseType>> proc_templates = {
        {BPType::ODD, {BaseType::INTEGER}, BaseType::BOOL},
        {BPType::LSL, {BaseType::INTEGER, BaseType::INTEGER}, BaseType::INTEGER},
        {BPType::ASR, {BaseType::INTEGER, BaseType::INTEGER}, BaseType::INTEGER},
        {BPType::ROR, {BaseType::INTEGER, BaseType::INTEGER}, BaseType::INTEGER},
        {BPType::FLOOR, {BaseType::REAL}, BaseType::INTEGER},
        {BPType::FLT, {BaseType::INTEGER}, BaseType::REAL},
        {BPType::CHR, {BaseType::INTEGER}, BaseType::CHAR}
    };
    auto templ_it = std::ranges::find_if(proc_templates, [this](auto& templ){ return std::get<0>(templ) == name; });
    if (templ_it != proc_templates.end()) {
        auto templ_params = std::get<1>(*templ_it);
        if (!expect_params_count(context, *this, templ_params.size())) return error;
        for (size_t i = 0; i < templ_params.size(); ++i) {
            auto [group, type] = params_types[i];
            auto base_type = type->is<BuiltInType>();
            if (!base_type) {
                context.messages.addErr(params[i]->place, "Expected expression of built-in type");
                return error;
            }
            if (base_type->type != templ_params[i]) {
                context.messages.addErr(params[i]->place, "Expected expression of {} type", basetype_to_str(templ_params[i]));
                return error;
            }
        }
        return std::pair(SymbolGroup::CONST, make_base_type(std::get<2>(*templ_it)));
    }
    if (name == BPType::ABS) {
        if (!expect_params_count(context, *this, 1)) return error;
        if (!expect_base_type(context, *this, params_types, 0, {BaseType::INTEGER, BaseType::BYTE, BaseType::REAL})) return error;
        return std::pair(SymbolGroup::CONST, params_types[0].second);
    }
    if (name == BPType::LEN) {
        if (!expect_params_count(context, *this, 1)) return error;
        // if (!exprect_var(context, *this, params_types, 0)) return error; //Действительно ли это нужно?
        if (!expect_type<ArrayType, ConstStringType>(context, *this, params_types, 0, "array type")) return error;
        return std::pair(SymbolGroup::CONST, make_type<BuiltInType>(BaseType::INTEGER));
    }
    if (name == BPType::ORD) {
        if (!expect_params_count(context, *this, 1)) return error;
        if (!expect_base_type(context, *this, params_types, 0, {BaseType::CHAR, BaseType::BOOL, BaseType::SET})) return error;
        return std::pair(SymbolGroup::CONST, make_type<BuiltInType>(BaseType::INTEGER));
    }
    if (name == BPType::INC || name == BPType::DEC) {
        if (!expect_params_count(context, *this, {1,2})) return error;
        if (!exprect_var(context, *this, params_types, 0)) return error;
        if (!expect_base_type(context, *this, params_types, 0, BaseType::INTEGER)) return error;
        if (params.size() == 2 && !expect_base_type(context, *this, params_types, 1, BaseType::INTEGER)) return error;
        return std::pair(SymbolGroup::CONST, make_base_type(BaseType::VOID));
    }
    if (name == BPType::INCL || name ==BPType::EXCL) {
        if (!expect_params_count(context, *this, 2)) return error;
        if (!exprect_var(context, *this, params_types, 0)) return error;
        if (!expect_base_type(context, *this, params_types, 0, BaseType::SET)) return error;
        if (!expect_base_type(context, *this, params_types, 1, BaseType::INTEGER)) return error;
        return std::pair(SymbolGroup::CONST, make_base_type(BaseType::VOID));
    }
    if (name == BPType::NEW) {
        if (!expect_params_count(context, *this, 1)) return error;
        if (!exprect_var(context, *this, params_types, 0)) return error;
        if (!expect_type<PointerType>(context, *this, params_types, 0, "pointer type")) return error;
        return std::pair(SymbolGroup::CONST, make_base_type(BaseType::VOID));
    }
    if (name == BPType::ASSERT) {
        if (!expect_params_count(context, *this, 1)) return error;
        if (!expect_base_type(context, *this, params_types, 0, BaseType::BOOL)) return error;
        return std::pair(SymbolGroup::CONST, make_base_type(BaseType::VOID));
    }
    if (name == BPType::PACK || name == BPType::UNPK) {
        if (!expect_params_count(context, *this, 1)) return error;
        if (!exprect_var(context, *this, params_types, 0)) return error;
        if (!expect_base_type(context, *this, params_types, 0, BaseType::REAL)) return error;
        if (!expect_base_type(context, *this, params_types, 1, BaseType::INTEGER)) return error;
        return std::pair(SymbolGroup::CONST, make_base_type(BaseType::VOID));
    }
    internal::compiler_error(__FUNCTION__);
}

Maybe<ValuePtr> BaseProcedure::eval_constant(Context& context) const {
    std::vector<ValuePtr> params_values;
    bool param_error = false;
    for (auto param : params) {
        auto param_value = param->eval_constant(context);
        if (!param_value) param_error = true;
        if (!param_error) params_values.push_back(param_value.value());
    }
    if (param_error) return error;
    auto type = get_type(context);
    if (!type) return error;
    auto first_integer = params_values[0]->is<ConstInteger>();
    if (name == BPType::ABS) {
        if (first_integer) return make_value<ConstInteger>(std::abs(first_integer->value));
        auto first_real = params_values[0]->is<ConstReal>();
        if (first_real) return make_value<ConstReal>(std::abs(first_real->value));
    } else if (name == BPType::ODD && first_integer) {
        return make_value<Boolean>(first_integer->value % 2 == 1);
    } else if (name == BPType::LEN) {
        auto first_string = params_values[0]->is<String>();
        if (first_string) return make_value<ConstInteger>(first_string->value.size());
    } else if (name == BPType::FLOOR) {
        auto first_real = params_values[0]->is<ConstReal>();
        return make_value<ConstInteger>(std::floor(first_real->value));
    } else if (name == BPType::FLT) {
        return make_value<ConstReal>(first_integer->value);
    } else if (name == BPType::ORD) {
        auto first_char = params_values[0]->is<Char>();
        if (first_char) return make_value<ConstInteger>(first_char->value);
        auto first_bool = params_values[0]->is<Boolean>();
        if (first_bool) return make_value<ConstInteger>(first_bool->value);
        auto first_set = params_values[0]->is<ConstSet>();
        if (first_set) return make_value<ConstInteger>(first_set->values.to_ulong());
    } else if (name == BPType::CHR) {
        if (first_integer->value > 255 || first_integer->value < 0) {
            context.messages.addErr(place, "Expected integer >= 0 and < 256");
            return error;
        }
        return make_value<Char>(first_integer->value);
    }

    if (params_values.size() == 2) {
        auto second_integer = params_values[1]->is<ConstInteger>();
        if (name == BPType::LSL && first_integer && second_integer) {
            return make_value<ConstInteger>(first_integer->value * std::pow(2, second_integer->value));
        } else if (name == BPType::ASR && first_integer && second_integer) {
            return make_value<ConstInteger>(first_integer->value / std::pow(2, second_integer->value));
        } else if (name == BPType::ROR && first_integer && second_integer) {
            auto rotr32 = [](uint n, int c) -> uint32_t {
                const uint mask = (8*sizeof(n) - 1);
                c &= mask;
                return (n>>c) | (n<<( (-c)&mask ));
            };
            uint abs = std::abs(first_integer->value);
            int sign = first_integer->value / abs;
            return make_value<ConstInteger>(sign*rotr32(abs, second_integer->value));
        }
    }

    context.messages.addErr(place, "This built-in function is incalculable in a constant expression");
    return error;
    internal::compiler_error(__FUNCTION__);
}

Maybe<ValuePtr> BaseProcedure::apply_operator(Context& context, OpType oper, const Value& other) const {
    auto this_value = eval_constant(context);
    if (!this_value) return error;
    return this_value.value()->apply_operator(context, oper, other);
}

BaseProcedure::BaseProcedure(std::string_view n, ExpList p) : name(read_bptype(n)), params(p) {}

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
    ConstSet::SetType set;
    for (auto elem : value) {
        auto first_int = check_value(context, *elem.first);
        if (!first_int) return error;
        if (first_int->value < 0 || first_int->value > 63) {
            context.messages.addErr(place, "Expected x >= 0 && x < 64, found: {}", first_int->value);
            return error;
        }
        if (elem.second) {
            auto second_int = check_value(context, *elem.second.value());
            if (second_int->value < 0 || second_int->value > 63) {
                context.messages.addErr(place, "Expected x >= 0 && x < 64, found: {}", first_int->value);
                return error;
            }
            if (!second_int) return error;
            for (Integer i = first_int->value; i <= second_int->value; ++i) {
                set.set(i);
            }
        } else {
            set.set(first_int->value);
        }
    }
    return make_value<ConstSet>(set);
}

Set::Set(std::optional<std::vector<SetElement>> v) {
    if (v)
        value = *v;
}

inline Maybe<const RecordType*> get_record_from_pointer(const Type* some_type, Context& context, CodePlace place) {
    const Type* start_type = some_type;
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
    if (auto scalar_type = some_type->is<ScalarType>(); scalar_type) {
        some_type = &scalar_type->get_type();
    }
    auto type = some_type->is<RecordType>();
    if (!type) {
        context.messages.addErr(place, "Expected record, pointer or scalar type, found {}", start_type->to_string());
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
    if (ident.selector.size() > 0 && !value.params && !value.commonParams) {
        auto back_selecter = ident.selector.back();
        if (std::holds_alternative<QualIdent>(back_selecter)) {
            auto desigRep = DesignatorRepairer(std::get<QualIdent>(back_selecter), std::vector<Selector>{});
            if (!desigRep.repair(context))
                return berror;
            auto desig = desigRep.get();
            auto symbol = context.symbols.get_symbol(context.messages, desig.ident);
            if (desig.selector.size() == 0 && symbol->group == SymbolGroup::TYPE) {
                return bsuccess;
            } else {
                ident.selector.pop_back();
                auto expr = make_expression<ProcCall>(std::nullopt, desig, std::nullopt);
                expr->place = desig.ident.ident.place;
                value.params = {expr};
            }
        }
    }
    return bsuccess;
}

std::string ProcCall::to_string() const {
    auto commonParams = data.unsafe_get().commonParams;
    auto params = data.unsafe_get().params;
    auto ident = data.unsafe_get().ident.unsafe_get();
    if (commonParams) {
        if (params) {
            return fmt::format("{{}}.{}({})", *commonParams, ident.to_string(), fmt::join(*params, ", "));
        } else {
            return fmt::format("{{}}.{}", *commonParams, ident.to_string());
        }
    } else {
        if (params) {
            return fmt::format("{}({})", ident.to_string(), fmt::join(*params, ", "));
        } else {
            return fmt::format("{}", ident.to_string());
        }
    }
}

Maybe<std::pair<SymbolGroup, TypePtr>> ProcCall::get_type(Context& context) const {
    if (!data.repair(context)) return error;
    auto ident = data.get().ident.get();
    auto params = data.get().params;
    auto commonParams = data.get().commonParams;
    auto symbolRes = ident.get_symbol(context, place);
    if (!symbolRes)
        return error;
    auto symbol = *symbolRes;
    if (!params && !commonParams) return std::pair(symbol.group, symbol.type);

    auto funcType = dynamic_cast<ProcedureType*>(symbol.type.get());
    if (!funcType) {
        context.messages.addErr(place, "Expected procedure type, found {}", symbol.type.get()->to_string());
        return error;
    }
    if (params && params->size() != funcType->params.formal.size()) {
        context.messages.addErr(place, "Expected {} formal parameters, found {}", funcType->params.formal.size(), params->size());
        return error;
    }
    if (commonParams && commonParams->size() != funcType->params.common.size()) {
        context.messages.addErr(place, "Expected {} common parameters, found {}", funcType->params.common.size(), commonParams->size());
        return error;
    }
    auto compatible_types = true;
    if (params) {
        for (size_t i = 0; i < funcType->params.formal.size(); ++i) {
            auto var = funcType->params.formal[i];
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
    }
    if (commonParams) {
        for (size_t i = 0; i < funcType->params.common.size(); ++i) {
            auto var = funcType->params.common[i];
            auto value = (*commonParams)[i];
            auto valueSymbol = context.symbols.get_symbol(context.messages, value);
            if (!valueSymbol) return error;
            auto valueType = valueSymbol.value().type;
            if (var.var && valueSymbol->group != SymbolGroup::VAR) {
                context.messages.addErr(value.ident.place, "Expected variable");
                compatible_types = false;
            }
            if (!var.type->assignment_compatible(context, *valueType) && !ArrayType::compatible(context, *valueType, *var.type)) {
                context.messages.addErr(place, "Can't match actual and common parameter: {} and {}", valueType->to_string(), var.type->to_string());
                compatible_types = false;
            }
        }
    }
    if (!compatible_types) {
        return error;
    }
    TypePtr return_type = make_base_type(BaseType::VOID);
    if (funcType->params.rettype) return_type = *funcType->params.rettype;
    return std::pair(SymbolGroup::CONST, return_type);
}

Maybe<ValuePtr> ProcCall::eval_constant(Context& context) const {
    if (!data.repair(context))
        return error;
    auto ident = data.get().ident.get();
    if (!data.get().commonParams && !data.get().params && ident.selector.size() == 0) {
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

Maybe<std::pair<SymbolGroup, TypePtr>> Term::get_type(Context& context) const {
    if (!oper && !second) {
        auto res = first->get_type(context);
        if (!res) return error;
        auto [group, type] = res.value();
        if (oper && !type->is<ConstInteger>() && !type->is<ConstReal>()) {
            context.messages.addErr(place, "Expected numeric value for sign");
            return error;
        }
        return res;
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
        if (sign) {
            int int_sign = sign == '-' ? -1 : 1;
            if (auto res_integer = firstRes.value()->is<ConstInteger>(); res_integer) {
                res_integer->value *= int_sign;
            } else if (auto res_real = firstRes.value()->is<ConstReal>(); res_real) {
                res_real->value *= int_sign;
            } else {
                context.messages.addErr(place, "Expected numeric value for sign");
                return error;
            }
        }
        if (!second) return firstRes;
        if (!oper) internal::compiler_error(__FUNCTION__);
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
