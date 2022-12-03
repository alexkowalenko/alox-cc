//
// ALOX-CC
//

#include <iostream>

#include <fmt/core.h>

#include "debug.hh"
#include "object.hh"
#include "value.hh"

void disassembleChunk(Chunk *chunk, const std::string_view &name) {
    fmt::print("== {} ==\n", name);

    for (int offset = 0; offset < chunk->get_count();) {
        offset = disassembleInstruction(chunk, offset);
    }
}

static int constantInstruction(const char *name, Chunk *chunk, int offset) {
    auto constant = const_index_t(chunk->get_code(offset + 1) << UINT8_WIDTH);
    constant |= chunk->get_code(offset + 2);
    fmt::print("{:<16}    {:d} '", name, constant);
    printValue(std::cout, chunk->get_value(constant));
    std::cout << "'\n";
    return offset + 3;
}

static int invokeInstruction(const char *name, Chunk *chunk, int offset) {
    auto constant = const_index_t(chunk->get_code(offset + 1) << UINT8_WIDTH);
    constant |= chunk->get_code(offset + 2);
    uint8_t argCount = chunk->get_code(offset + 3);
    fmt::print("{:<16}    ({:d} args) {:4d} '", name, argCount, constant);
    printValue(std::cout, chunk->get_value(constant));
    std::cout << "'\n";
    return offset + 4;
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
    auto jump = (uint16_t)(chunk->get_code(offset + 1) << UINT8_WIDTH);
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

    auto instruction = OpCode(chunk->get_code(offset));
    switch (instruction) {
    case OpCode::CONSTANT:
        return constantInstruction("CONSTANT", chunk, offset);
    case OpCode::NIL:
        return simpleInstruction("NIL", offset);
    case OpCode::TRUE:
        return simpleInstruction("TRUE", offset);
    case OpCode::FALSE:
        return simpleInstruction("FALSE", offset);
    case OpCode::ZERO:
        return simpleInstruction("ZERO", offset);
    case OpCode::ONE:
        return simpleInstruction("ONE", offset);
    case OpCode::POP:
        return simpleInstruction("POP", offset);
    case OpCode::GET_LOCAL:
        return byteInstruction("GET_LOCAL", chunk, offset);
    case OpCode::SET_LOCAL:
        return byteInstruction("SET_LOCAL", chunk, offset);
    case OpCode::GET_GLOBAL:
        return constantInstruction("GET_GLOBAL", chunk, offset);
    case OpCode::DEFINE_GLOBAL:
        return constantInstruction("DEFINE_GLOBAL", chunk, offset);
    case OpCode::SET_GLOBAL:
        return constantInstruction("SET_GLOBAL", chunk, offset);
    case OpCode::GET_UPVALUE:
        return byteInstruction("GET_UPVALUE", chunk, offset);
    case OpCode::SET_UPVALUE:
        return byteInstruction("SET_UPVALUE", chunk, offset);
    case OpCode::GET_PROPERTY:
        return constantInstruction("GET_PROPERTY", chunk, offset);
    case OpCode::SET_PROPERTY:
        return constantInstruction("SET_PROPERTY", chunk, offset);
    case OpCode::GET_SUPER:
        return constantInstruction("GET_SUPER", chunk, offset);
    case OpCode::EQUAL:
        return simpleInstruction("EQUAL", offset);
    case OpCode::GREATER:
        return simpleInstruction("GREATER", offset);
    case OpCode::LESS:
        return simpleInstruction("LESS", offset);
    case OpCode::NOT_EQUAL:
        return simpleInstruction("NOT_EQUAL", offset);
    case OpCode::NOT_GREATER:
        return simpleInstruction("NOT_GREATER", offset);
    case OpCode::NOT_LESS:
        return simpleInstruction("NOT_LESS", offset);
    case OpCode::ADD:
        return simpleInstruction("ADD", offset);
    case OpCode::SUBTRACT:
        return simpleInstruction("SUBTRACT", offset);
    case OpCode::MULTIPLY:
        return simpleInstruction("MULTIPLY", offset);
    case OpCode::DIVIDE:
        return simpleInstruction("DIVIDE", offset);
    case OpCode::NOT:
        return simpleInstruction("NOT", offset);
    case OpCode::NEGATE:
        return simpleInstruction("NEGATE", offset);
    case OpCode::PRINT:
        return simpleInstruction("PRINT", offset);
    case OpCode::JUMP:
        return jumpInstruction("JUMP", 1, chunk, offset);
    case OpCode::JUMP_IF_FALSE:
        return jumpInstruction("JUMP_IF_FALSE", 1, chunk, offset);
    case OpCode::LOOP:
        return jumpInstruction("LOOP", -1, chunk, offset);
    case OpCode::CALL:
        return byteInstruction("CALL", chunk, offset);
    case OpCode::INVOKE:
        return invokeInstruction("INVOKE", chunk, offset);
    case OpCode::SUPER_INVOKE:
        return invokeInstruction("SUPER_INVOKE", chunk, offset);
    case OpCode::CLOSURE: {
        offset++;
        auto constant = const_index_t(chunk->get_code(offset++) << UINT8_WIDTH);
        constant |= chunk->get_code(offset++);
        fmt::print("{:<16} {:4d} ", "CLOSURE", constant);
        printValue(std::cout, chunk->get_value(constant));
        fmt::print("\n");

        ObjFunction *function = AS_FUNCTION(chunk->get_value(constant));
        for (int j = 0; j < function->upvalueCount; j++) {
            const int isLocal = chunk->get_code(offset++);
            int       index = chunk->get_code(offset++);
            fmt::print("{:04d}      |                     {} {:d}\n", offset - 2,
                       isLocal ? "local" : "upvalue", index);
        }

        return offset;
    }
    case OpCode::CLOSE_UPVALUE:
        return simpleInstruction("CLOSE_UPVALUE", offset);
    case OpCode::RETURN:
        return simpleInstruction("RETURN", offset);
    case OpCode::CLASS:
        return constantInstruction("CLASS", chunk, offset);
    case OpCode::INHERIT:
        return simpleInstruction("INHERIT", offset);
    case OpCode::METHOD:
        return constantInstruction("METHOD", chunk, offset);
    default:
        fmt::print("Unknown opcode {:d}\n", uint8_t(instruction));
        return offset + 1;
    }
}
