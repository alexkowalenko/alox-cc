//
// ALOX-CC
//

#include <iostream>

#include <fmt/core.h>

#include "debug.hh"
#include "object.hh"
#include "value.hh"

void disassembleChunk(Chunk *chunk, const char *name) {
    fmt::print("== {} ==\n", name);

    for (int offset = 0; offset < chunk->get_count();) {
        offset = disassembleInstruction(chunk, offset);
    }
}

static int constantInstruction(const char *name, Chunk *chunk, int offset) {
    uint8_t constant = chunk->get_code(offset + 1);
    fmt::print("{:<16}    {:d} '", name, constant);
    printValue(chunk->get_value(constant));
    std::cout << "'\n";
    return offset + 2;
}

static int invokeInstruction(const char *name, Chunk *chunk, int offset) {
    uint8_t constant = chunk->get_code(offset + 1);
    uint8_t argCount = chunk->get_code(offset + 2);
    fmt::print("{:<16}    ({:d} args) {:4d} '", name, argCount, constant);
    printValue(chunk->get_value(constant));
    std::cout << "'\n";
    return offset + 3;
}

static int simpleInstruction(const char *name, int offset) {
    fmt::print("{}\n", name);
    return offset + 1;
}

static int byteInstruction(const char *name, Chunk *chunk, int offset) {
    uint8_t slot = chunk->get_code(offset + 1);
    fmt::print("{:<16} {:4d}\n", name, slot);
    return offset + 2; // [debug]
}

static int jumpInstruction(const char *name, int sign, Chunk *chunk, int offset) {
    auto jump = (uint16_t)(chunk->get_code(offset + 1) << 8);
    jump |= chunk->get_code(offset + 2);
    fmt::print("{:<16}   {:4d} -> {:d}\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

int disassembleInstruction(Chunk *chunk, int offset) {
    fmt::print("{:04d} ", offset);
    if (offset > 0 && chunk->get_line(offset) == chunk->get_line(offset - 1)) {
        fmt::print("   | ");
    } else {
        fmt::print("{:04d} ", chunk->get_line(offset));
    }

    uint8_t instruction = chunk->get_code(offset);
    switch (instruction) {
    case OP_CONSTANT:
        return constantInstruction("OP_CONSTANT", chunk, offset);
    case OP_NIL:
        return simpleInstruction("OP_NIL", offset);
    case OP_TRUE:
        return simpleInstruction("OP_TRUE", offset);
    case OP_FALSE:
        return simpleInstruction("OP_FALSE", offset);
    case OP_POP:
        return simpleInstruction("OP_POP", offset);
    case OP_GET_LOCAL:
        return byteInstruction("OP_GET_LOCAL", chunk, offset);
    case OP_SET_LOCAL:
        return byteInstruction("OP_SET_LOCAL", chunk, offset);
    case OP_GET_GLOBAL:
        return constantInstruction("OP_GET_GLOBAL", chunk, offset);
    case OP_DEFINE_GLOBAL:
        return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
    case OP_SET_GLOBAL:
        return constantInstruction("OP_SET_GLOBAL", chunk, offset);
    case OP_GET_UPVALUE:
        return byteInstruction("OP_GET_UPVALUE", chunk, offset);
    case OP_SET_UPVALUE:
        return byteInstruction("OP_SET_UPVALUE", chunk, offset);
    case OP_GET_PROPERTY:
        return constantInstruction("OP_GET_PROPERTY", chunk, offset);
    case OP_SET_PROPERTY:
        return constantInstruction("OP_SET_PROPERTY", chunk, offset);
    case OP_GET_SUPER:
        return constantInstruction("OP_GET_SUPER", chunk, offset);
    case OP_EQUAL:
        return simpleInstruction("OP_EQUAL", offset);
    case OP_GREATER:
        return simpleInstruction("OP_GREATER", offset);
    case OP_LESS:
        return simpleInstruction("OP_LESS", offset);
    case OP_ADD:
        return simpleInstruction("OP_ADD", offset);
    case OP_SUBTRACT:
        return simpleInstruction("OP_SUBTRACT", offset);
    case OP_MULTIPLY:
        return simpleInstruction("OP_MULTIPLY", offset);
    case OP_DIVIDE:
        return simpleInstruction("OP_DIVIDE", offset);
    case OP_NOT:
        return simpleInstruction("OP_NOT", offset);
    case OP_NEGATE:
        return simpleInstruction("OP_NEGATE", offset);
    case OP_PRINT:
        return simpleInstruction("OP_PRINT", offset);
    case OP_JUMP:
        return jumpInstruction("OP_JUMP", 1, chunk, offset);
    case OP_JUMP_IF_FALSE:
        return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
    case OP_LOOP:
        return jumpInstruction("OP_LOOP", -1, chunk, offset);
    case OP_CALL:
        return byteInstruction("OP_CALL", chunk, offset);
    case OP_INVOKE:
        return invokeInstruction("OP_INVOKE", chunk, offset);
    case OP_SUPER_INVOKE:
        return invokeInstruction("OP_SUPER_INVOKE", chunk, offset);
    case OP_CLOSURE: {
        offset++;
        uint8_t constant = chunk->get_code(offset++);
        fmt::print("{:<16} {:4d} ", "OP_CLOSURE", constant);
        printValue(chunk->get_value(constant));
        fmt::print("\n");

        ObjFunction *function = AS_FUNCTION(chunk->get_value(constant));
        for (int j = 0; j < function->upvalueCount; j++) {
            int isLocal = chunk->get_code(offset++);
            int index = chunk->get_code(offset++);
            fmt::print("{:04d}      |                     {} {:d}\n", offset - 2,
                       isLocal ? "local" : "upvalue", index);
        }

        return offset;
    }
    case OP_CLOSE_UPVALUE:
        return simpleInstruction("OP_CLOSE_UPVALUE", offset);
    case OP_RETURN:
        return simpleInstruction("OP_RETURN", offset);
    case OP_CLASS:
        return constantInstruction("OP_CLASS", chunk, offset);
    case OP_INHERIT:
        return simpleInstruction("OP_INHERIT", offset);
    case OP_METHOD:
        return constantInstruction("OP_METHOD", chunk, offset);
    default:
        fmt::print("Unknown opcode {:d}\n", instruction);
        return offset + 1;
    }
}
