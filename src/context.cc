//
// ALOX-CC
//

#include "context.hh"

void Context::init(Context *enclosing, FunctionType type) {
    // Compiler is created on the stack.
    this->enclosing = enclosing;
    this->type = type;
    this->function = newFunction();
}

BreakContext Context::save_break_context() {
    BreakContext c{last_continue, last_break, last_scope_depth};
    last_scope_depth = scopeDepth;
    return c;
};

void Context::restore_break_context(const BreakContext &context) {
    this->last_continue = std::get<0>(context);
    this->last_break = std::get<1>(context);
    this->last_scope_depth = std::get<2>(context);
}
