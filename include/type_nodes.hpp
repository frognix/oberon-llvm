#pragma once

#include "expression.hpp"
#include "internal_error.hpp"
#include "type.hpp"

namespace nodes {

struct BuiltInType : Type {
    std::string to_string() const override;
    Maybe<TypePtr> normalize(Context&, bool normalize_pointers) const override;
    bool equal_to(BaseType t) const;
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
    Maybe<TypePtr> normalize(Context&, bool normalize_pointers) const override;
    Maybe<TypePtr> dereference(Context& table) const;
    TypeName(QualIdent i) : ident(i) {
        if (!i.qual && is_base_type(i.ident))
            internal::compiler_error("BaseType in TypeName");
    }
    QualIdent ident;
};

struct ImportTypeName : Type {
    std::string to_string() const override;
    Maybe<TypePtr> normalize(Context&, bool normalize_pointers) const override;
    ImportTypeName(IdentDef i) : ident{{}, i.ident} {
        if (!ident.qual && is_base_type(ident.ident))
            internal::compiler_error("BaseType in TypeName");
    }
    QualIdent ident;
};

using FieldListSequence = std::vector<FieldList>;

struct RecordType : Type {
    std::string to_string() const override;
    Maybe<TypePtr> normalize(Context&, bool normalize_pointers) const override;
    Maybe<TypePtr> has_field(const Ident& ident, Context& table) const;
    bool extends(Context&, const Type&) const;
    RecordType(std::optional<QualIdent> b, FieldListSequence s) : basetype(b), seq(s) {}
    std::optional<QualIdent> basetype;
    FieldListSequence seq;
};

struct PointerType : Type {
    std::string to_string() const override;
    bool check_type(Context& table);
    const RecordType& get_type(Context&) const;
    Maybe<TypePtr> normalize(Context&, bool normalize_pointers) const override;
    PointerType(TypePtr t) : type(t) {}
    TypePtr type;
};

struct ConstStringType : Type {
    std::string to_string() const override;
    Maybe<TypePtr> normalize(Context&, bool normalize_pointers) const override;
    ConstStringType(size_t s) : size(s) {}
    size_t size;
};

struct ArrayType : Type {
    std::string to_string() const override;
    Maybe<TypePtr> normalize(Context&, bool normalize_pointers) const override;
    Maybe<TypePtr> drop_dimensions(size_t, Context&) const;
    ArrayType(std::vector<ExpressionPtr> l, TypePtr t, bool u = false);
    ExpressionPtr length;
    TypePtr type;
    bool open_array;
};

struct FPSection {
    std::optional<Ident> var;
    std::vector<Ident> idents;
    TypePtr type;
};

struct FormalParameter {
    Ident ident;
    TypePtr type;
    bool var;
};

struct FormalParameters {
    std::vector<FormalParameter> params;
    std::optional<TypePtr> rettype;
    FormalParameters() {}
    FormalParameters(std::vector<FPSection> sections, std::optional<TypePtr> r) : rettype(r) {
        for (auto section : sections) {
            for (auto ident : section.idents) {
                params.push_back(FormalParameter{ident, section.type, section.var.has_value()});
            }
        }
    }
    bool operator==(const FormalParameters&) const = default;
    bool match(Context&, const FormalParameters&) const;
};

struct ProcedureType : Type {
    std::string to_string() const override;
    Maybe<TypePtr> normalize(Context&, bool normalize_pointers) const override;
    ProcedureType() {}
    ProcedureType(std::optional<FormalParameters> par);
    FormalParameters params;
};

} // namespace nodes
