//
// ALOX-CC
//

#pragma once

#include "chunk.hh"
#include "context.hh"
#include "error.hh"
#include <cstddef>

namespace alox {

class CodeGen {
  public:
    CodeGen(ErrorManager &e) : err(e){};

    void           emitByte(uint8_t byte);
    constexpr void emitByte(OpCode byte) { return emitByte(uint8_t(byte)); };
    void           emitBytes(uint8_t byte1, uint8_t byte2);
    void           emitByteConst(OpCode byte1, const_index_t c);
    constexpr void emitBytes(OpCode byte1, OpCode byte2) {
        return emitBytes(uint8_t(byte1), uint8_t(byte2));
    }
    constexpr void emitBytes(OpCode byte1, uint8_t byte2) {
        return emitBytes(uint8_t(byte1), byte2);
    }
    void          emitLoop(int loopStart);
    int           emitJump(OpCode instruction);
    void          emitReturn(FunctionType type);
    const_index_t makeConstant(Value value);
    void          emitConstant(Value value);
    void          patchJump(size_t offset);

    void set_chunk(Chunk *c) { cur = c; };

    void                 set_linenumber(size_t l) { linenumber = l; }
    [[nodiscard]] size_t get_linenumber() const { return linenumber; }

    size_t get_position() { return cur->get_count(); }

  private:
    Chunk        *cur;
    size_t        linenumber{0};
    ErrorManager &err;
};

} // namespace lox