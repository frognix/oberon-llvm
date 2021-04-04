#include "internal_error.hpp"
#include "node_formatters.hpp"
#include "nodes.hpp"
#include "parser_tools.hpp"
#include "semantic_context.hpp"
#include "symbol_table.hpp"
#include "type_nodes.hpp"

using namespace nodes;

const char* basetype_to_str(BaseType type) {
    switch (type) {
        case BaseType::BOOL: return "BOOL";
        case BaseType::CHAR: return "CHAR";
        case BaseType::INTEGER: return "INTEGER";
        case BaseType::REAL: return "REAL";
        case BaseType::BYTE: return "BYTE";
        case BaseType::SET: return "SET";
        case BaseType::NIL: return "NIL";
        default: internal::compiler_error("Unexpected BaseType");
    }
}

BaseType ident_to_basetype(Ident i) {
    if (i.equal_to("BOOLEAN"))
        return BaseType::BOOL;
    if (i.equal_to("CHAR"))
        return BaseType::CHAR;
    if (i.equal_to("INTEGER"))
        return BaseType::INTEGER;
    if (i.equal_to("REAL"))
        return BaseType::REAL;
    if (i.equal_to("BYTE"))
        return BaseType::BYTE;
    if (i.equal_to("SET"))
        return BaseType::SET;
    if (i.equal_to("NIL"))
        return BaseType::NIL;
    internal::compiler_error(fmt::format("Unexpected BaseType ident: '{}'", i));
}

bool BuiltInType::equal_to(BaseType other) const {
    return type == other;
}

BuiltInType::BuiltInType(Ident i) : type(ident_to_basetype(i)) {}

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

bool BuiltInType::assignment_compatible(Context& context, const Type& expr) {
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

bool TypeName::assignment_compatible(Context& context, const Type& expr) {
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

bool ImportTypeName::assignment_compatible(Context& context, const Type& expr) {
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

bool RecordType::assignment_compatible(Context& context, const Type& expr) {
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

bool PointerType::assignment_compatible(Context& context, const Type& expr) {
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

bool ConstStringType::assignment_compatible(Context&, const Type&) {
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
        auto integer = dynamic_cast<ConstInteger*>(expr->get());
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

bool ArrayType::assignment_compatible(Context& context, const Type& expr) {
    if (auto expr_string = expr.is<ConstStringType>(); expr_string) {
        auto array_type = type->is<BuiltInType>();
        auto maybe_size = length.get(context);
        if (!maybe_size) return berror;
        auto size = maybe_size.value()->is<ConstInteger>();
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

//! Matching formal parameters lists http://miasap.se/obnc/type-compatibility.html
bool FormalParameters::match(Context& context, const FormalParameters& other) const {
    if (params.size() != other.params.size()) return false;
    if (!rettype == !other.rettype) return false;
    if (rettype && !rettype.value()->same(context, **other.rettype)) return false;
    for (size_t i = 0; i < params.size(); ++i) {
        if (params[i].var != other.params[i].var) return false;
        if (!params[i].type->equal(context, *other.params[i].type)) return false;
    }
    return true;
}

std::string ProcedureType::to_string() const {
    return fmt::format("PROCEDURE {}", params);
}

Maybe<TypePtr> ProcedureType::normalize(Context& context, bool normalize_pointers) const {
    ProcedureType copy = *this;
    for (auto& section : copy.params.params) {
        auto res = section.type->normalize(context, normalize_pointers);
        if (!res)
            return error;
        section.type = *res;
    }
    return make_type<ProcedureType>(copy);
}

bool ProcedureType::same(Context& context, const Type& other) const {
    if (other.is<TypeName>()) return other.same(context, *this);
    return this == &other;
}

bool ProcedureType::equal(Context& context, const Type& other) const {
    if (auto other_procedure = other.is<ProcedureType>(); other_procedure) {
        return params.match(context, other_procedure->params);
    } else if (auto other_name = other.is<TypeName>(); other_name) {
        auto other_type = other_name->dereference(context);
        if (!other_type) return berror;
        return equal(context, *other_type.value());
    }
    return false;
}

bool ProcedureType::assignment_compatible(Context& context, const Type& expr) {
    if (auto expr_procedure = expr.is<ProcedureType>(); expr_procedure) {
        return params.match(context, expr_procedure->params);
    } else if (auto expr_base = expr.is<BuiltInType>(); expr_base && expr_base->type == BaseType::NIL) {
        return true;
    }
    return same(context, expr);
}

ProcedureType::ProcedureType(std::optional<FormalParameters> par) {
    if (par)
        params = *par;
}
