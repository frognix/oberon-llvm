#pragma once

#include "symbol_table.hpp"
#include "module_table_i.hpp"

class ModuleTable : public ModuleTableI {
public:
    static std::unique_ptr<ModuleTable> parse(const nodes::Module& module, std::vector<std::pair<nodes::Import, ModuleTablePtr>> imports, MessageContainer& mm);

    static std::unique_ptr<ModuleTable> parse(const nodes::Definition& def, std::vector<std::pair<nodes::Import, ModuleTablePtr>> imports, MessageContainer& mm);

    Maybe<SymbolToken> get_symbol_out(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const override;
    virtual Maybe<SymbolToken> get_symbol(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const override;
    virtual Maybe<nodes::ValuePtr> get_value(MessageContainer&, const nodes::QualIdent& ident, bool secretly = false) const override;
    Maybe<TablePtr> get_table(MessageContainer& messages, const nodes::QualIdent& ident, bool secretly) const override;

    const SymbolTable& get_symbols() const override { return symbols; }

    virtual std::string to_string() const override;
    bool analyze_code(MessageContainer& messages) const override;
    void add_export(nodes::Ident ident) { m_exports.insert(ident); }
private:
    ModuleTable() {}
    bool add_import(MessageContainer&, nodes::Import import, ModuleTablePtr module);
    SymbolTable symbols;
    nodes::Ident m_name;
    SymbolMap<Import> m_imports;
    SymbolSet m_exports;
};
