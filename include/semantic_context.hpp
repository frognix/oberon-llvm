#pragma once

// #include "symbol_table.hpp"
#include "message_manager.hpp"

class SymbolTable;

struct SemanticContext {
    SemanticContext(MessageManager& m, const SymbolTable& s) : messages(m), symbols(s) {}
    MessageManager& messages;
    const SymbolTable& symbols;
};

template <class T>
using Maybe = std::optional<T>;

const auto error = std::nullopt;
const auto berror = false;
const auto bsuccess = true;
