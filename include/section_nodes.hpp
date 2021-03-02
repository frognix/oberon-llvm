#pragma once

#include "expression.hpp"
#include "section.hpp"
#include "type_nodes.hpp"
#include "statement.hpp"

namespace nodes {

struct ConstDecl {
    IdentDef ident;
    ExpressionPtr expression;
};

struct TypeDecl {
    IdentDef ident;
    TypePtr type;
};

using VariableDecl = FieldList;

struct DeclarationSequence {
    std::vector<ConstDecl> constDecls;
    std::vector<TypeDecl> typeDecls;
    std::vector<VariableDecl> variableDecls;
    std::vector<SectionPtr> procedureDecls;
};

struct ProcedureDeclaration : Section {
    std::string to_string() const override;
    ProcedureDeclaration() {}
    ProcedureDeclaration(IdentDef n, ProcedureType t, DeclarationSequence d, StatementSequence b,
                         std::optional<ExpressionPtr> r)
        : name(n), type(t), decls(d), body(b), ret(r) {}
    IdentDef name;
    ProcedureType type;
    DeclarationSequence decls;
    StatementSequence body;
    std::optional<ExpressionPtr> ret;
};

struct Import {
    Import() {}
    Import(Ident first, std::optional<Ident> second);
    Ident name;
    Ident real_name;
};

using ImportList = std::vector<Import>;

struct Module : Section {
    std::string to_string() const override;
    Module() {}
    Module(Ident n, ImportList i, DeclarationSequence d, StatementSequence b)
        : name(n), imports(i), declarations(d), body(b) {}
    Ident name;
    ImportList imports;
    DeclarationSequence declarations;
    StatementSequence body;
};

} // namespace nodes
