#include "nodes.hpp"

using namespace nodes;

bool BuiltInType::is_equal(const Type& other) const {
    return type == static_cast<const BuiltInType&>(other).type;
}

Error BuiltInType::check(const SymbolTable&) const {
    return {};
}

bool TypeName::is_equal(const Type& other) const {
    return ident == static_cast<const TypeName&>(other).ident;
}

Error TypeName::check(const SymbolTable& table) const {

}

bool RecordType::is_equal(const Type& _other) const {
    auto other = static_cast<const RecordType&>(_other);
    return basetype == other.basetype && seq == other.seq;
}

Error RecordType::check(const SymbolTable&) const {}

bool PointerType::is_equal(const Type& other) const {
    return type == static_cast<const PointerType&>(other).type;
}

Error PointerType::check(const SymbolTable&) const {}

//! \todo Нормальное сравнение массивов
bool ArrayType::is_equal(const Type& _other) const {
    auto other = static_cast<const ArrayType&>(_other);
    return type == other.type && lengths.size() == other.lengths.size();
}

Error ArrayType::check(const SymbolTable&) const {}

bool ProcedureType::is_equal(const Type& other) const {
    return params == static_cast<const ProcedureType&>(other).params;
}

Error ProcedureType::check(const SymbolTable&) const {}

bool AnyType::is_equal(const Type& other) const {
    return true;
}

Error AnyType::check(const SymbolTable&) const {
    return {};
}
