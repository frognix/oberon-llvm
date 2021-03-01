#include "nodes.hpp"
#include "symbol_table.hpp"

using namespace nodes;

inline Error check_type(const QualIdent& ident, const SymbolTable& table) {
    auto res = table.get_symbol(ident);
    if (!res) return res.get_err();
    if (res.get_ok().group != SymbolGroup::TYPE)
        return ErrorBuilder(ident.ident.place).text("Expected type name").build();
    return {};
}

bool BuiltInType::is_equal(const Type& other) const {
    return type == static_cast<const BuiltInType&>(other).type;
}

Error BuiltInType::check(const SymbolTable&) const {
    return {};
}

TypeResult BuiltInType::normalize(const SymbolTable&, bool) {
    return make_type<BuiltInType>(*this);
}

bool TypeName::is_equal(const Type& other) const {
    return ident == static_cast<const TypeName&>(other).ident;
}

Error TypeName::check(const SymbolTable& table) const {
    return check_type(ident, table);
}

TypeResult TypeName::dereference(const SymbolTable& table) const {
    auto symbol = table.get_symbol(ident);
    if (!symbol) return symbol.get_err();
    if (auto typeName = symbol.get_ok().type->is<TypeName>())
        return typeName->dereference(table);
    else
        return symbol.get_ok().type;
}

TypeResult TypeName::normalize(const SymbolTable& table, bool normalize_pointers) {
    auto symbol = table.get_symbol(ident);
    if (!symbol) return symbol.get_err();
    return symbol.get_ok().type->normalize(table, normalize_pointers);
}

bool RecordType::is_equal(const Type& _other) const {
    auto other = static_cast<const RecordType&>(_other);
    return basetype == other.basetype && seq == other.seq;
}

Error RecordType::check(const SymbolTable& table) const {
    if (basetype) {
        auto err = check_type(*basetype, table);
        if (err) return err;
    }
    for (auto& fieldList : seq) {
        auto err = fieldList.type->check(table);
        if (err) return err;
    }
    return {};
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
        if (!base) return base.get_err();
        else {
            auto baseptr = dynamic_cast<RecordType*>(base.get_ok().type.get());
            if (!baseptr) return ErrorBuilder(basetype->ident.place).format("Internal compiler error in RecordType::has_field").build();
            return baseptr->has_field(ident, table);
        }
    }
    return ErrorBuilder(this->place).format("Field {} not found in {}", ident, this->to_string()).build();
}

TypeResult RecordType::normalize(const SymbolTable& table, bool normalize_pointers) {
    RecordType copy = *this;
    for (auto& list : copy.seq) {
        auto res = list.type->normalize(table, normalize_pointers);
        if (!res) return res;
        list.type = res.get_ok();
    }
    return make_type<RecordType>(copy);
}

bool PointerType::is_equal(const Type& other) const {
    return type == static_cast<const PointerType&>(other).type;
}

Error PointerType::check(const SymbolTable& table) const {
    return type->check(table);
}

Error PointerType::check_type(const SymbolTable& table) {
    auto res = type->normalize(table, false);
    if (!res) return res.get_err();
    return {};
}

TypeResult PointerType::normalize(const SymbolTable& table, bool normalize_pointers) {
    if (!normalize_pointers) return make_type<PointerType>(*this);
    PointerType copy = *this;
    auto res = copy.type->normalize(table, normalize_pointers);
    if (!res) return res;
    return make_type<PointerType>(copy);
}

//! \todo Нормальное сравнение массивов
bool ArrayType::is_equal(const Type& _other) const {
    auto other = static_cast<const ArrayType&>(_other);
    return type == other.type && lengths.size() == other.lengths.size();
}

Error ArrayType::check(const SymbolTable& table) const {
    auto err = type->check(table);
    if (err) return err;
    for (auto length : lengths) {
        auto expr = length->eval(table);
        if (!expr) return expr.get_err();
        auto integer = dynamic_cast<Number*>(expr.get_ok().get());
        if (!integer) return ErrorBuilder(length->place).format("Expected number, found {}", length->to_string()).build();
        if (!std::holds_alternative<Integer>(integer->value))
            return ErrorBuilder(length->place).format("Expected integer, found real").build();
    }
    return {};
}

TypeResult ArrayType::normalize(const SymbolTable& table, bool normalize_pointers) {
    ArrayType copy = *this;
    auto res = copy.type->normalize(table, normalize_pointers);
    if (!res) return res;
    for (auto& length : copy.lengths) {
        auto expr = length->eval(table);
        if (!expr) return expr.get_err();
        auto integer = dynamic_cast<Number*>(expr.get_ok().get());
        if (!integer) return ErrorBuilder(length->place).format("Expected number, found {}", length->to_string()).build();
        if (!std::holds_alternative<Integer>(integer->value))
            return ErrorBuilder(length->place).format("Expected integer, found real").build();
        length = expr.get_ok();
    }
    return make_type<ArrayType>(copy);
}

bool ProcedureType::is_equal(const Type& other) const {
    return params == static_cast<const ProcedureType&>(other).params;
}

//! \todo Проблемы с QualIdent на месте rettype?
Error ProcedureType::check(const SymbolTable&) const {
    // if (params.rettype) {
    //     auto ret = table.get_
    // }
    return {};
}

TypeResult ProcedureType::normalize(const SymbolTable& table, bool normalize_pointers) {
    ProcedureType copy = *this;
    for (auto& section : copy.params.sections) {
        auto res = section.type->normalize(table, normalize_pointers);
        if (!res) return res;
        section.type = res.get_ok();
    }
    return make_type<ProcedureType>(copy);
}

// bool AnyType::is_equal(const Type&) const {
//     return true;
// }

// Error AnyType::check(const SymbolTable&) const {
//     return {};
// }

// TypeResult AnyType::normalize(const SymbolTable&, bool) {
//     return make_type<AnyType>();
// }

// bool FormalType::equalTo(const TypePtr& type, const SymbolTable& table) const {
//     auto _symbol = table.get_symbol(ident);
//     if (!type || _symbol.get_ok().group != SymbolGroup::TYPE)
//         throw std::runtime_error("Internal compiler error (FormalType::equalTo)");
//     auto symbol = _symbol.get_ok();
//     if (type == symbol.type) return true;
//     auto arrayType = dynamic_cast<ArrayType*>(type.get());
//     if (array && arrayType && arrayType->type == symbol.type) return true;
//     else return false;
// }
