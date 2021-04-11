#pragma once

#include "message_container.hpp"
#include "section_nodes.hpp"
#include "symbol_container.hpp"
#include "symbol_table.hpp"
#include <memory>

class ProcedureTable : public SemanticUnit {
public:
    virtual bool can_overload(MessageContainer&, const ProcedureTable&) const = 0;
    virtual bool overload(MessageContainer&, std::shared_ptr<ProcedureTable>) = 0;
    virtual const SymbolTable& parent() const = 0;

    virtual Maybe<SymbolToken> get_symbol(MessageContainer& messages, const nodes::QualIdent& ident, bool secretly) const override {
        if (ident.qual) {
            return parent().get_symbol(messages, ident, secretly);
        } else {
            if (!get_symbols().has_symbol(ident)) {
                return parent().get_symbol(messages, ident, secretly);
            } else {
                auto res = get_symbols().get_symbol(messages, ident, secretly);
                if (!res) return error;
                return res;
            }
        }
    }

    virtual Maybe<nodes::ValuePtr> get_value(MessageContainer& messages, const nodes::QualIdent& ident, bool secretly) const override {
        if (ident.qual) {
            return parent().get_value(messages, ident, secretly);
        } else {
            if (!get_symbols().has_symbol(ident)) {
                return parent().get_value(messages, ident, secretly);
            } else {
                auto res = get_symbols().get_value(messages, ident, secretly);
                if (!res) return error;
                return res;
            }
        }
    }

    virtual Maybe<TablePtr> get_table(MessageContainer& messages, const nodes::QualIdent& ident, bool secretly) const override {
        if (ident.qual) {
            return parent().get_table(messages, ident, secretly);
        } else {
            if (!get_symbols().has_symbol(ident)) {
                return parent().get_table(messages, ident, secretly);
            } else {
                auto res = get_symbols().get_table(messages, ident, secretly);
                if (!res) return error;
                return res;
            }
        }
    }

    virtual std::string to_string() const override {
        return get_symbols().to_string();
    }

    virtual bool analyze_code(MessageContainer& messages) const override {
        auto context = nodes::Context(messages, *this);
        return get_symbols().analyze_code(context);
    }
};

std::unique_ptr<ProcedureTable> build_procedure_table(const nodes::ProcedureDeclaration& proc,
                                                       const SymbolTable* parent, MessageContainer& mm);

bool parseProcedureType(MessageContainer& messages, SymbolTable& table, const nodes::ProcedureDeclaration& type);
