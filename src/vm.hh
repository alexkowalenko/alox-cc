//
// ALOX-CC
//

#pragma once

#include <memory>

#include "error.hh"
#include "object.hh"
#include "options.hh"
#include "table.hh"
#include "value.hh"

namespace alox {

class Compiler;

constexpr auto FRAMES_MAX = 64;
constexpr auto STACK_MAX = (FRAMES_MAX * UINT8_COUNT);

struct CallFrame {
    ObjClosure *closure;
    uint8_t    *ip;
    Value      *slots;
};

enum InterpretResult {
    INTERPRET_OK,
    INTERPRET_PARSE_ERROR,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
};

class VM {
  public:
    VM(const Options &opt) : options(opt){};
    ~VM() = default;

    void init();
    void free();

    void            set_error_manager(ErrorManager *err) { errors = err; }
    InterpretResult run(ObjFunction *function);

  private:
    void resetStack();

    constexpr void push(const Value value) noexcept {
        *stackTop = value;
        stackTop++;
    }
    constexpr Value pop() noexcept {
        stackTop--;
        return *stackTop;
    }
    [[nodiscard]] constexpr Value peek(const int distance) const noexcept {
        return stackTop[-1 - distance];
    }

    template <typename... T> void runtimeError(const char *format, const T &...msg);

    void def_stdlib();
    void defineNative(const std::string &name, NativeFn function);

    bool        call(ObjClosure *closure, int argCount);
    bool        callValue(Value callee, int argCount);
    bool        invokeFromClass(ObjClass *klass, ObjString *name, int argCount);
    bool        invoke(ObjString *name, int argCount);
    bool        bindMethod(ObjClass *klass, ObjString *name);
    ObjUpvalue *captureUpvalue(Value *local);
    void        closeUpvalues(Value const *last);
    void        defineMethod(ObjString *name);

    static constexpr bool isFalsey(const Value value) noexcept {
        return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
    };
    void            concatenate();
    InterpretResult run();

    int addConstant(Value value);

    const Options &options;
    ErrorManager  *errors;

    CallFrame frames[FRAMES_MAX];
    int       frameCount;

    Value  stack[STACK_MAX];
    Value *stackTop;
    Table  globals;

    ObjString  *initString{nullptr}; // name of LOX class constructor method.
    ObjUpvalue *openUpvalues;

    // std::unique_ptr<Compiler> compiler;
};

} // namespace alox