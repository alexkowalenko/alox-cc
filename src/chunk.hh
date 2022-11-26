//
// ALOX-CC
//

#pragma once

#include "common.hh"
#include "val_array.hh"
#include "value.hh"

enum OpCode {
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    OP_GET_PROPERTY,
    OP_SET_PROPERTY,
    OP_GET_SUPER,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_PRINT,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_CALL,
    OP_INVOKE,
    OP_SUPER_INVOKE,
    OP_CLOSURE,
    OP_CLOSE_UPVALUE,
    OP_RETURN,
    OP_CLASS,
    OP_INHERIT,
    OP_METHOD
};

class Chunk {
  public:
    void init();
    void free();
    void write(uint8_t byte, int line);
    int  addConstant(Value value);

    [[nodiscard]] constexpr int get_count() const { return count; }
    [[nodiscard]] constexpr int get_line(size_t n) const { return lines[n]; }

    [[nodiscard]] constexpr ValueArray &get_constants() { return constants; }
    [[nodiscard]] constexpr Value &get_value(size_t n) { return constants.get_value(n); }

    constexpr uint8_t               &get_code(size_t n) { return code[n]; };
    [[nodiscard]] constexpr uint8_t *get_code() const { return code; };

  private:
    int      count;
    int      capacity;
    int     *lines;
    uint8_t *code;

    ValueArray constants;
};