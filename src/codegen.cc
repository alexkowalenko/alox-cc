//
// ALOX-CC
//

#include "codegen.hh"

constexpr auto MAX_CONSTANTS = UINT16_MAX;

void CodeGen::emitByte(uint8_t byte) {
    cur->write(byte, linenumber);
}

void CodeGen::emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

void CodeGen::emitByteConst(OpCode byte1, const_index_t c) {
    emitByte(uint8_t(byte1));
    emitByte((c >> UINT8_WIDTH) & 0xff);
    emitByte(c & 0xff);
}

void CodeGen::emitLoop(int loopStart) {
    emitByte(OpCode::LOOP);

    auto offset = cur->get_count() - loopStart + 2;
    if (offset > UINT16_MAX) {
        err.errorAt(cur->line_last(), "Loop body too large.");
    }

    emitByte((offset >> UINT8_WIDTH) & 0xff);
    emitByte(offset & 0xff);
}

int CodeGen::emitJump(OpCode instruction) {
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return cur->get_count() - 2;
}

void CodeGen::emitReturn(FunctionType type) {
    if (type == TYPE_INITIALIZER) {
        emitBytes(OpCode::GET_LOCAL, 0);
    } else {
        emitByte(OpCode::NIL);
    }

    emitByte(OpCode::RETURN);
}

const_index_t CodeGen::makeConstant(Value value) {
    auto constant = cur->add_constant(value);
    if (constant > MAX_CONSTANTS) {
        err.errorAt(linenumber, "Too many constants in one chunk.");
        return 0;
    }
    return constant;
}

void CodeGen::emitConstant(Value value) {
    emitByteConst(OpCode::CONSTANT, makeConstant(value));
}

void CodeGen::patchJump(size_t offset) {
    // -2 to adjust for the bytecode for the jump offset itself.
    auto jump = cur->get_count() - offset - 2;

    if (jump > UINT16_MAX) {
        err.errorAt(linenumber, "Too much code to jump over.");
    }

    cur->get_code(offset) = (jump >> UINT8_WIDTH) & 0xff;
    cur->get_code(offset + 1) = jump & 0xff;
}