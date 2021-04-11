#include "internal_error.hpp"
#include "node_formatters.hpp"
#include "nodes.hpp"
#include "parser_tools.hpp"
#include "semantic_context.hpp"
#include "type_nodes.hpp"
#include "symbol_table.hpp"

#include <ranges>
#include <algorithm>


using namespace nodes;

const char* nodes::basetype_to_str(BaseType type) {
    switch (type) {
        case BaseType::BOOL: return "BOOL";
        case BaseType::CHAR: return "CHAR";
        case BaseType::INTEGER: return "INTEGER";
        case BaseType::REAL: return "REAL";
        case BaseType::BYTE: return "BYTE";
        case BaseType::SET: return "SET";
        case BaseType::NIL: return "NIL";
        case BaseType::VOID: return "VOID";
        default: internal::compiler_error("Unexpected BaseType");
    }
}

BaseType ident_to_basetype(std::string_view i) {
    if (i == "BOOLEAN")
        return BaseType::BOOL;
    if (i == "CHAR")
        return BaseType::CHAR;
    if (i == "INTEGER")
        return BaseType::INTEGER;
    if (i == "REAL")
        return BaseType::REAL;
    if (i == "BYTE")
        return BaseType::BYTE;
    if (i == "SET")
        return BaseType::SET;
    if (i == "NIL")
        return BaseType::NIL;
    internal::compiler_error(fmt::format("Unexpected BaseType ident: '{}'", i));
}

bool BuiltInType::equal_to(BaseType other) const {
    return type == other;
}

BuiltInType::BuiltInType(std::string_view i) : type(ident_to_basetype(i)) {}

BuiltInType::BuiltInType(BaseType t) : type(t) {}

std::string BuiltInType::to_string() const {
    return fmt::format("@{}", basetype_to_str(type));
}

Maybe<TypePtr> BuiltInType::normalize(Context&, bool) const {
    return make_type<BuiltInType>(*this);
}

bool BuiltInType::same(Context& context, const Type& other) const {
    if (auto type_name = other.is<TypeName>(); type_name) {
        auto other_type = type_name->dereference(context);
        if (!other_type) return berror;
        same(context, **other_type);
    } else if (auto base_type = other.is<BuiltInType>(); base_type) {
        return type == base_type->type;
    }
    return false;
}

bool BuiltInType::equal(Context& context, const Type& other) const {
    return same(context, other);
}

bool BuiltInType::assignment_compatible(Context& context, const Type& expr) const {
    if (same(context, expr)) return true;
    if (auto expr_string = expr.is<ConstStringType>(); expr_string && expr_string->size == 1)
        return true;
    if (auto expr_base = expr.is<BuiltInType>(); expr_base) {
        return (type == BaseType::INTEGER || type == BaseType::BYTE)
            && (expr_base->type == BaseType::INTEGER || expr_base->type == BaseType::BYTE);
    }
    return false;
}

std::string TypeName::to_string() const {
    return ident.to_string();
}

Maybe<TypePtr> TypeName::dereference(Context& context) const {
    auto symbol = context.symbols.get_symbol(context.messages, ident);
    if (!symbol)
        return error;
    if (auto typeName = symbol->type->is<TypeName>())
        return typeName->dereference(context);
    else
        return symbol->type;
}

Maybe<TypePtr> TypeName::normalize(Context& context, bool) const {
    auto symbol = context.symbols.get_symbol(context.messages, ident);
    if (!symbol)
        return error;
    return symbol->type;
}

bool TypeName::same(Context& context, const Type& other) const {
    auto this_type = dereference(context);
    if (!this_type) return berror;
    const Type* other_type = &other;
    if (auto other_name = other.is<TypeName>(); other_name) {
        if (ident == other_name->ident) return true;
        auto maybe_other_type = other_name->dereference(context);
        if (!maybe_other_type) return berror;
        other_type = maybe_other_type.value().get();
    }
    return this_type.value()->same(context, *other_type);
}

bool TypeName::equal(Context& context, const Type& other) const {
    return same(context, other);
}

