#include "node_formatters.hpp"
#include "nodes.hpp"
#include "semantic_context.hpp"
#include "symbol_table.hpp"
#include "type_nodes.hpp"

using namespace nodes;

//! Same types: http://miasap.se/obnc/type-compatibility.html
bool nodes::same_types(Context& context, const Type& left, const Type& right) {
    auto lname = left.is<TypeName>();
    auto rname = right.is<TypeName>();
    if (lname && rname) {
        return lname->ident == rname->ident;
    }
    const TypeName* name;
    const Type* type;
    if (lname) {
        name = lname;
        type = &right;
    } else if (rname) {
        name = rname;
        type = &left;
    } else {
        auto lbase = left.is<BuiltInType>();
        auto rbase = right.is<BuiltInType>();
        if (lbase && rbase) return lbase->equal_to(rbase->type);
        return &left == &right;
    }
    auto sres = context.symbols.get_symbol(context.messages, name->ident);
    if (!sres) return berror;
    return same_types(context, *sres->type, *type);
}

bool nodes::equal_types(Context& context, const Type& left, const Type& right) {
    if (same_types(context, left, right)) return true;
    auto larray = left.is<ArrayType>();
    auto rarray = right.is<ArrayType>();
    if (!larray != !rarray) return false;
    if (larray && larray->open_array && rarray->open_array) return true;
    auto lproc = left.is<ProcedureType>();
    auto rproc = left.is<ProcedureType>();
    if (!lproc || !rproc) return false;
    return lproc->params.match(context, rproc->params);
}

bool nodes::assignment_compatible_types(Context& context, const Type& var, const Type& expr) {
    //Te and Tv are the same type;
    if (same_types(context, var, expr)) return true;
    //Tv is CHAR and e is a single-character string constant;
    if (auto base = var.is<BuiltInType>(); base && base->equal_to(BaseType::CHAR)) {
        auto string = expr.is<ConstStringType>();
        return string && string->size == 1;
    }
    //Te is INTEGER and Tv is BYTE or vice versa;
    {
        auto vbase = var.is<BuiltInType>();
        auto ebase = expr.is<BuiltInType>();
        if (vbase && ebase) {
            return (vbase->equal_to(BaseType::INTEGER) || vbase->equal_to(BaseType::BYTE))
                && (ebase->equal_to(BaseType::INTEGER) || ebase->equal_to(BaseType::BYTE));
        }
    }
    //Tv is ARRAY n OF CHAR, e is a string constant with m characters, and m < n;
    {
        auto varray = var.is<ArrayType>();
        auto estring = expr.is<ConstStringType>();
        if (varray && !varray->open_array && estring) {
            if (varray->open_array) return false;
            auto atype = varray->type->is<BuiltInType>();
            if (!atype || !atype->equal_to(BaseType::CHAR)) return false;
            auto n = std::get<1>(varray->length->is<Number>()->value);
            auto m = estring->size;
            return static_cast<int>(m) < n;
        }
    }
    //Te is an open array, Tv is a non-open array and their element types are equal;
    {
        auto varray = var.is<ArrayType>();
        auto earray = expr.is<ArrayType>();
        if (varray && earray) {
            return earray->open_array && !varray->open_array && equal_types(context, *varray->type, *earray->type);
        }
    }
    //Te and Tv are record types and Te is an extension of Tv and the dynamic type of v is Tv;
    {
        auto vrecord = var.is<RecordType>();
        auto erecord = expr.is<RecordType>();
        if (vrecord && erecord) {
            return erecord->extends(context, *vrecord);
            //dynamic type checking is only possible in runtime
        }
    }
    //Te and Tv are pointer types and Te is an extension of Tv;
    {
        auto vpointer = var.is<PointerType>();
        auto epointer = expr.is<PointerType>();
        if (vpointer && epointer) {
            return epointer->get_type(context).extends(context, vpointer->get_type(context));
        }
    }
    //Tv is a pointer or a procedure type and e is NIL;
    {
        auto vpointer = var.is<PointerType>();
        auto vproc = var.is<ProcedureType>();
        auto enil = expr.is<BuiltInType>();
        if ((vpointer || vproc) && enil) {
            return enil->equal_to(BaseType::NIL);
        }
    }
    //Tv is a procedure type and e is the name of a procedure whose formal parameters match those of Tv.
    {
        auto vproc = var.is<ProcedureType>();
        auto eproc = expr.is<ProcedureType>();
        if (vproc && eproc) {
            return vproc->params.match(context, eproc->params);
        }
    }
    return false;
}

