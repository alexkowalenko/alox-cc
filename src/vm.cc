//
// ALOX-CC
//

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <functional>
#include <iostream>

#include <fmt/core.h>
#include <memory>

#include "common.hh"
#include "compiler.hh"
#include "debug.hh"
#include "memory.hh"
#include "object.hh"
#include "value.hh"
#include "vm.hh"

namespace alox {

inline constexpr auto debug_vm{false};
template <typename S, typename... Args>
static void debug(const S &format, const Args &...msg) {
    if constexpr (debug_vm) {
        std::cout << "compiler: " << fmt::format(fmt::runtime(format), msg...) << '\n';
    }
}

constexpr auto DEBUG_TRACE_EXECUTION{true};

void VM::resetStack() {
    stackTop = stack;
    frameCount = 0;
    openUpvalues = nullptr;
}

template <typename... T> void VM::runtimeError(const char *format, const T &...msg) {
    options.err << fmt::format(fmt::runtime(format), msg...); // NOLINT
    options.err << '\n';

    for (int i = frameCount - 1; i >= 0; i--) {
        CallFrame   *frame = &frames[i];
        ObjFunction *function = frame->closure->function;
        const size_t instruction = frame->ip - function->chunk.get_code() - 1;
        options.err << fmt::format("[line {:d}] in ", // [minus]
                                   function->chunk.get_line(instruction));
        if (function->name == nullptr) {
            options.err << "script\n";
        } else {
            options.err << fmt::format("{}()\n", function->name->str);
        }
    }

    resetStack();
    if (errors) {
        errors->hadError = true;
    }
}

// Not necessarily fast with optimised code
// #define push(value)    (*stackTop = (value), stackTop++)
// #define pop()          (stackTop--, *stackTop)
// #define peek(distance) (stackTop[-1 - (distance)])

void VM::init() {
    resetStack();

    initString = newString("init");

    // these are defined before the compiler starts
    def_stdlib();
}

void VM::free() {
    initString = nullptr;
}

bool VM::call(ObjClosure *closure, int argCount) {
    if (argCount != closure->function->arity) {
        runtimeError("Expected {:d} arguments but got {:d}.", closure->function->arity,
                     argCount);
        return false;
    }

    if (frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }

    CallFrame *frame = &frames[frameCount++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.get_code();
    frame->slots = stackTop - argCount - 1;
    return true;
}

bool VM::callValue(Value callee, int argCount) {
    if (is<Obj>(callee)) {
        switch (obj_type(callee)) {
        case OBJ_BOUND_METHOD: {
            ObjBoundMethod *bound = as<ObjBoundMethod *>(callee);
            stackTop[-argCount - 1] = bound->receiver;
            return call(bound->method, argCount);
        }
        case OBJ_CLASS: {
            ObjClass *klass = as<ObjClass *>(callee);
            stackTop[-argCount - 1] = value<Obj *>(newInstance(klass));
            Value initializer;
            if (klass->methods.get(initString, &initializer)) {
                return call(as<ObjClosure *>(initializer), argCount);
            }
            if (argCount != 0) {
                runtimeError("Expected 0 arguments but got {:d}.", argCount);
                return false;
            }
            return true;
        }
        case OBJ_CLOSURE:
            return call(as<ObjClosure *>(callee), argCount);
        case OBJ_NATIVE: {
            NativeFn    native = as<NativeFn>(callee);
            const Value result = native(argCount, stackTop - argCount);
            stackTop -= argCount + 1;
            push(result);
            return true;
        }
        default:
            break; // Non-callable object type.
        }
    }
    runtimeError("Can only call functions and classes.");
    return false;
}

bool VM::invokeFromClass(ObjClass *klass, ObjString *name, int argCount) {
    Value method;
    if (!klass->methods.get(name, &method)) {
        runtimeError("Undefined property '{}'.", name->str);
        return false;
    }
    return call(as<ObjClosure *>(method), argCount);
}

bool VM::invoke(ObjString *name, int argCount) {
    const Value receiver = peek(argCount);

    if (!is<ObjInstance>(receiver)) {
        runtimeError("Only instances have methods.");
        return false;
    }

    ObjInstance *instance = as<ObjInstance *>(receiver);

    Value value;
    if (instance->fields.get(name, &value)) {
        stackTop[-argCount - 1] = value;
        return callValue(value, argCount);
    }

    return invokeFromClass(instance->klass, name, argCount);
}

bool VM::bindMethod(ObjClass *klass, ObjString *name) {
    Value method;
    if (!klass->methods.get(name, &method)) {
        runtimeError("Undefined property '{}'.", name->str);
        return false;
    }

    ObjBoundMethod *bound = newBoundMethod(peek(0), as<ObjClosure *>(method));
    pop();
    push(value<Obj *>(bound));
    return true;
}

ObjUpvalue *VM::captureUpvalue(Value *local) {
    ObjUpvalue *prevUpvalue = nullptr;
    ObjUpvalue *upvalue = openUpvalues;
    while (upvalue != nullptr && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != nullptr && upvalue->location == local) {
        return upvalue;
    }

    ObjUpvalue *createdUpvalue = newUpvalue(local);
    createdUpvalue->next = upvalue;

    if (prevUpvalue == nullptr) {
        openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }

    return createdUpvalue;
}

void VM::closeUpvalues(Value const *last) {
    while (openUpvalues != nullptr && openUpvalues->location >= last) {
        ObjUpvalue *upvalue = openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        openUpvalues = upvalue->next;
    }
}

void VM::defineMethod(ObjString *name) {
    const Value method = peek(0);
    ObjClass   *klass = as<ObjClass *>(peek(1));
    klass->methods.set(name, method);
    pop();
}

void VM::concatenate() {
    ObjString *b = as<ObjString *>(peek(0));
    ObjString *a = as<ObjString *>(peek(1));
    ObjString *result = newString(a->str + b->str);
    pop();
    pop();
    push(value<Obj *>(result));
}

const inline auto number_zero = value<double>(0);
const inline auto number_one = value<double>(1);

InterpretResult VM::run() {
    CallFrame *frame = &frames[frameCount - 1];
    uint8_t   *ip = frame->ip;

#define READ_BYTE() (*ip++)

#define READ_SHORT() (ip += 2, (uint16_t)(ip[-2] << UINT8_WIDTH) | ip[-1])

#define READ_CONSTANT() (frame->closure->function->chunk.get_value(READ_SHORT()))

#define READ_STRING() as<ObjString *>(READ_CONSTANT())
#define BINARY_OP(valueType, op)                                                         \
    do {                                                                                 \
        if (!is<double>(peek(0)) || !is<double>(peek(1))) {                              \
            frame->ip = ip;                                                              \
            runtimeError("Operands must be numbers.");                                   \
            return INTERPRET_RUNTIME_ERROR;                                              \
        }                                                                                \
        const double b = as<double>(pop());                                              \
        const double a = as<double>(pop());                                              \
        push(valueType(a op b));                                                         \
    } while (false)

    for (;;) {
        if constexpr (DEBUG_TRACE_EXECUTION) {
            if (options.trace) {
                std::cout << "          ";
                for (Value *slot = stack; slot < stackTop; slot++) {
                    std::cout << "[ ";
                    printValue(std::cout, *slot);
                    std::cout << " ]";
                }
                std::cout << "\n";
                disassembleInstruction(
                    &frame->closure->function->chunk,
                    (int)(ip - frame->closure->function->chunk.get_code()));
            }
        }

        auto instruction = OpCode(READ_BYTE());
        switch (instruction) {
        case OpCode::CONSTANT: {
            const Value constant = READ_CONSTANT();
            push(constant);
            break;
        }
        case OpCode::NIL:
            push(NIL_VAL);
            break;
        case OpCode::TRUE:
            push(value<bool>(true));
            break;
        case OpCode::FALSE:
            push(value<bool>(false));
            break;
        case OpCode::ZERO:
            push(number_zero);
            break;
        case OpCode::ONE:
            push(number_one);
            break;
        case OpCode::POP:
            pop();
            break;
        case OpCode::GET_LOCAL: {
            const uint8_t slot = READ_BYTE();
            push(frame->slots[slot]);
            break;
        }
        case OpCode::SET_LOCAL: {
            const uint8_t slot = READ_BYTE();
            frame->slots[slot] = peek(0);
            break;
        }
        case OpCode::GET_GLOBAL: {
            ObjString *name = READ_STRING();
            Value      value;
            if (!globals.get(name, &value)) {
                frame->ip = ip;
                runtimeError("Undefined variable '{}'.", name->str);
                return INTERPRET_RUNTIME_ERROR;
            }
            push(value);
            break;
        }
        case OpCode::DEFINE_GLOBAL: {
            ObjString *name = READ_STRING();
            globals.set(name, peek(0));
            pop();
            break;
        }
        case OpCode::SET_GLOBAL: {
            ObjString *name = READ_STRING();
            if (globals.set(name, peek(0))) {
                globals.del(name); // [delete]
                frame->ip = ip;
                runtimeError("Undefined variable '{}'.", name->str);
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OpCode::GET_UPVALUE: {
            const uint8_t slot = READ_BYTE();
            push(*frame->closure->upvalues[slot]->location);
            break;
        }
        case OpCode::SET_UPVALUE: {
            const uint8_t slot = READ_BYTE();
            *frame->closure->upvalues[slot]->location = peek(0);
            break;
        }
        case OpCode::GET_PROPERTY: {
            if (!is<ObjInstance>(peek(0))) {
                frame->ip = ip;
                runtimeError("Only instances have properties.");
                return INTERPRET_RUNTIME_ERROR;
            }

            ObjInstance *instance = as<ObjInstance *>(peek(0));
            ObjString   *name = READ_STRING();

            Value value;
            if (instance->fields.get(name, &value)) {
                pop(); // Instance.
                push(value);
                break;
            }

            if (!bindMethod(instance->klass, name)) {
                frame->ip = ip;
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OpCode::SET_PROPERTY: {
            if (!is<ObjInstance>(peek(1))) {
                frame->ip = ip;
                runtimeError("Only instances have fields.");
                return INTERPRET_RUNTIME_ERROR;
            }

            ObjInstance *instance = as<ObjInstance *>(peek(1));
            instance->fields.set(READ_STRING(), peek(0));
            const Value value = pop();
            pop();
            push(value);
            break;
        }
        case OpCode::GET_SUPER: {
            ObjString *name = READ_STRING();
            ObjClass  *superclass = as<ObjClass *>(pop());

            if (!bindMethod(superclass, name)) {
                frame->ip = ip;
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OpCode::EQUAL: {
            const Value b = pop();
            const Value a = pop();
            push(value<bool>(valuesEqual(a, b)));
            break;
        }
        case OpCode::NOT_EQUAL: {
            const Value b = pop();
            const Value a = pop();
            push(value<bool>(!valuesEqual(a, b)));
            break;
        }
        case OpCode::GREATER:
            BINARY_OP(value<bool>, >);
            break;
        case OpCode::NOT_GREATER:
            BINARY_OP(value<bool>, <=);
            break;
        case OpCode::LESS:
            BINARY_OP(value<bool>, <);
            break;
        case OpCode::NOT_LESS:
            BINARY_OP(value<bool>, >=);
            break;
        case OpCode::ADD: {
            if (is<ObjString>(peek(0)) && is<ObjString>(peek(1))) {
                concatenate();
            } else if (is<double>(peek(0)) && is<double>(peek(1))) {
                double b = as<double>(pop());
                double a = as<double>(pop());
                push(value<double>(a + b));
            } else {
                frame->ip = ip;
                runtimeError("Operands must be two numbers or two strings.");
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OpCode::SUBTRACT:
            BINARY_OP(value<double>, -);
            break;
        case OpCode::MULTIPLY:
            BINARY_OP(value<double>, *);
            break;
        case OpCode::DIVIDE:
            BINARY_OP(value<double>, /);
            break;
        case OpCode::NOT:
            push(value<bool>(isFalsey(pop())));
            break;
        case OpCode::NEGATE:
            if (!is<double>(peek(0))) {
                frame->ip = ip;
                runtimeError("Operand must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            push(value<double>(-as<double>(pop())));
            break;
        case OpCode::PRINT: {
            printValue(options.out, pop());
            std::cout << "\n";
            break;
        }
        case OpCode::JUMP: {
            const uint16_t offset = READ_SHORT();
            ip += offset;
            break;
        }
        case OpCode::JUMP_IF_FALSE: {
            const uint16_t offset = READ_SHORT();
            if (isFalsey(peek(0)))
                ip += offset;
            break;
        }
        case OpCode::LOOP: {
            const uint16_t offset = READ_SHORT();
            ip -= offset;
            break;
        }
        case OpCode::CALL: {
            const int argCount = READ_BYTE();
            frame->ip = ip;
            if (!callValue(peek(argCount), argCount)) {
                frame->ip = ip;
                return INTERPRET_RUNTIME_ERROR;
            }
            frame = &frames[frameCount - 1];
            ip = frame->ip;
            break;
        }
        case OpCode::INVOKE: {
            ObjString *method = READ_STRING();
            const int  argCount = READ_BYTE();
            frame->ip = ip;
            if (!invoke(method, argCount)) {
                frame->ip = ip;
                return INTERPRET_RUNTIME_ERROR;
            }
            frame = &frames[frameCount - 1];
            ip = frame->ip;
            break;
        }
        case OpCode::SUPER_INVOKE: {
            ObjString *method = READ_STRING();
            const int  argCount = READ_BYTE();
            ObjClass  *superclass = as<ObjClass *>(pop());
            frame->ip = ip;
            if (!invokeFromClass(superclass, method, argCount)) {
                frame->ip = ip;
                return INTERPRET_RUNTIME_ERROR;
            }
            frame = &frames[frameCount - 1];
            ip = frame->ip;
            break;
        }
        case OpCode::CLOSURE: {
            ObjFunction *function = as<ObjFunction *>(READ_CONSTANT());
            ObjClosure  *closure = newClosure(function);
            push(value<Obj *>(closure));
            for (int i = 0; i < closure->upvalueCount; i++) {
                const uint8_t isLocal = READ_BYTE();
                const uint8_t index = READ_BYTE();
                if (isLocal) {
                    closure->upvalues[i] = captureUpvalue(frame->slots + index);
                } else {
                    closure->upvalues[i] = frame->closure->upvalues[index];
                }
            }
            break;
        }
        case OpCode::CLOSE_UPVALUE:
            closeUpvalues(stackTop - 1);
            pop();
            break;
        case OpCode::RETURN: {
            const Value result = pop();
            closeUpvalues(frame->slots);
            frameCount--;
            if (frameCount == 0) {
                pop();
                frame->ip = ip;
                return INTERPRET_OK;
            }

            stackTop = frame->slots;
            push(result);
            frame = &frames[frameCount - 1];
            ip = frame->ip;
            break;
        }
        case OpCode::CLASS:
            push(value<Obj *>(newClass(READ_STRING())));
            break;
        case OpCode::INHERIT: {
            const Value superclass = peek(1);
            if (!is<ObjClass>(superclass)) {
                frame->ip = ip;
                runtimeError("Superclass must be a class.");
                return INTERPRET_RUNTIME_ERROR;
            }

            ObjClass *subclass = as<ObjClass *>(peek(0));
            Table::addAll(as<ObjClass *>(superclass)->methods, subclass->methods);
            pop(); // Subclass.
            break;
        }
        case OpCode::METHOD:
            defineMethod(READ_STRING());
            break;
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

InterpretResult VM::run(ObjFunction *function) {

    push(value<Obj *>(function));
    ObjClosure *closure = newClosure(function);
    pop();
    push(value<Obj *>(closure));
    call(closure, 0);

    if (options.debug_code && !options.trace) {
        return INTERPRET_OK;
    }
    return run();
}

} // namespace alox