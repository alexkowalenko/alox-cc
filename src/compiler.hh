//
// ALOX-CC
//

#pragma once

#include "ast/binary.hh"
#include "ast/includes.hh"
#include "ast/unary.hh"
#include "ast_base.hh"
#include "chunk.hh"
#include "context.hh"
#include "object.hh"
#include "options.hh"
#include "parser.hh"
#include "scanner.hh"

class Compiler {
  public:
    Compiler(const Options &opt) : options(opt){};
    ~Compiler() = default;

    ObjFunction *compile(Declaration *ast, Parser *source);
    void         markCompilerRoots();

    // Compile the AST
    void declaration(Declaration *ast);
    void statement(Statement *ast);
    void printStatement(Print *ast);
    void exprStatement(Expr *ast);

    void expr(Expr *ast);
    void binary(Binary *ast);
    void unary(Unary *ast);
    void number(Number *ast);

    void and_(bool /*canAssign*/);

    void call(bool /*canAssign*/);
    void dot(bool canAssign);
    void number(bool);
    void literal(bool /*canAssign*/);
    void or_(bool /*canAssign*/);
    void string(bool /*canAssign*/);
    void super_(bool /*canAssign*/);
    void this_(bool /*canAssign*/);
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
    void forStatement();
    void ifStatement();
    void returnStatement();
    void whileStatement();
    void breakStatement(TokenType t);

    const Options &options;
    Parser        *parser{nullptr};

    Context      *current{nullptr};
    ClassContext *currentClass{nullptr};
};
