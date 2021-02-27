#pragma once

#include "type.hpp"
#include "expression.hpp"

namespace nodes {

struct BuiltInType : Type {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const { return fmt::format("{}", type); }
    bool is_equal(const Type& other) const override;
    Error check(const SymbolTable&) const override;
    BuiltInType() : type() {}
    BuiltInType(Ident i) : type(i) {}
    Ident type;
};

inline TypePtr make_built_in(const char * str) {
    return make_type<BuiltInType>(str_to_ident(str));
}

inline TypePtr built_in_bool() {
    return make_built_in("BOOLEAN");
}

inline TypePtr built_in_char() {
    return make_built_in("CHAR");
}

inline TypePtr built_in_int() {
    return make_built_in("INTEGER");
}

inline TypePtr built_in_real() {
    return make_built_in("REAL");
}

inline TypePtr built_in_byte() {
    return make_built_in("BYTE");
}

inline TypePtr built_in_set() {
    return make_built_in("SET");
}

inline TypePtr built_in_nil() {
    return make_built_in("NIL");
}

struct TypeName : Type {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const { return ident.to_string(); }
    bool is_equal(const Type& other) const override;
    Error check(const SymbolTable&) const override;
    TypeName(QualIdent i) : ident(i) {}
    QualIdent ident;
};

using FieldListSequence = std::vector<FieldList>;

struct RecordType : Type {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const {
        if (basetype)
            return fmt::format("RECORD ({}) {} END", *basetype, seq);
        else
            return fmt::format("RECORD {} END", seq);
    }
    bool is_equal(const Type& other) const override;
    Error check(const SymbolTable&) const override;
    RecordType(std::optional<QualIdent> b, FieldListSequence s) : basetype(b), seq(s) {}
    std::optional<QualIdent> basetype;
    FieldListSequence seq;
};

struct PointerType : Type {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const { return fmt::format("POINTER TO {}", type); }
    bool is_equal(const Type& other) const override;
    Error check(const SymbolTable&) const override;
    PointerType(TypePtr t) : type(t) {}
    TypePtr type;
};

struct ArrayType : Type {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const { return fmt::format("ARRAY {} OF {}", fmt::join(lengths, ", "), type); }
    bool is_equal(const Type& other) const override;
    Error check(const SymbolTable&) const override;
    ArrayType(std::vector<ExpressionPtr> l, TypePtr t) : lengths(l), type(t) {}
    std::vector<ExpressionPtr> lengths;
    TypePtr type;
};

struct FormalType {
    bool array;
    QualIdent ident;
    bool operator == (const FormalType&) const = default;
};

struct FPSection {
    std::optional<Ident> var;
    std::vector<Ident> idents;
    FormalType type;
    bool operator == (const FPSection& other) const {
        return var == other.var && idents.size() == other.idents.size() && type == other.type;
    }
};

struct FormalParameters {
    std::vector<FPSection> sections;
    std::optional<QualIdent> rettype;
    bool operator == (const FormalParameters&) const = default;
};

struct ProcedureType : Type {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const { return fmt::format("PROCEDURE {}", params); }
    bool is_equal(const Type& other) const override;
    Error check(const SymbolTable&) const override;
    ProcedureType() {}
    ProcedureType(std::optional<FormalParameters> par) {
        if (par)
            params = *par;
    }
    FormalParameters params;
};

struct AnyType : Type {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const { return "ANY"; }
    bool is_equal(const Type& other) const override;
    Error check(const SymbolTable&) const override;
};

}
