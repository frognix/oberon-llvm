#include "node_formatters.hpp"
#include "nodes.hpp"
#include "semantic_context.hpp"
#include "symbol_table.hpp"

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
        default: throw std::runtime_error("Internal error: Bad BaseType");
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
    throw std::runtime_error(fmt::format("Internal error: Bad BaseType ({})", i));
}

bool BuiltInType::equal_to(BaseType other) {
    return type == other;
}

BuiltInType::BuiltInType(Ident i) : type(ident_to_basetype(i)) {}

BuiltInType::BuiltInType(BaseType t) : type(t) {}

std::string BuiltInType::to_string() const {
    return fmt::format("@{}", basetype_to_str(type));
}

bool BuiltInType::is_equal(const Type& other) const {
    return type == static_cast<const BuiltInType&>(other).type;
}

Maybe<TypePtr> BuiltInType::normalize(Context&, bool) {
    return make_type<BuiltInType>(*this);
}

std::string TypeName::to_string() const {
    return ident.to_string();
}

bool TypeName::is_equal(const Type& other) const {
    return ident == static_cast<const TypeName&>(other).ident;
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

Maybe<TypePtr> TypeName::normalize(Context& context, bool normalize_pointers) {
    auto symbol = context.symbols.get_symbol(context.messages, ident);
    if (!symbol)
        return error;
    return symbol->type->normalize(context, normalize_pointers);
}

std::string RecordType::to_string() const {
    if (basetype)
        return fmt::format("RECORD ({}) {} END", *basetype, seq);
    else
        return fmt::format("RECORD {} END", seq);
}

bool RecordType::is_equal(const Type& _other) const {
    auto other = static_cast<const RecordType&>(_other);
    return basetype == other.basetype && seq == other.seq;
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

Maybe<TypePtr> RecordType::normalize(Context& context, bool normalize_pointers) {
    RecordType copy = *this;
    for (auto& list : copy.seq) {
        auto res = list.type->normalize(context, normalize_pointers);
        if (!res)
            return error;
        list.type = *res;
    }
    return make_type<RecordType>(copy);
}

std::string PointerType::to_string() const {
    return fmt::format("POINTER TO {}", type);
}

bool PointerType::is_equal(const Type& other) const {
    return type == static_cast<const PointerType&>(other).type;
}

bool PointerType::check_type(Context& context) {
    auto res = type->normalize(context, false);
    if (!res)
        return berror;
    return bsuccess;
}

Maybe<TypePtr> PointerType::normalize(Context& context, bool normalize_pointers) {
    if (!normalize_pointers)
        return make_type<PointerType>(*this);
    PointerType copy = *this;
    auto res = copy.type->normalize(context, normalize_pointers);
    if (!res)
        return res;
    return make_type<PointerType>(copy);
}

std::string ArrayType::to_string() const {
    if (!unsized)
        return fmt::format("ARRAY {} OF {}", fmt::join(lengths, ", "), type);
    else {
        std::string tmp;
        for (size_t i = 0; i < lengths.size(); i++)
            tmp += "ARRAY OF ";
        return fmt::format("{} {}", tmp, type);
    }
}

//! \todo Нормальное сравнение массивов
bool ArrayType::is_equal(const Type& _other) const {
    auto other = static_cast<const ArrayType&>(_other);
    // if (unsized && other.unsized)
    return *type == *other.type && lengths.size() == other.lengths.size();
}

Maybe<TypePtr> ArrayType::normalize(Context& context, bool normalize_pointers) {
    ArrayType copy = *this;
    auto res = copy.type->normalize(context, normalize_pointers);
    if (!res)
        return res;
    for (auto& length : copy.lengths) {
        auto expr = length->eval(context);
        if (!expr)
            return error;
        auto integer = dynamic_cast<Number*>(expr->get());
        if (!integer) {
            context.messages.addErr(length->place, "Expected number, found {}", length->to_string());
            return error;
        }
        if (!std::holds_alternative<Integer>(integer->value)) {
            context.messages.addErr(length->place, "Expected integer, found real");
            return error;
        }
        length = *expr;
    }
    return make_type<ArrayType>(copy);
}

Maybe<TypePtr> ArrayType::drop_dimensions(size_t count, Context& context) const {
    if (count > lengths.size()) {
        context.messages.addErr(place, "Array of type {} has only {} dimensions", this->to_string(), lengths.size());
        return error;
    }
    if (count == lengths.size())
        return TypePtr(type);
    return make_type<ArrayType>(std::vector(lengths.begin() + count, lengths.end()), type);
}

std::string ProcedureType::to_string() const {
    return fmt::format("PROCEDURE {}", params);
}

bool ProcedureType::is_equal(const Type& other) const {
    return params == static_cast<const ProcedureType&>(other).params;
}

Maybe<TypePtr> ProcedureType::normalize(Context& context, bool normalize_pointers) {
    ProcedureType copy = *this;
    for (auto& section : copy.params.sections) {
        auto res = section.type->normalize(context, normalize_pointers);
        if (!res)
            return error;
        section.type = *res;
    }
    return make_type<ProcedureType>(copy);
}

ProcedureType::ProcedureType(std::optional<FormalParameters> par) {
    if (par)
        params = *par;
}