bool TypeName::assignment_compatible(Context& context, const Type& expr) const {
    auto this_type = dereference(context);
    if (!this_type) return berror;
    const Type* expr_type = &expr;
    if (auto expr_name = expr.is<TypeName>(); expr_name) {
        auto maybe_type = expr_name->dereference(context);
        if (!maybe_type) return berror;
        expr_type = maybe_type.value().get();
    }
    return this_type.value()->same(context, *expr_type);
}

std::string ImportTypeName::to_string() const {
    return fmt::format("${}", ident);
}

Maybe<TypePtr> ImportTypeName::normalize(Context&, bool) const {
    return make_type<ImportTypeName>(*this);
}

bool ImportTypeName::same(Context& context, const Type& other) const {
    if (auto import_name = other.is<ImportTypeName>(); import_name) {
        return ident == import_name->ident;
    } else {
        return other.same(context, *this);
    }
}

bool ImportTypeName::equal(Context& context, const Type& other) const {
    return same(context, other);
}

bool ImportTypeName::assignment_compatible(Context& context, const Type& expr) const {
    return equal(context, expr);
}

std::string RecordType::to_string() const {
    if (basetype)
        return fmt::format("RECORD ({}) {} END", *basetype, seq);
    else
        return fmt::format("RECORD {} END", seq);
}

Maybe<TypePtr> RecordType::has_field(const Ident& ident, Context& context) const {
    for (auto& list : seq) {
        auto res = std::find_if(list.list.begin(), list.list.end(), [&ident](auto i) { return i.ident == ident; });
        if (res != list.list.end()) {
            return TypePtr(list.type);
        }
    }
    if (basetype) {
        auto base = context.symbols.get_symbol(context.messages, *basetype);
        if (!base)
            return error;
        else {
            auto baseptr = dynamic_cast<RecordType*>(base->type.get());
            if (!baseptr) {
                context.messages.addErr(basetype->ident.place, "Internal compiler error in RecordType::has_field");
                return error;
            }
            return baseptr->has_field(ident, context);
        }
    }
    context.messages.addErr(place, "Field {} not found in {}", ident, this->to_string());
    return error;
}

bool RecordType::extends(Context& context, const Type& type) const {
    auto typeRes = type.is<RecordType>();
    if (!typeRes) return false;
    if (this->same(context, *typeRes)) return true;
    if (!basetype) return false;
    auto res = context.symbols.get_symbol(context.messages, *basetype);
    if (!res) internal::compiler_error("Basetype symbol not found");
    if (res->type->same(context, *typeRes)) return true;
    else {
        auto record = res->type->is<RecordType>();
        if (!record) internal::compiler_error("Basetype is not record type");
        return record->extends(context, type);
    }
}

Maybe<TypePtr> RecordType::normalize(Context& context, bool normalize_pointers) const {
    RecordType copy = *this;
    for (auto& list : copy.seq) {
        auto res = list.type->normalize(context, normalize_pointers);
        if (!res)
            return error;
        list.type = *res;
    }
    return make_type<RecordType>(copy);
}

bool RecordType::same(Context& context, const Type& other) const {
    if (other.is<TypeName>()) return other.same(context, *this);
    return this == &other;
}

bool RecordType::equal(Context& context, const Type& other) const {
    return same(context, other);
}

bool RecordType::assignment_compatible(Context& context, const Type& expr) const {
    if (auto expr_record = expr.is<RecordType>(); expr_record) {
        return expr_record->extends(context, *this);
    }
    return same(context, expr);
}

std::string PointerType::to_string() const {
    return fmt::format("POINTER TO {}", type);
}

bool PointerType::check_type(Context& context) {
    auto res = type->normalize(context, false);
    if (!res)
        return berror;
    return bsuccess;
}

const RecordType& PointerType::get_type(Context& context) const {
    Type* tmp_type;
    if (auto name = type->is<TypeName>()) {
        auto res = context.symbols.get_symbol(context.messages, name->ident);
        if (!res) internal::compiler_error("Symbol for typename in pointer not found");
        tmp_type = res->type.get();
    } else {
        tmp_type = type.get();
    }
    auto record = tmp_type->is<RecordType>();
    if (!record) internal::compiler_error("Pointer type is not record type");
    return *record;
}