bool nodes::array_compatible(Context& context, const Type& actual, const Type& formal) {
    //Tf and Ta are the same type;
    if (same_types(context, actual, formal)) return true;

    auto farray = formal.is<ArrayType>();
    if (!farray) return false;
    //Tf is an open array, Ta is any array, and their element types are array compatible;
    if (auto aarray = actual.is<ArrayType>(); aarray) {
        return farray->open_array && array_compatible(context, *aarray->type, *farray->type);
    }
    //Tf is ARRAY OF CHAR and a is a string.
    if (auto astring = actual.is<ConstStringType>(); astring) {
        auto base = farray->type->is<BuiltInType>();
        return base->equal_to(BaseType::CHAR); //Size?
    }
    return false;
}

const char* basetype_to_str(BaseType type) {
    switch (type) {
        case BaseType::BOOL: return "BOOL";
        case BaseType::CHAR: return "CHAR";
        case BaseType::INTEGER: return "INTEGER";
        case BaseType::REAL: return "REAL";
        case BaseType::BYTE: return "BYTE";
        case BaseType::SET: return "SET";
        case BaseType::NIL: return "NIL";
        default: throw std::runtime_error("Internal error: Bad BaseType");
    }
}

BaseType ident_to_basetype(Ident i) {
    if (i.equal_to("BOOLEAN"))
        return BaseType::BOOL;
    if (i.equal_to("CHAR"))
        return BaseType::CHAR;
    if (i.equal_to("INTEGER"))
        return BaseType::INTEGER;
    if (i.equal_to("REAL"))
        return BaseType::REAL;
    if (i.equal_to("BYTE"))
        return BaseType::BYTE;
    if (i.equal_to("SET"))
        return BaseType::SET;
    if (i.equal_to("NIL"))
        return BaseType::NIL;
    throw std::runtime_error(fmt::format("Internal error: Bad BaseType ({})", i));
}

bool BuiltInType::equal_to(BaseType other) const {
    return type == other;
}

BuiltInType::BuiltInType(Ident i) : type(ident_to_basetype(i)) {}

BuiltInType::BuiltInType(BaseType t) : type(t) {}

std::string BuiltInType::to_string() const {
    return fmt::format("@{}", basetype_to_str(type));
}

Maybe<TypePtr> BuiltInType::normalize(Context&, bool) {
    return make_type<BuiltInType>(*this);
}

std::string TypeName::to_string() const {
    return ident.to_string();
}

Maybe<TypePtr> TypeName::dereference(Context& context) const {
    auto symbol = context.symbols.get_symbol(context.messages, ident);
    if (!symbol)
        return error;
    if (auto typeName = symbol->type->is<TypeName>())
        return typeName->dereference(context);
    else
        return symbol->type;
}

Maybe<TypePtr> TypeName::normalize(Context& context, bool normalize_pointers) {
    auto symbol = context.symbols.get_symbol(context.messages, ident);
    if (!symbol)
        return error;
    return symbol->type->normalize(context, normalize_pointers);
}

std::string RecordType::to_string() const {
    if (basetype)
        return fmt::format("RECORD ({}) {} END", *basetype, seq);
    else
        return fmt::format("RECORD {} END", seq);
}

Maybe<TypePtr> RecordType::has_field(const Ident& ident, Context& context) const {
    for (auto& list : seq) {
        auto res = std::find_if(list.list.begin(), list.list.end(), [&ident](auto i) { return i.ident == ident; });
        if (res != list.list.end()) {
            return TypePtr(list.type);
        }
    }
    if (basetype) {
        auto base = context.symbols.get_symbol(context.messages, *basetype);
        if (!base)
            return error;
        else {
            auto baseptr = dynamic_cast<RecordType*>(base->type.get());
            if (!baseptr) {
                context.messages.addErr(basetype->ident.place, "Internal compiler error in RecordType::has_field");
                return error;
            }
            return baseptr->has_field(ident, context);
        }
    }
    context.messages.addErr(place, "Field {} not found in {}", ident, this->to_string());
    return error;
}

bool RecordType::extends(Context& context, const Type& type) const {
    auto typeRes = type.is<RecordType>();
    if (!typeRes) return false;
    auto other = *typeRes;
    if (!basetype) return false;
    auto res = context.symbols.get_symbol(context.messages, *basetype);
    if (!res) throw std::runtime_error("Internal error (RecordType::extends)");
    if (same_types(context, *res->type, other)) return true;
    else {
        auto record = res->type->is<RecordType>();
        if (!record) throw std::runtime_error("Internal error (RecordType::extends)");
        return record->extends(context, type);
    }
}

