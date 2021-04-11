#pragma once

#include "const_value.hpp"
#include "expression.hpp"
#include "expression_nodes.hpp"
#include "internal_error.hpp"
#include "node.hpp"
#include "type.hpp"

namespace nodes {

struct BuiltInType : Type {
    std::string to_string() const override;
    Maybe<TypePtr> normalize(Context&, bool normalize_pointers) const override;
    virtual bool same(Context& context, const Type& other) const override;
    virtual bool equal(Context& context, const Type& other) const override;
    virtual bool assignment_compatible(Context& context, const Type& expr) const override;
    bool equal_to(BaseType t) const;
    BuiltInType() : type() {}
    BuiltInType(std::string_view i);
    BuiltInType(BaseType t);
    BaseType type = BaseType::VOID;
};

inline TypePtr make_base_type(BaseType t) {
    return make_type<BuiltInType>(t);
}

inline bool is_base_type(Ident ident) {
    return ident.equal_to("BOOLEAN") || ident.equal_to("CHAR") || ident.equal_to("INTEGER") || ident.equal_to("REAL") ||
           ident.equal_to("BYTE") || ident.equal_to("SET");
}

const char* basetype_to_str(BaseType type);

struct TypeName : Type {
    std::string to_string() const override;
    Maybe<TypePtr> normalize(Context&, bool normalize_pointers) const override;
    virtual bool same(Context& context, const Type& other) const override;
    virtual bool equal(Context& context, const Type& other) const override;
    virtual bool assignment_compatible(Context& context, const Type& expr) const override;
    Maybe<TypePtr> dereference(Context& table) const;
    TypeName() {}
    TypeName(QualIdent i) : ident(i) {
        if (!i.qual && is_base_type(i.ident))
            internal::compiler_error("BaseType in TypeName");
    }
    QualIdent ident;
};

struct ImportTypeName : Type {
    std::string to_string() const override;
    Maybe<TypePtr> normalize(Context&, bool normalize_pointers) const override;
    virtual bool same(Context& context, const Type& other) const override;
    virtual bool equal(Context& context, const Type& other) const override;
    virtual bool assignment_compatible(Context& context, const Type& expr) const override;
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
    virtual bool same(Context& context, const Type& other) const override;
    virtual bool equal(Context& context, const Type& other) const override;
    virtual bool assignment_compatible(Context& context, const Type& expr) const override;
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
    virtual bool same(Context& context, const Type& other) const override;
    virtual bool equal(Context& context, const Type& other) const override;
    virtual bool assignment_compatible(Context& context, const Type& expr) const override;
    PointerType(TypePtr t) : type(t) {}
    TypePtr type;
};

struct ConstStringType : Type {
    std::string to_string() const override;
    Maybe<TypePtr> normalize(Context&, bool normalize_pointers) const override;
    virtual bool same(Context& context, const Type& other) const override;
    virtual bool equal(Context& context, const Type& other) const override;
    virtual bool assignment_compatible(Context& context, const Type& expr) const override;
    ConstStringType(size_t s) : size(s) {}
    size_t size;
};

struct ArrayType : Type {
    std::string to_string() const override;
    Maybe<TypePtr> normalize(Context&, bool normalize_pointers) const override;
    virtual bool same(Context& context, const Type& other) const override;
    virtual bool equal(Context& context, const Type& other) const override;
    virtual bool assignment_compatible(Context& context, const Type& expr) const override;
    Maybe<TypePtr> drop_dimensions(size_t, Context&) const;
    static bool compatible(Context&, const Type& actual, const Type& formal);
    ArrayType(std::vector<ExpressionPtr> l, TypePtr t, bool u = false);
    ConstValue length;
    TypePtr type;
    bool open_array;
};

struct FPSection {
    bool var;
    std::vector<Ident> idents;
    TypePtr type;
};

struct FormalParameter {
    Ident ident;
    TypePtr type;
    bool var;
};

struct FormalParameters {
    std::vector<FormalParameter> common;
    std::vector<FormalParameter> formal;
    std::optional<TypePtr> rettype;
    FormalParameters() {}
    FormalParameters(std::optional<std::vector<FPSection>> commonSections, std::optional<std::vector<FPSection>> sections, std::optional<TypePtr> r) : rettype(r) {
        if (commonSections) {
            for (auto section : commonSections.value()) {
                for (auto ident : section.idents) {
                    common.push_back(FormalParameter{ident, section.type, section.var});
                }
            }
        }
        if (sections) {
            for (auto section : sections.value()) {
                for (auto ident : section.idents) {
                    formal.push_back(FormalParameter{ident, section.type, section.var});
                }
            }
        }
    }
    bool operator==(const FormalParameters&) const = default;
    bool match(Context& context, const FormalParameters& other, bool match_first, bool match_second) const;
};

struct ProcedureType : Type {
    std::string to_string() const override;
    Maybe<TypePtr> normalize(Context&, bool normalize_pointers) const override;
    virtual bool same(Context& context, const Type& other) const override;
    virtual bool equal(Context& context, const Type& other) const override;
    virtual bool assignment_compatible(Context& context, const Type& expr) const override;
    ProcedureType() {}
    ProcedureType(std::optional<FormalParameters> par);
    FormalParameters params;
};

struct CommonFeature {
    bool operator == (const CommonFeature&) const = default;
    std::variant<Ident, ConstInteger, String> value;
};

struct CommonFeatureType {
    enum {
        INTEGER, BYTE, SET, CHAR, TYPE, LOCAL
    } value;
    CommonFeatureType() {}
    CommonFeatureType(std::string_view type) {
        if (type == "INTEGER") value = INTEGER;
        else if (type == "BYTE") value = BYTE;
        else if (type == "SET") value = SET;
        else if (type == "CHAR") value = CHAR;
        else if (type == "TYPE") value = TYPE;
        else if (type == "LOCAL") value = LOCAL;
        else internal::compiler_error("Incorrect CommonFeatureType");
    }
    const char* to_string() const {
        switch (value) {
            case INTEGER: return "INTEGER";
            case BYTE:    return "BYTE";
            case SET:     return "SET";
            case CHAR:    return "CHAR";
            case TYPE:    return "TYPE";
            case LOCAL:   return "LOCAL";
            default: internal::compiler_error("Incorrect CommonFeatureType");
        }
    }
};

struct CommonPair {
    CommonFeature feature;
    TypePtr type;
};

struct CommonType : Type {
    std::string to_string() const override;
    Maybe<TypePtr> normalize(Context&, bool normalize_pointers) const override;
    bool same(Context& context, const Type& other) const override;
    bool equal(Context& context, const Type& other) const override;
    bool assignment_compatible(Context& context, const Type& expr) const override;

    bool has_case(CommonFeature feature) const;
    const Type& get_case(CommonFeature feature) const;

    CommonType() {}
    CommonType(std::optional<CommonFeatureType> cft, std::vector<CommonPair> pl, std::optional<TypePtr> ec)
        : common_feature_type(cft), pair_list(pl), else_clause(ec) {}

    std::optional<CommonFeatureType> common_feature_type;
    std::vector<CommonPair> pair_list;
    std::optional<TypePtr> else_clause;
};

struct ScalarType : Type {
    std::string to_string() const override;
    Maybe<TypePtr> normalize(Context&, bool normalize_pointers) const override;
    bool same(Context& context, const Type& other) const override;
    bool equal(Context& context, const Type& other) const override;
    bool assignment_compatible(Context& context, const Type& expr) const override;

    ScalarType() {}
    ScalarType(TypePtr t, CommonFeature f) : type(t), feature(f) {}

    const Type& get_type() const { return type->is<CommonType>()->get_case(feature); }

    TypePtr type;
    CommonFeature feature;
};

} // namespace nodes
