#pragma once

#include "expression.hpp"
#include "section.hpp"
#include "type_nodes.hpp"
#include "statement.hpp"
#include "const_value.hpp"
#include <string_view>

namespace nodes {

struct ConstDecl {
    IdentDef ident;
    ConstValue expression;
};

struct TypeDecl {
    IdentDef ident;
    TypePtr type;
    TypeDecl() {}
    TypeDecl(IdentDef i, TypePtr t) : ident(i), type(t) {}
    TypeDecl(IdentDef i) : ident(i) {
        type = make_type<ImportTypeName>(ident);
    }
};

using VariableDecl = FieldList;

struct DeclarationSequence {
    std::vector<ConstDecl> constDecls;
    std::vector<TypeDecl> typeDecls;
    std::vector<VariableDecl> variableDecls;
    std::vector<SectionPtr> procedureDecls;
};

struct ProcedureDeclarationBody {
    DeclarationSequence decls;
    StatementSequence statements;
    std::optional<ExpressionPtr> ret;
};

struct ProcedureDeclaration : Section {
    std::string to_string() const override;
    ProcedureDeclaration() {}
    ProcedureDeclaration(IdentDef n, ProcedureType t, std::variant<std::string_view, ProcedureDeclarationBody> var)
        : name(n), type(t) {
        if (var.index() == 1) {
            body = std::get<1>(var);
        }
    }
    IdentDef name;
    ProcedureType type;
    std::optional<ProcedureDeclarationBody> body;
};

struct Import {
    Import() {}
    Import(Ident first, std::optional<Ident> second);
    Ident name;
    Ident real_name;
};

using ImportList = std::vector<Import>;

struct IModule : Section {
    virtual Ident get_name() const = 0;
    virtual ImportList get_imports() const = 0;
};

struct Module : IModule {
    std::string to_string() const override;
    Module() {}
    Module(Ident n, ImportList i, DeclarationSequence d, StatementSequence b)
        : name(n), imports(i), declarations(d), body(b) {}
    Ident get_name() const override { return name; };
    ImportList get_imports() const override { return imports; }
    Ident name;
    ImportList imports;
    DeclarationSequence declarations;
    StatementSequence body;
};

struct ProcedureDefinition : Section {
    std::string to_string() const override;
    ProcedureDefinition() {}
    ProcedureDefinition(IdentDef n, ProcedureType t)
        : name(n), type(t) {}
    IdentDef name;
    ProcedureType type;
};

struct DefinitionSequence {
    std::vector<ConstDecl> constDecls;
    std::vector<TypeDecl> typeDecls;
    std::vector<VariableDecl> variableDecls;
    std::vector<ProcedureDefinition> procedureDecls;
};

struct Definition : IModule {
    std::string to_string() const override;
    Definition() {}
    Definition(Ident n, ImportList i, DefinitionSequence d)
        : name(n), imports(i), definitions(d) {}
    Ident get_name() const override { return name; };
    ImportList get_imports() const override { return imports; }
    Ident name;
    ImportList imports;
    DefinitionSequence definitions;
};

} // namespace nodes
