//
// ALOX-CC
//

#pragma once

#include "common.hh"
#include "val_array.hh"
#include "value.hh"

enum class OpCode {
    CONSTANT,
    NIL,
    TRUE,
    FALSE,
    ZERO,
    ONE,
    POP,
    GET_LOCAL,
    SET_LOCAL,
    GET_GLOBAL,
    DEFINE_GLOBAL,
    SET_GLOBAL,
    GET_UPVALUE,
    SET_UPVALUE,
    GET_PROPERTY,
    SET_PROPERTY,
    GET_SUPER,
    EQUAL,
    GREATER,
    LESS,
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE,
    NOT,
    NEGATE,
    PRINT,
    JUMP,
    JUMP_IF_FALSE,
    LOOP,
    CALL,
    INVOKE,
    SUPER_INVOKE,
    CLOSURE,
    CLOSE_UPVALUE,
    RETURN,
    CLASS,
    INHERIT,
    METHOD
};

class Chunk {
  public:
    Chunk() = default;
    ~Chunk() = default;

    Chunk(const Chunk &) = delete;

    void free();
    void write(uint8_t byte, int line);
    int  addConstant(Value value);

    [[nodiscard]] constexpr size_t get_count() const { return count; }
    [[nodiscard]] constexpr size_t get_line(size_t n) const { return lines[n]; }

    [[nodiscard]] constexpr ValueArray &get_constants() { return constants; }
    [[nodiscard]] constexpr Value &get_value(size_t n) { return constants.get_value(n); }

    constexpr uint8_t               &get_code(size_t n) { return code[n]; };
    [[nodiscard]] constexpr uint8_t *get_code() const { return code; };

  private:
    size_t   count{0};
    size_t   capacity{0};
    uint8_t *code{nullptr};

    std::vector<size_t> lines;
    ValueArray          constants;
};