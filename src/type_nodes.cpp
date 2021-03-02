#include "nodes.hpp"
#include "symbol_table.hpp"
#include "node_formatters.hpp"

using namespace nodes;

inline Error check_type(const QualIdent& ident, const SymbolTable& table) {
    auto res = table.get_symbol(ident);
    if (!res)
        return res.get_err();
    if (res.get_ok().group != SymbolGroup::TYPE)
        return ErrorBuilder(ident.ident.place).text("Expected type name").build();
    return {};
}

std::string BuiltInType::to_string() const {
    return fmt::format("@{}", type);
}

bool BuiltInType::is_equal(const Type& other) const {
    return type == static_cast<const BuiltInType&>(other).type;
}

TypeResult BuiltInType::normalize(const SymbolTable&, bool) {
    return make_type<BuiltInType>(*this);
}

std::string TypeName::to_string() const {
    return ident.to_string();
}

bool TypeName::is_equal(const Type& other) const {
    return ident == static_cast<const TypeName&>(other).ident;
}

TypeResult TypeName::dereference(const SymbolTable& table) const {
    auto symbol = table.get_symbol(ident);
    if (!symbol)
        return symbol.get_err();
    if (auto typeName = symbol.get_ok().type->is<TypeName>())
        return typeName->dereference(table);
    else
        return symbol.get_ok().type;
}

TypeResult TypeName::normalize(const SymbolTable& table, bool normalize_pointers) {
    auto symbol = table.get_symbol(ident);
    if (!symbol)
        return symbol.get_err();
    return symbol.get_ok().type->normalize(table, normalize_pointers);
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

TypeResult RecordType::has_field(const Ident& ident, const SymbolTable& table) const {
    for (auto& list : seq) {
        auto res = std::find_if(list.list.begin(), list.list.end(), [&ident](auto i) { return i.ident == ident; });
        if (res != list.list.end()) {
            return TypePtr(list.type);
        }
    }
    if (basetype) {
        auto base = table.get_symbol(*basetype);
        if (!base)
            return base.get_err();
        else {
            auto baseptr = dynamic_cast<RecordType*>(base.get_ok().type.get());
            if (!baseptr)
                return ErrorBuilder(basetype->ident.place)
                    .format("Internal compiler error in RecordType::has_field")
                    .build();
            return baseptr->has_field(ident, table);
        }
    }
    return ErrorBuilder(this->place).format("Field {} not found in {}", ident, this->to_string()).build();
}

TypeResult RecordType::normalize(const SymbolTable& table, bool normalize_pointers) {
    RecordType copy = *this;
    for (auto& list : copy.seq) {
        auto res = list.type->normalize(table, normalize_pointers);
        if (!res)
            return res;
        list.type = res.get_ok();
    }
    return make_type<RecordType>(copy);
}

std::string PointerType::to_string() const {
    return fmt::format("POINTER TO {}", type);
}

bool PointerType::is_equal(const Type& other) const {
    return type == static_cast<const PointerType&>(other).type;
}

Error PointerType::check_type(const SymbolTable& table) {
    auto res = type->normalize(table, false);
    if (!res)
        return res.get_err();
    return {};
}

TypeResult PointerType::normalize(const SymbolTable& table, bool normalize_pointers) {
    if (!normalize_pointers)
        return make_type<PointerType>(*this);
    PointerType copy = *this;
    auto res = copy.type->normalize(table, normalize_pointers);
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

TypeResult ArrayType::normalize(const SymbolTable& table, bool normalize_pointers) {
    ArrayType copy = *this;
    auto res = copy.type->normalize(table, normalize_pointers);
    if (!res)
        return res;
    for (auto& length : copy.lengths) {
        auto expr = length->eval(table);
        if (!expr)
            return expr.get_err();
        auto integer = dynamic_cast<Number*>(expr.get_ok().get());
        if (!integer)
            return ErrorBuilder(length->place).format("Expected number, found {}", length->to_string()).build();
        if (!std::holds_alternative<Integer>(integer->value))
            return ErrorBuilder(length->place).format("Expected integer, found real").build();
        length = expr.get_ok();
    }
    return make_type<ArrayType>(copy);
}

TypeResult ArrayType::drop_dimensions(size_t count) const {
    if (count > lengths.size())
        return ErrorBuilder(this->place)
            .format("Array of type {} has only {} dimensions", this->to_string(), lengths.size())
            .build();
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

TypeResult ProcedureType::normalize(const SymbolTable& table, bool normalize_pointers) {
    ProcedureType copy = *this;
    for (auto& section : copy.params.sections) {
        auto res = section.type->normalize(table, normalize_pointers);
        if (!res)
            return res;
        section.type = res.get_ok();
    }
    return make_type<ProcedureType>(copy);
}

ProcedureType::ProcedureType(std::optional<FormalParameters> par) {
    if (par)
        params = *par;
}
