//
// ALOX-CC
//

#pragma once

#include "object.hh"
#include "scanner.hh"

struct Local {
    std::string name;
    int         depth{};
    bool        isCaptured{false};
};

struct Upvalue {
    uint8_t index;
    bool    isLocal;
};

enum FunctionType { TYPE_FUNCTION, TYPE_INITIALIZER, TYPE_METHOD, TYPE_SCRIPT };

using BreakContext = std::tuple<int, int, int>;

/**
 * @brief This has all the info for the current function being compiled.
 *
 */
class Context {
  public:
    void init(Context *enclosing, FunctionType type);

    BreakContext save_break_context();
    void         restore_break_context(const BreakContext &context);

    Context     *enclosing{nullptr};
    ObjFunction *function{nullptr};
    FunctionType type;

    std::array<Local, UINT8_COUNT>   locals;
    int                              localCount{0};
    std::array<Upvalue, UINT8_COUNT> upvalues{};
    int                              scopeDepth{0};

    size_t last_continue{0};
    size_t last_break{0};
    int    last_scope_depth{0};
    int    enclosing_loop{0};
};

struct ClassContext {
    struct ClassContext *enclosing;
    bool                 hasSuperclass;
};