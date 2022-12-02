//
// ALOX-CC
//

#pragma once

#include "chunk.hh"
#include "context.hh"
#include "object.hh"
#include "options.hh"
#include "parser.hh"
#include "scanner.hh"

enum class Precedence {
    NONE,
    ASSIGNMENT, // =
    OR,         // or
    AND,        // and
    EQUALITY,   // == !=
    COMPARISON, // < > <= >=
    TERM,       // + -
    FACTOR,     // * /
    UNARY,      // ! -
    CALL,       // . ()
    PRIMARY
};

class Compiler;
using ParseFn = std::function<void(Compiler *, bool)>;

struct ParseRule {
    ParseFn    prefix;
    ParseFn    infix;
    Precedence precedence;
};

class Compiler {
  public:
    Compiler(const Options &opt) : options(opt){};
    ~Compiler() = default;

    ObjFunction *compile(const std::string &source);
    void         markCompilerRoots();

    void and_(bool /*canAssign*/);
    void binary(bool /*canAssign*/);
    void call(bool /*canAssign*/);
    void dot(bool canAssign);
    void literal(bool /*canAssign*/);
    void grouping(bool /*canAssign*/);
    void number(bool /*canAssign*/);
    void or_(bool /*canAssign*/);
    void string(bool /*canAssign*/);
    void super_(bool /*canAssign*/);
    void this_(bool /*canAssign*/);
    void unary(bool /*canAssign*/);
    void variable(bool canAssign);

  private:
    Chunk *currentChunk() { return &current->function->chunk; }

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
    void          emitReturn();
    const_index_t makeConstant(Value value);
    void          emitConstant(Value value);
    void          patchJump(int offset);

    void         initCompiler(Context *compiler, FunctionType type);
    ObjFunction *endCompiler();

    void beginScope();
    void endScope();
    void adjust_locals(int depth);

    void                    expression();
    void                    statement();
    void                    declaration();
    static ParseRule const *getRule(TokenType type);
    void                    parsePrecedence(Precedence precedence);

    const_index_t identifierConstant(Token *name);
    static bool   identifiersEqual(Token *a, Token *b);
    int           resolveLocal(Context *compiler, Token *name);
    int           addUpvalue(Context *compiler, uint8_t index, bool isLocal);
    int           resolveUpvalue(Context *compiler, Token *name);
    void          addLocal(Token name);
    void          declareVariable();
    const_index_t parseVariable(const char *errorMessage);
    void          markInitialized();
    void          defineVariable(const_index_t global);

    uint8_t argumentList();

    void namedVariable(Token name, bool canAssign);

    static Token syntheticToken(const char *text);

    void block();
    void function(FunctionType type);
    void method();
    void classDeclaration();
    void funDeclaration();
    void varDeclaration();
    void expressionStatement();
    void forStatement();
    void ifStatement();
    void printStatement();
    void returnStatement();
    void whileStatement();
    void breakStatement(TokenType t);
    void synchronize();

    const Options &options;

    std::unique_ptr<Parser> parser;

    Context      *current{nullptr};
    ClassContext *currentClass{nullptr};
};