Maybe<TypePtr> RecordType::normalize(Context& context, bool normalize_pointers) {
    RecordType copy = *this;
    for (auto& list : copy.seq) {
        auto res = list.type->normalize(context, normalize_pointers);
        if (!res)
            return error;
        list.type = *res;
    }
    return make_type<RecordType>(copy);
}

std::string PointerType::to_string() const {
    return fmt::format("POINTER TO {}", type);
}

bool PointerType::check_type(Context& context) {
    auto res = type->normalize(context, false);
    if (!res)
        return berror;
    return bsuccess;
}

const RecordType& PointerType::get_type(Context& context) const {
    TypePtr tmp_type;
    if (auto name = type->is<TypeName>()) {
        auto res = context.symbols.get_symbol(context.messages, name->ident);
        if (!res) throw std::runtime_error("Internal error (PointerType::get_type)");
        tmp_type = res->type;
    } else {
        tmp_type = type;
    }
    auto record = tmp_type->is<RecordType>();
    if (!record) throw std::runtime_error("Internal error (PointerType::get_type)");
    return *record;
}

Maybe<TypePtr> PointerType::normalize(Context& context, bool normalize_pointers) {
    if (!normalize_pointers)
        return make_type<PointerType>(*this);
    PointerType copy = *this;
    auto res = copy.type->normalize(context, false);
    if (!res) return res;
    return make_type<PointerType>(copy);
}

std::string ConstStringType::to_string() const { return fmt::format("String[{}]", size); }

Maybe<TypePtr> ConstStringType::normalize(Context&, bool) {
    return make_type<ConstStringType>(*this);
}

std::string ArrayType::to_string() const {
    if (!open_array)
        return fmt::format("ARRAY {} OF ({})", length, type);
    else {
        return fmt::format("ARRAY OF ({})", type);
    }
}

Maybe<TypePtr> ArrayType::normalize(Context& context, bool normalize_pointers) {
    ArrayType copy = *this;
    auto res = copy.type->normalize(context, normalize_pointers);
    if (!res)
        return res;
    copy.type = *res;
    if (!open_array) {
        auto expr = length->eval(context);
        if (!expr)
            return error;
        auto integer = dynamic_cast<Number*>(expr->get());
        if (!integer) {
            context.messages.addErr(length->place, "Expected number, found {}", length->to_string());
            return error;
        }
        if (!std::holds_alternative<Integer>(integer->value)) {
            context.messages.addErr(length->place, "Expected integer, found real");
            return error;
        }
        length = *expr;
    }
    return make_type<ArrayType>(copy);
}

Maybe<TypePtr> ArrayType::drop_dimensions(size_t count, Context& context) const {
    if (count == 0) return make_type<ArrayType>(*this);
    else if (auto array = type->is<ArrayType>(); !array) {
        if (count == 1) return type;
        else {
            context.messages.addErr(place, "Expected array, found {}", type);
            return error;
        }
    } else {
        return array->drop_dimensions(count - 1, context);
    }
}

ArrayType::ArrayType(std::vector<ExpressionPtr> l, TypePtr t, bool u) : open_array(u) {
    length = l.front();
    if (l.size() > 1) {
        type = make_type<ArrayType>(std::vector(l.begin() + 1, l.end()), t, u);
    } else {
        type = t;
    }
}

//! Matching formal parameters lists http://miasap.se/obnc/type-compatibility.html
bool FormalParameters::match(Context& context, const FormalParameters& other) const {
    if (params.size() != other.params.size()) return false;
    if (!rettype == !other.rettype) return false;
    if (rettype && !same_types(context, **rettype, **other.rettype)) return false;
    for (size_t i = 0; i < params.size(); ++i) {
        if (params[i].var != other.params[i].var) return false;
        if (!equal_types(context, *params[i].type, *other.params[i].type)) return false;
    }
    return true;
}

std::string ProcedureType::to_string() const {
    return fmt::format("PROCEDURE {}", params);
}

Maybe<TypePtr> ProcedureType::normalize(Context& context, bool normalize_pointers) {
    ProcedureType copy = *this;
    for (auto& section : copy.params.params) {
        auto res = section.type->normalize(context, normalize_pointers);
        if (!res)
            return error;
        section.type = *res;
    }
    return make_type<ProcedureType>(copy);
}

ProcedureType::ProcedureType(std::optional<FormalParameters> par) {
    if (par)
        params = *par;
}
