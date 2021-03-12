#pragma once

// #include "symbol_table.hpp"
#include "message_container.hpp"

class SymbolTableI;

struct SemanticContext {
    SemanticContext(MessageContainer& m, const SymbolTableI& s) : messages(m), symbols(s) {}
    MessageContainer& messages;
    const SymbolTableI& symbols;
};

template <class T>
using Maybe = std::optional<T>;

const auto error = std::nullopt;
const auto berror = false;
const auto bsuccess = true;