Maybe<TypePtr> PointerType::normalize(Context& context, bool normalize_pointers) const {
    if (!normalize_pointers)
        return make_type<PointerType>(*this);
    PointerType copy = *this;
    auto res = copy.type->normalize(context, false);
    if (!res) return res;
    return make_type<PointerType>(copy);
}

bool PointerType::same(Context& context, const Type& other) const {
    if (other.is<TypeName>()) return other.same(context, *this);
    return this == &other;
}

bool PointerType::equal(Context& context, const Type& other) const {
    return same(context, other);
}

bool PointerType::assignment_compatible(Context& context, const Type& expr) const {
    if (auto expr_pointer = expr.is<PointerType>(); expr_pointer) {
        auto& expr_type = expr_pointer->get_type(context);
        return expr_type.extends(context, get_type(context));
    } else if (auto expr_base = expr.is<BuiltInType>(); expr_base && expr_base->type == BaseType::NIL) {
        return true;
    }
    return same(context, expr);
}

std::string ConstStringType::to_string() const { return fmt::format("String[{}]", size); }

Maybe<TypePtr> ConstStringType::normalize(Context&, bool) const {
    return make_type<ConstStringType>(*this);
}

bool ConstStringType::same(Context& context, const Type& other) const {
    if (auto string = other.is<ConstStringType>(); string) {
        return size == string->size;
    } else {
        return other.same(context, *this);
    }
}

bool ConstStringType::equal(Context& context, const Type& other) const {
    return same(context, other);
}

bool ConstStringType::assignment_compatible(Context&, const Type&) const {
    return false;
}

std::string ArrayType::to_string() const {
    if (!open_array)
        return fmt::format("ARRAY {} OF ({})", length, type);
    else {
        return fmt::format("ARRAY OF ({})", type);
    }
}

Maybe<TypePtr> ArrayType::normalize(Context& context, bool normalize_pointers) const {
    ArrayType copy = *this;
    auto res = copy.type->normalize(context, normalize_pointers);
    if (!res)
        return res;
    copy.type = *res;
    if (!open_array) {
        auto expr = copy.length.get(context);
        if (!expr)
            return error;
        auto integer = dynamic_cast<IntegerValue*>(expr->get());
        if (!integer) {
            context.messages.addErr(expr.value()->place, "Expected integer");
            return error;
        }
    }
    return make_type<ArrayType>(copy);
}

bool ArrayType::same(Context& context, const Type& other) const {
    if (other.is<TypeName>()) return other.same(context, *this);
    return this == &other;
}

bool ArrayType::equal(Context& context, const Type& other) const {
    if (auto other_array = other.is<ArrayType>(); other_array && open_array && other_array->open_array) {
        return type->equal(context, *other_array->type);
    } else if (auto other_name = other.is<TypeName>(); other_name) {
        auto other_type = other_name->dereference(context);
        if (!other_type) return berror;
        return equal(context, *other_type.value());
    }
    return false;
}

bool ArrayType::assignment_compatible(Context& context, const Type& expr) const {
    if (auto expr_string = expr.is<ConstStringType>(); expr_string) {
        auto array_type = type->is<BuiltInType>();
        auto maybe_size = length.get(context);
        if (!maybe_size) return berror;
        auto size = maybe_size.value()->is<IntegerValue>();
        return !open_array && array_type && array_type->type == BaseType::CHAR
            && size && (size->value > static_cast<int>(expr_string->size));
    } else if (auto expr_array = expr.is<ArrayType>(); expr_array && expr_array->open_array) {
        return type->equal(context, *expr_array->type);
    }
    return same(context, expr);
}

Maybe<TypePtr> ArrayType::drop_dimensions(size_t count, Context& context) const {
    if (count == 0) return make_type<ArrayType>(*this);
    else if (auto array = type->is<ArrayType>(); !array) {
        if (count == 1) return type;
        else {
            context.messages.addErr(place, "Expected array, found {}", type);
            return error;
        }
    } else {
        return array->drop_dimensions(count - 1, context);
    }
}

