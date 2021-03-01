#include "statement_nodes.hpp"
#include "expression_nodes.hpp"
#include "node_formatters.hpp"

#include "symbol_table.hpp"

using namespace nodes;

Error Assignment::check(const SymbolTable& table) const {
    auto validIdent = variable.get(table);
    if (!validIdent) return validIdent.get_err();
    auto lsymbol = validIdent.get_ok().get_symbol(table, place);
    if (!lsymbol)
        return lsymbol.get_err();
    if (lsymbol.get_ok().group != SymbolGroup::VAR)
        return ErrorBuilder(this->place).format("Expected variable").build();
    auto rtype = value->get_type(table);
    if (!rtype)
        return rtype.get_err();
    auto lltype = lsymbol.get_ok().type;
    auto rrtype = rtype.get_ok();
    auto rbuiltin = rrtype->is<BuiltInType>();
    if (*lltype == *rrtype || (lltype->is<PointerType>() && rbuiltin && rbuiltin->type == str_to_ident("NIL"))) {
        return {};
    } else {
        return ErrorBuilder(place)
            .format("Incompatible types in assignment: {} and {}", lsymbol.get_ok().type->to_string(),
                    rtype.get_ok()->to_string())
            .build();
    }
}

Error IfStatement::check(const SymbolTable&) const {
    return {};
}

Error CaseStatement::check(const SymbolTable&) const {
    return {};
}

Error WhileStatement::check(const SymbolTable&) const {
    return {};
}

Error RepeatStatement::check(const SymbolTable&) const {
    return {};
}

Error ForStatement::check(const SymbolTable&) const {
    return {};
}

Error ProcCall::check(const SymbolTable&) const {
    return {};
}
