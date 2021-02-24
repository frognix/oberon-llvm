#pragma once

#include "type.hpp"

namespace nodes {

struct TypeName : Type {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const { return ident.to_string(); }
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
    RecordType(std::optional<QualIdent> b, FieldListSequence s) : basetype(b), seq(s) {}
    std::optional<QualIdent> basetype;
    FieldListSequence seq;
};

struct PointerType : Type {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const { return fmt::format("POINTER TO {}", type); }
    PointerType(TypePtr t) : type(t) {}
    TypePtr type;
};

struct ArrayType : Type {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const { return fmt::format("ARRAY {} OF {}", fmt::join(lengths, ", "), type); }
    ArrayType(std::vector<ExpressionPtr> l, TypePtr t) : lengths(l), type(t) {}
    std::vector<ExpressionPtr> lengths;
    TypePtr type;
};

struct FormalType {
    bool array;
    QualIdent ident;
};

struct FPSection {
    std::optional<Ident> var;
    std::vector<Ident> idents;
    FormalType type;
};

struct FormalParameters {
    std::vector<FPSection> sections;
    std::optional<QualIdent> rettype;
};

struct ProcedureType : Type {
    const std::type_info& type_info() const { return typeid(*this); }
    std::string to_string() const { return fmt::format("PROCEDURE {}", params); }
    ProcedureType() {}
    ProcedureType(std::optional<FormalParameters> par) {
        if (par)
            params = *par;
    }
    FormalParameters params;
};

}
