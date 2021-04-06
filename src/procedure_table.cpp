#include "simple_procedure_table.hpp"

std::unique_ptr<IProcedureTable> build_procedure_table(const nodes::ProcedureDeclaration& proc, const nodes::ProcedureType& type,
                                                       const SymbolTableI* parent, MessageContainer& mm) {
    return std::unique_ptr<IProcedureTable>(SimpleProcedureTable::parse(proc, type, parent, mm).release());
}