bool ArrayType::compatible(Context& context, const Type& actual, const Type& formal) {
    //Tf and Ta are the same type;
    if (actual.same(context, formal)) return true;

    auto farray = formal.is<ArrayType>();
    if (!farray) return false;
    //Tf is an open array, Ta is any array, and their element types are array compatible;
    if (auto aarray = actual.is<ArrayType>(); aarray) {
        return farray->open_array && compatible(context, *aarray->type, *farray->type);
    }
    //Tf is ARRAY OF CHAR and a is a string.
    if (auto astring = actual.is<ConstStringType>(); astring) {
        auto base = farray->type->is<BuiltInType>();
        return base->equal_to(BaseType::CHAR); //Size?
    }
    return false;
}

ArrayType::ArrayType(std::vector<ExpressionPtr> l, TypePtr t, bool u) : open_array(u) {
    length = l.front();
    if (l.size() > 1) {
        type = make_type<ArrayType>(std::vector(l.begin() + 1, l.end()), t, u);
    } else {
        type = t;
    }
}

bool match_params_lists(Context& context, const std::vector<FormalParameter>& first, const std::vector<FormalParameter>& second) {
    if (first.size() != second.size()) return false;
    if (first.size() == 0 && second.size() == 0) return true;
    for (size_t i = 0; i < first.size(); ++i) {
        if (first[i].var != second[i].var) return false;
        if (!first[i].type->equal(context, *second[i].type)) return false;
    }
    return true;
}

//! Matching formal parameters lists http://miasap.se/obnc/type-compatibility.html
bool FormalParameters::match(Context& context, const FormalParameters& other, bool match_first, bool match_second) const {
    bool res = true;
    if (!rettype != !other.rettype) return false;
    if (rettype && !rettype.value()->same(context, **other.rettype)) return false;
    if (match_first) {
        res = res && match_params_lists(context, common, other.common);
    }
    if (match_second) {
        res = res && match_params_lists(context, formal, other.formal);
    }
    return res;
}

std::string ProcedureType::to_string() const {
    return fmt::format("PROCEDURE {}", params);
}

Maybe<TypePtr> ProcedureType::normalize(Context& context, bool normalize_pointers) const {
    ProcedureType type = *this;
    if (type.params.rettype) {
        auto new_ret_type = type.params.rettype.value()->normalize(context, normalize_pointers);
        if (!new_ret_type) return error;
        type.params.rettype = new_ret_type;
    }
    for (auto& section : type.params.common) {
        if (auto type = section.type->normalize(context, normalize_pointers); type) {
            section.type = type.value();
            // auto group = section.var ? SymbolGroup::VAR : SymbolGroup::CONST;
            // auto res = table.get_symbols().add_symbol(messages, nodes::IdentDef{section.ident, false}, group, type.value());
            // if (!res) return error;
        } else {
            return error;
        }
    }
    for (auto& section : type.params.formal) {
        if (auto type = section.type->normalize(context, normalize_pointers); type) {
            section.type = type.value();
            // auto group = section.var ? SymbolGroup::VAR : SymbolGroup::CONST;
            // auto res = table.get_symbols().add_symbol(messages, nodes::IdentDef{section.ident, false}, group, type.value());
            // if (!res) return error;
        } else {
            return error;
        }
    }
    return make_type<ProcedureType>(type);
}

bool ProcedureType::same(Context& context, const Type& other) const {
    if (other.is<TypeName>()) return other.same(context, *this);
    return this == &other;
}

bool ProcedureType::equal(Context& context, const Type& other) const {
    if (auto other_procedure = other.is<ProcedureType>(); other_procedure) {
        return params.match(context, other_procedure->params, true, true);
    } else if (auto other_name = other.is<TypeName>(); other_name) {
        auto other_type = other_name->dereference(context);
        if (!other_type) return berror;
        return equal(context, *other_type.value());
    }
    return false;
}

bool ProcedureType::assignment_compatible(Context& context, const Type& expr) const {
    if (auto expr_procedure = expr.is<ProcedureType>(); expr_procedure) {
        return params.match(context, expr_procedure->params, true, true);
    } else if (auto expr_base = expr.is<BuiltInType>(); expr_base && expr_base->type == BaseType::NIL) {
        return true;
    }
    return same(context, expr);
}

ProcedureType::ProcedureType(std::optional<FormalParameters> par) {
    if (par)
        params = *par;
}

