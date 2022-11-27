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
#include "vm.hh"

VM vm; // [one]

static Value clockNative(int /*argCount*/, Value * /*args*/) {
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

void VM::resetStack() {
    stackTop = stack;
    frameCount = 0;
    openUpvalues = nullptr;
}

template <typename... T> void VM::runtimeError(const char *format, const T &...msg) {
    std::cerr << fmt::format(fmt::runtime(format), msg...); // NOLINT
    std::cerr << '\n';

    for (int i = frameCount - 1; i >= 0; i--) {
        CallFrame   *frame = &frames[i];
        ObjFunction *function = frame->closure->function;
        size_t       instruction = frame->ip - function->chunk.get_code() - 1;
        std::cerr << fmt::format("[line {:d}] in ", // [minus]
                                 function->chunk.get_line(instruction));
        if (function->name == nullptr) {
            std::cerr << "script\n";
        } else {
            std::cerr << fmt::format("{}()\n", function->name->chars);
        }
    }

    resetStack();
}

void VM::defineNative(const char *name, NativeFn function) {
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    globals.set(AS_STRING(stack[0]), stack[1]);
    pop();
    pop();
}

void VM::init() {
    resetStack();

    globals.init();

    initString = nullptr;
    initString = copyString("init", 4);

    defineNative("clock", clockNative);
}

void VM::free() {
    globals.free();
    initString = nullptr;
    gc.free();
    gc.freeObjects();
}

void VM::markRoots() {
    for (Value *slot = this->stack; slot < this->stackTop; slot++) {
        gc.markValue(*slot);
    }

    for (int i = 0; i < this->frameCount; i++) {
        gc.markObject((Obj *)this->frames[i].closure);
    }

    for (ObjUpvalue *upvalue = this->openUpvalues; upvalue != nullptr;
         upvalue = upvalue->next) {
        gc.markObject((Obj *)upvalue);
    }

    this->globals.mark();
    this->compiler->markCompilerRoots();
    gc.markObject((Obj *)this->initString);
}

void VM::push(Value value) {
    *stackTop = value;
    stackTop++;
}

Value VM::pop() {
    stackTop--;
    return *stackTop;
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
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
        case OBJ_BOUND_METHOD: {
            ObjBoundMethod *bound = AS_BOUND_METHOD(callee);
            stackTop[-argCount - 1] = bound->receiver;
            return call(bound->method, argCount);
        }
        case OBJ_CLASS: {
            ObjClass *klass = AS_CLASS(callee);
            stackTop[-argCount - 1] = OBJ_VAL(newInstance(klass));
            Value initializer;
            if (klass->methods.get(initString, &initializer)) {
                return call(AS_CLOSURE(initializer), argCount);
            }
            if (argCount != 0) {
                runtimeError("Expected 0 arguments but got {:d}.", argCount);
                return false;
            }
            return true;
        }
        case OBJ_CLOSURE:
            return call(AS_CLOSURE(callee), argCount);
        case OBJ_NATIVE: {
            NativeFn native = AS_NATIVE(callee);
            Value    result = native(argCount, stackTop - argCount);
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
        runtimeError("Undefined property '{}'.", name->chars);
        return false;
    }
    return call(AS_CLOSURE(method), argCount);
}

bool VM::invoke(ObjString *name, int argCount) {
    Value receiver = peek(argCount);

    if (!IS_INSTANCE(receiver)) {
        runtimeError("Only instances have methods.");
        return false;
    }

    ObjInstance *instance = AS_INSTANCE(receiver);

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
        runtimeError("Undefined property '{}'.", name->chars);
        return false;
    }

    ObjBoundMethod *bound = newBoundMethod(peek(0), AS_CLOSURE(method));
    pop();
    push(OBJ_VAL(bound));
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

void VM::closeUpvalues(Value *last) {
    while (openUpvalues != nullptr && openUpvalues->location >= last) {
        ObjUpvalue *upvalue = openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        openUpvalues = upvalue->next;
    }
}

void VM::defineMethod(ObjString *name) {
    Value     method = peek(0);
    ObjClass *klass = AS_CLASS(peek(1));
    klass->methods.set(name, method);
    pop();
}

void VM::concatenate() {
    ObjString *b = AS_STRING(peek(0));
    ObjString *a = AS_STRING(peek(1));

    int   length = a->length + b->length;
    char *chars = allocate<char>(length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString *result = takeString(chars, length);
    pop();
    pop();
    push(OBJ_VAL(result));
}

InterpretResult VM::run() {
    CallFrame *frame = &frames[frameCount - 1];

#define READ_BYTE() (*frame->ip++)

#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

#define READ_CONSTANT() (frame->closure->function->chunk.get_value(READ_BYTE()))

#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(valueType, op)                                                         \
    do {                                                                                 \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) {                                \
            runtimeError("Operands must be numbers.");                                   \
            return INTERPRET_RUNTIME_ERROR;                                              \
        }                                                                                \
        double b = AS_NUMBER(pop());                                                     \
        double a = AS_NUMBER(pop());                                                     \
        push(valueType(a op b));                                                         \
    } while (false)

    for (;;) {
        if constexpr (DEBUG_TRACE_EXECUTION) {
            std::cout << "          ";
            for (Value *slot = stack; slot < stackTop; slot++) {
                std::cout << "[ ";
                printValue(*slot);
                std::cout << " ]";
            }
            std::cout << "\n";
            disassembleInstruction(
                &frame->closure->function->chunk,
                (int)(frame->ip - frame->closure->function->chunk.get_code()));
        }

        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
        case OP_CONSTANT: {
            Value constant = READ_CONSTANT();
            push(constant);
            break;
        }
        case OP_NIL:
            push(NIL_VAL);
            break;
        case OP_TRUE:
            push(BOOL_VAL(true));
            break;
        case OP_FALSE:
            push(BOOL_VAL(false));
            break;
        case OP_POP:
            pop();
            break;
        case OP_GET_LOCAL: {
            uint8_t slot = READ_BYTE();
            push(frame->slots[slot]);
            break;
        }
        case OP_SET_LOCAL: {
            uint8_t slot = READ_BYTE();
            frame->slots[slot] = peek(0);
            break;
        }
        case OP_GET_GLOBAL: {
            ObjString *name = READ_STRING();
            Value      value;
            if (!globals.get(name, &value)) {
                runtimeError("Undefined variable '{}'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            push(value);
            break;
        }
        case OP_DEFINE_GLOBAL: {
            ObjString *name = READ_STRING();
            globals.set(name, peek(0));
            pop();
            break;
        }
        case OP_SET_GLOBAL: {
            ObjString *name = READ_STRING();
            if (globals.set(name, peek(0))) {
                globals.del(name); // [delete]
                runtimeError("Undefined variable '{}'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_GET_UPVALUE: {
            uint8_t slot = READ_BYTE();
            push(*frame->closure->upvalues[slot]->location);
            break;
        }
        case OP_SET_UPVALUE: {
            uint8_t slot = READ_BYTE();
            *frame->closure->upvalues[slot]->location = peek(0);
            break;
        }
        case OP_GET_PROPERTY: {
            if (!IS_INSTANCE(peek(0))) {
                runtimeError("Only instances have properties.");
                return INTERPRET_RUNTIME_ERROR;
            }

            ObjInstance *instance = AS_INSTANCE(peek(0));
            ObjString   *name = READ_STRING();

            Value value;
            if (instance->fields.get(name, &value)) {
                pop(); // Instance.
                push(value);
                break;
            }

            if (!bindMethod(instance->klass, name)) {
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_SET_PROPERTY: {
            if (!IS_INSTANCE(peek(1))) {
                runtimeError("Only instances have fields.");
                return INTERPRET_RUNTIME_ERROR;
            }

            ObjInstance *instance = AS_INSTANCE(peek(1));
            instance->fields.set(READ_STRING(), peek(0));
            Value value = pop();
            pop();
            push(value);
            break;
        }
        case OP_GET_SUPER: {
            ObjString *name = READ_STRING();
            ObjClass  *superclass = AS_CLASS(pop());

            if (!bindMethod(superclass, name)) {
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_EQUAL: {
            Value b = pop();
            Value a = pop();
            push(BOOL_VAL(valuesEqual(a, b)));
            break;
        }
        case OP_GREATER:
            BINARY_OP(BOOL_VAL, >);
            break;
        case OP_LESS:
            BINARY_OP(BOOL_VAL, <);
            break;
        case OP_ADD: {
            if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                concatenate();
            } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(a + b));
            } else {
                runtimeError("Operands must be two numbers or two strings.");
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_SUBTRACT:
            BINARY_OP(NUMBER_VAL, -);
            break;
        case OP_MULTIPLY:
            BINARY_OP(NUMBER_VAL, *);
            break;
        case OP_DIVIDE:
            BINARY_OP(NUMBER_VAL, /);
            break;
        case OP_NOT:
            push(BOOL_VAL(isFalsey(pop())));
            break;
        case OP_NEGATE:
            if (!IS_NUMBER(peek(0))) {
                runtimeError("Operand must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            push(NUMBER_VAL(-AS_NUMBER(pop())));
            break;
        case OP_PRINT: {
            printValue(pop());
            std::cout << "\n";
            break;
        }
        case OP_JUMP: {
            uint16_t offset = READ_SHORT();
            frame->ip += offset;
            break;
        }
        case OP_JUMP_IF_FALSE: {
            uint16_t offset = READ_SHORT();
            if (isFalsey(peek(0)))
                frame->ip += offset;
            break;
        }
        case OP_LOOP: {
            uint16_t offset = READ_SHORT();
            frame->ip -= offset;
            break;
        }
        case OP_CALL: {
            int argCount = READ_BYTE();
            if (!callValue(peek(argCount), argCount)) {
                return INTERPRET_RUNTIME_ERROR;
            }
            frame = &frames[frameCount - 1];
            break;
        }
        case OP_INVOKE: {
            ObjString *method = READ_STRING();
            int        argCount = READ_BYTE();
            if (!invoke(method, argCount)) {
                return INTERPRET_RUNTIME_ERROR;
            }
            frame = &frames[frameCount - 1];
            break;
        }
        case OP_SUPER_INVOKE: {
            ObjString *method = READ_STRING();
            int        argCount = READ_BYTE();
            ObjClass  *superclass = AS_CLASS(pop());
            if (!invokeFromClass(superclass, method, argCount)) {
                return INTERPRET_RUNTIME_ERROR;
            }
            frame = &frames[frameCount - 1];
            break;
        }
        case OP_CLOSURE: {
            ObjFunction *function = AS_FUNCTION(READ_CONSTANT());
            ObjClosure  *closure = newClosure(function);
            push(OBJ_VAL(closure));
            for (int i = 0; i < closure->upvalueCount; i++) {
                uint8_t isLocal = READ_BYTE();
                uint8_t index = READ_BYTE();
                if (isLocal) {
                    closure->upvalues[i] = captureUpvalue(frame->slots + index);
                } else {
                    closure->upvalues[i] = frame->closure->upvalues[index];
                }
            }
            break;
        }
        case OP_CLOSE_UPVALUE:
            closeUpvalues(stackTop - 1);
            pop();
            break;
        case OP_RETURN: {
            Value result = pop();
            closeUpvalues(frame->slots);
            frameCount--;
            if (frameCount == 0) {
                pop();
                return INTERPRET_OK;
            }

            stackTop = frame->slots;
            push(result);
            frame = &frames[frameCount - 1];
            break;
        }
        case OP_CLASS:
            push(OBJ_VAL(newClass(READ_STRING())));
            break;
        case OP_INHERIT: {
            Value superclass = peek(1);
            if (!IS_CLASS(superclass)) {
                runtimeError("Superclass must be a class.");
                return INTERPRET_RUNTIME_ERROR;
            }

            ObjClass *subclass = AS_CLASS(peek(0));
            Table::addAll(&AS_CLASS(superclass)->methods, &subclass->methods);
            pop(); // Subclass.
            break;
        }
        case OP_METHOD:
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

InterpretResult VM::interpret(const char *source) {
    compiler = std::make_unique<Lox_Compiler>();
    ObjFunction *function = compiler->compile(source);
    if (function == nullptr) {
        return INTERPRET_COMPILE_ERROR;
    }

    push(OBJ_VAL(function));
    ObjClosure *closure = newClosure(function);
    pop();
    push(OBJ_VAL(closure));
    call(closure, 0);

    auto ret = run();
    return ret;
}
