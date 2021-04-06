#include "simple_procedure_table.hpp"

std::unique_ptr<ProcedureTable> build_procedure_table(const nodes::ProcedureDeclaration& proc, const nodes::ProcedureType& type,
                                                       const SymbolTable* parent, MessageContainer& mm) {
    return std::unique_ptr<ProcedureTable>(SimpleProcedureTable::parse(proc, type, parent, mm).release());
}
