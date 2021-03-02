#pragma once

#include "expression.hpp"
#include "type.hpp"

namespace nodes {

struct BuiltInType : Type {
    std::string to_string() const override;
    bool is_equal(const Type& other) const override;
    TypeResult normalize(const SymbolTable&, bool normalize_pointers) override;
    BuiltInType() : type() {}
    BuiltInType(Ident i) : type(i) {}
    Ident type;
};

inline TypePtr make_built_in(const char* str) {
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

inline bool is_base_type(Ident ident) {
    return str_to_ident("BOOLEAN") == ident || str_to_ident("CHAR") == ident || str_to_ident("INTEGER") == ident ||
           str_to_ident("REAL") == ident || str_to_ident("BYTE") == ident || str_to_ident("SET") == ident;
}

struct TypeName : Type {
    std::string to_string() const override;
    bool is_equal(const Type& other) const override;
    TypeResult normalize(const SymbolTable&, bool normalize_pointers) override;
    TypeResult dereference(const SymbolTable& table) const;
    TypeName(QualIdent i) : ident(i) {}
    QualIdent ident;
};

using FieldListSequence = std::vector<FieldList>;

struct RecordType : Type {
    std::string to_string() const override;
    bool is_equal(const Type& other) const override;
    TypeResult normalize(const SymbolTable&, bool normalize_pointers) override;
    TypeResult has_field(const Ident& ident, const SymbolTable& table) const;
    RecordType(std::optional<QualIdent> b, FieldListSequence s) : basetype(b), seq(s) {}
    std::optional<QualIdent> basetype;
    FieldListSequence seq;
};

struct PointerType : Type {
    std::string to_string() const override;
    bool is_equal(const Type& other) const override;
    Error check_type(const SymbolTable& table);
    TypeResult normalize(const SymbolTable&, bool normalize_pointers) override;
    PointerType(TypePtr t) : type(t) {}
    TypePtr type;
};

struct ArrayType : Type {
    std::string to_string() const override;
    bool is_equal(const Type& other) const override;
    TypeResult normalize(const SymbolTable&, bool normalize_pointers) override;
    TypeResult drop_dimensions(size_t count) const;
    ArrayType(std::vector<ExpressionPtr> l, TypePtr t, bool u = false) : lengths(l), type(t), unsized(u) {}
    std::vector<ExpressionPtr> lengths;
    TypePtr type;
    bool unsized;
};

struct FPSection {
    std::optional<Ident> var;
    std::vector<Ident> idents;
    TypePtr type;
    bool operator==(const FPSection& other) const {
        return var == other.var && idents.size() == other.idents.size() && type == other.type;
    }
};

struct FormalParameters {
    std::vector<FPSection> sections;
    std::optional<QualIdent> rettype;
    bool operator==(const FormalParameters&) const = default;
};

struct ProcedureType : Type {
    std::string to_string() const override;
    bool is_equal(const Type& other) const override;
    TypeResult normalize(const SymbolTable&, bool normalize_pointers) override;
    ProcedureType() {}
    ProcedureType(std::optional<FormalParameters> par);
    FormalParameters params;
};

} // namespace nodes
