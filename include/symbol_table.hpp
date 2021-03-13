#pragma once

#include "symbol_table_i.hpp"
#include "nodes.hpp"
#include <unordered_map>
#include <unordered_set>

class ProcedureTable;

namespace std {

template <>
struct hash<nodes::Ident> {
    std::size_t operator()(nodes::Ident const& id) const noexcept {
        return std::hash<std::string_view>{}(std::string_view(id.value.data(), id.value.size()));
    }
};

template <>
struct hash<nodes::QualIdent> {
    std::size_t operator()(nodes::QualIdent const& id) const noexcept {
        if (!id.qual) {
            return std::hash<nodes::Ident>{}(id.ident);
        } else {
            return std::hash<nodes::Ident>{}(id.ident) ^ std::hash<nodes::Ident>{}(*id.qual);
        }
    }
};

} // namespace std

template <class T>
using SymbolMap = std::unordered_map<nodes::Ident, T>;

using SymbolSet = std::unordered_set<nodes::Ident>;

class MessageContainer;

using ParseReturnType = std::optional<std::unique_ptr<SemanticUnitI>>;

class SymbolTable;

using TablePtr = std::shared_ptr<SymbolTable>;

class SymbolTable : public SemanticUnitI {
public:
    // SymbolTable(nodes::StatementSequence b);
    static ParseReturnType parse(const nodes::DeclarationSequence&, nodes::StatementSequence, MessageContainer&);

    //SymbolTableI
    virtual Maybe<SymbolToken> get_symbol(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const override;
    virtual Maybe<nodes::ExpressionPtr> get_value(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const override;
    virtual Maybe<TablePtr> get_table(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const;
    virtual bool add_symbol(MessageContainer&, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type);
    bool add_value(MessageContainer&, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, nodes::ExpressionPtr value);
    bool add_table(MessageContainer&, nodes::IdentDef ident, SymbolGroup group, nodes::TypePtr type, TablePtr table);

    //CodeSectionI
    bool analyze_code(MessageContainer& messages) const override;

    bool has_symbol(const nodes::QualIdent& ident) const { return symbols.contains(ident.ident); }

    std::string to_string() const override;
protected:
    SymbolTable() {}
    static ParseReturnType base_parse(std::unique_ptr<SymbolTable>, const nodes::DeclarationSequence&, nodes::StatementSequence, MessageContainer&);
private:
    SymbolMap<SymbolToken> symbols;
    SymbolMap<nodes::ExpressionPtr> values;
    SymbolMap<std::shared_ptr<SymbolTable>> tables;
    nodes::StatementSequence body;
};
