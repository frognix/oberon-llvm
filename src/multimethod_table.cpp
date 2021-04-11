#include "multimethod_table.hpp"
#include "message_container.hpp"
#include "semantic_context.hpp"
#include "symbol_container.hpp"
#include "symbol_table.hpp"
#include "type_nodes.hpp"
#include <ranges>

std::unique_ptr<MultimethodTable> MultimethodTable::parse(const nodes::ProcedureDeclaration& proc, const SymbolTable* parent,
                                                          MessageContainer& messages) {
    if (proc.type.params.common.size() == 0)
        internal::compiler_error("Bad ProcedureDeclaration structure");
    std::unique_ptr<MultimethodTable> table(new MultimethodTable());
    table->m_name = proc.name.ident;
    table->m_parent = parent;
    table->m_type = proc.type;
    auto success = parseProcedureType(messages, *table, proc);
    if (!success) return nullptr;
    return table;
}

bool MultimethodTable::suitable_declaration(const nodes::ProcedureDeclaration& proc) {
    if (proc.type.params.common.size() == 0) return false;
    for (auto section : proc.type.params.common) {
        if (!section.type->is<nodes::CommonType>()) return false;
    }
    return true;
}

bool MultimethodTable::instance_compatible(MessageContainer& messages, bool with_messages, const ProcedureTable& table) const {
    if (auto instance = dynamic_cast<const MultimethodInstanceTable*>(&table); instance) {
        auto& multi = m_type.params.common;
        auto& inst  = instance->m_type.params.common;
        auto multiIt = multi.begin();
        auto instIt = inst.begin();
        auto context = nodes::Context(messages, *this);
        for (; instIt != inst.end() && multiIt != multi.end(); (++multiIt,++instIt)) {
            if (auto scalar_type = instIt->type->is<nodes::ScalarType>(); scalar_type) {
                auto res = multiIt->type->same(context, *scalar_type->type);
                if (!res) {
                    if (with_messages) messages.addErr(instance->m_name.place, "Expected same common type, found: {}", instIt->type->to_string());
                    return false;
                }
            } else {
                if (with_messages) messages.addErr(instance->m_name.place, "Expected scalar type, found: {}", instIt->type->to_string());
                return false;
            }
        }
        if (instIt != inst.end() || multiIt != multi.end()) {
            if (with_messages) messages.addErr(instance->m_name.place, "Incompatible common parameters types");
            return false;
        }
        if (!m_type.params.match(context, instance->m_type.params, false, true)) {
            if (with_messages) messages.addErr(instance->m_name.place, "Incompatible formal parameters types");
            return false;
        }
        return true;
    }
    return false;
}

bool MultimethodTable::can_overload(MessageContainer& messages, const ProcedureTable& table) const {
    return instance_compatible(messages, false, table);
}

bool MultimethodTable::overload(MessageContainer& messages, std::shared_ptr<ProcedureTable> table) {
    if (instance_compatible(messages, true, *table)) {
        m_instances.push_back(std::static_pointer_cast<MultimethodInstanceTable>(table));
        return true;
    }
    return false;
}

bool MultimethodTable::analyze_code(MessageContainer& messages) const {
    auto context = nodes::Context(messages, *this);
    bool success = get_symbols().analyze_code(context);
    for (auto instance : m_instances) {
        if (!instance->analyze_code(messages)) success = false;
    }
    return success;
}

std::unique_ptr<MultimethodInstanceTable> MultimethodInstanceTable::parse(const nodes::ProcedureDeclaration& proc,
                                                                          const SymbolTable* parent,
                                                                          MessageContainer& messages) {
    if (proc.type.params.common.size() == 0 || !proc.body)
        internal::compiler_error("Bad ProcedureDeclaration structure");
    auto multimethod_base = parent->get_symbol(messages, nodes::QualIdent{{}, proc.name.ident}, true);
    if (!multimethod_base) {
        messages.addErr(proc.name.ident.place, "Multimethod base not found");
        return nullptr;
    }
    std::unique_ptr<MultimethodInstanceTable> table(new MultimethodInstanceTable());
    table->m_name = proc.name.ident;
    table->m_parent = parent;
    table->m_type = proc.type;
    auto success = parseProcedureType(messages, *table, proc);
    if (!success) return nullptr;
    return table;
}

bool MultimethodInstanceTable::suitable_declaration(const nodes::ProcedureDeclaration& proc) {
    if (proc.type.params.common.size() == 0) return false;
    if (!proc.body) return false;
    for (auto section : proc.type.params.common) {
        if (!section.type->is<nodes::ScalarType>()) return false;
    }
    return true;
}
