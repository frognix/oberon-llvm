#pragma once

#include "expression.hpp"
#include "type.hpp"

namespace nodes {

enum class BaseType
{
    NONE,
    BOOL,
    CHAR,
    INTEGER,
    REAL,
    BYTE,
    SET,
    NIL
};

struct BuiltInType : Type {
    std::string to_string() const override;
    bool is_equal(const Type& other) const override;
    Maybe<TypePtr> normalize(Context&, bool normalize_pointers) override;
    bool equal_to(BaseType t);
    BuiltInType() : type() {}
    BuiltInType(Ident i);
    BuiltInType(BaseType t);
    BaseType type = BaseType::NONE;
};

inline TypePtr make_base_type(BaseType t) {
    return make_type<BuiltInType>(t);
}

inline bool is_base_type(Ident ident) {
    return ident.equal_to("BOOLEAN") || ident.equal_to("CHAR") || ident.equal_to("INTEGER") || ident.equal_to("REAL") ||
           ident.equal_to("BYTE") || ident.equal_to("SET");
}

struct TypeName : Type {
    std::string to_string() const override;
    bool is_equal(const Type& other) const override;
    Maybe<TypePtr> normalize(Context&, bool normalize_pointers) override;
    Maybe<TypePtr> dereference(Context& table) const;
    TypeName(QualIdent i) : ident(i) {
        if (!i.qual && is_base_type(i.ident))
            throw std::runtime_error("Internal error: BaseType in TypeName");
    }
    QualIdent ident;
};

using FieldListSequence = std::vector<FieldList>;

struct RecordType : Type {
    std::string to_string() const override;
    bool is_equal(const Type& other) const override;
    Maybe<TypePtr> normalize(Context&, bool normalize_pointers) override;
    Maybe<TypePtr> has_field(const Ident& ident, Context& table) const;
    RecordType(std::optional<QualIdent> b, FieldListSequence s) : basetype(b), seq(s) {}
    std::optional<QualIdent> basetype;
    FieldListSequence seq;
};

struct PointerType : Type {
    std::string to_string() const override;
    bool is_equal(const Type& other) const override;
    bool check_type(Context& table);
    Maybe<TypePtr> normalize(Context&, bool normalize_pointers) override;
    PointerType(TypePtr t) : type(t) {}
    TypePtr type;
};

struct ArrayType : Type {
    std::string to_string() const override;
    bool is_equal(const Type& other) const override;
    Maybe<TypePtr> normalize(Context&, bool normalize_pointers) override;
    Maybe<TypePtr> drop_dimensions(size_t, Context&) const;
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
    std::optional<TypePtr> rettype;
    bool operator==(const FormalParameters&) const = default;
};

struct ProcedureType : Type {
    std::string to_string() const override;
    bool is_equal(const Type& other) const override;
    Maybe<TypePtr> normalize(Context&, bool normalize_pointers) override;
    ProcedureType() {}
    ProcedureType(std::optional<FormalParameters> par);
    FormalParameters params;
};

} // namespace nodes