std::string CommonType::to_string() const {
    return fmt::format("CASE {} OF {}{} END",
                       common_feature_type ? common_feature_type->to_string() : "", fmt::join(pair_list, " | "),
                       else_clause ? fmt::format(" ELSE {}", else_clause.value()->to_string()) : "");
}

Maybe<TypePtr> CommonType::normalize(Context& context, bool normalize_pointers) const {
    CommonType copy;
    copy.common_feature_type = common_feature_type;
    //Проверка на то что все варианты имеют один тип метки
    if (pair_list.size() > 0) {
        auto featureIndex = pair_list.front().feature.value.index();
        auto check = [featureIndex] (auto p) {
            return p.feature.value.index() == featureIndex;
        };
        if (!std::ranges::all_of(pair_list, check)) {
            context.messages.addErr(place, "Common features have different types");
            return error;
        }
    }
    for (auto pair : pair_list) {
        if (copy.has_case(pair.feature)) {
            context.messages.addErr(place, "Multiple common features with same name: {}", pair.feature);
            return error;
        }
        auto type = pair.type->normalize(context, normalize_pointers);
        if (!type) return error;
        if (!type.value()->is<RecordType>() && !type.value()->is<CommonType>()) {
            context.messages.addErr(pair.type->place, "Expected Record or Common type");
            return error;
        }
        copy.pair_list.emplace_back(pair.feature, type.value());
    }
    if (else_clause) {
        auto type = else_clause.value()->normalize(context, normalize_pointers);
        if (!type) return error;
        copy.else_clause = type.value();
    }
    return make_type<CommonType>(copy);
}

bool CommonType::same(Context& context, const Type& other) const {
    if (other.is<TypeName>()) return other.same(context, *this);
    return this == &other;
}

bool CommonType::equal(Context& context, const Type& other) const {
    if (auto scalar_other = other.is<ScalarType>(); scalar_other) {
        return equal(context, *scalar_other->type);
    }
    return same(context, other);
}

//! \todo Не совсем понятно что тут должно быть
bool CommonType::assignment_compatible(Context& context, const Type& expr) const {
    if (same(context, expr)) return true;
    if (auto scalar_expr = expr.is<ScalarType>(); scalar_expr) {
        return same(context, *scalar_expr->type);
    } else {
        return std::ranges::any_of(pair_list, [&expr, &context](auto p){ return p.type->assignment_compatible(context, expr); });
    }
}

bool CommonType::has_case(CommonFeature feature) const {
    return std::ranges::find_if(pair_list, [feature](auto p){return p.feature == feature;}) != pair_list.end();
}

const Type& CommonType::get_case(CommonFeature feature) const {
    auto _case = std::ranges::find_if(pair_list, [feature](auto p){return p.feature == feature;});
    if (_case == pair_list.end()) internal::compiler_error(__FUNCTION__);
    return *_case->type;
}

std::string ScalarType::to_string() const {
    return fmt::format("({})<{}>", type->to_string(), feature);
}

Maybe<TypePtr> ScalarType::normalize(Context& context, bool normalize_pointers) const {
    ScalarType copy;
    copy.feature = feature;
    auto new_type = type->normalize(context, normalize_pointers);
    if (!new_type) return error;
    if (auto common_type = new_type.value()->is<CommonType>(); !common_type) {
        context.messages.addErr(type->place, "Expected CommonType, found: {}", new_type.value()->to_string());
        return error;
    } else if (!common_type->has_case(feature)) {
        context.messages.addErr(place, "Feature with name {}, not found in {}", feature, type->to_string());
        return error;
    }
    copy.type = new_type.value();
    return make_type<ScalarType>(copy);
}

bool ScalarType::same(Context& context, const Type& other) const {
    if (other.is<TypeName>()) return other.same(context, *this);
    return this == &other;
}

bool ScalarType::equal(Context& context, const Type& other) const {
    if (auto scalar_other = other.is<ScalarType>(); scalar_other) {
        return type->same(context, *scalar_other->type) && feature == scalar_other->feature;
    }
    return same(context, other);
}

bool ScalarType::assignment_compatible(Context& context, const Type& expr) const {
    if (auto common_this = type->is<CommonType>(); common_this) {
        return common_this->get_case(feature).assignment_compatible(context, expr);
    } else {
        internal::compiler_error(__FUNCTION__);
    }
}
