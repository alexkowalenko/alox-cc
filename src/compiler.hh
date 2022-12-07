//
// ALOX-CC
//

#pragma once

#include "ast/assign.hh"
#include "ast/binary.hh"
#include "ast/block.hh"
#include "ast/boolean.hh"
#include "ast/break.hh"
#include "ast/includes.hh"
#include "ast/return.hh"
#include "ast/unary.hh"
#include "ast/vardec.hh"
#include "ast_base.hh"
#include "chunk.hh"
#include "context.hh"
#include "object.hh"
#include "options.hh"
#include "parser.hh"
#include "scanner.hh"
#include <string_view>

class Compiler {
  public:
    Compiler(const Options &opt, Error &err) : options(opt), err(err){};
    ~Compiler() = default;

    ObjFunction *compile(Declaration *ast, Parser *source);
    void         markCompilerRoots();

  private:
    // Compile the AST
    void declaration(Declaration *ast);
    void decs_statement(Obj *);
    void varDeclaration(VarDec *ast);

    void statement(Statement *ast);
    void ifStatement(If *ast);
    void forStatement(For *ast);
    void whileStatement(While *ast);
    void printStatement(Print *ast);
    void returnStatement(Return *ast);
    void breakStatement(Break *ast);
    void block(Block *);
    void exprStatement(Expr *ast);

    void expr(Expr *ast, bool canAssign = false);
    void binary(Binary *ast, bool canAssign);
    void assign(Assign *ast);
    void and_(Binary *ast, bool canAssign);
    void or_(Binary *ast, bool canAssign);
    void unary(Unary *ast, bool canAssign);
    void variable(Identifier *ast, bool canAssign);
    void number(Number *ast);
    void string(String *ast);
    void boolean(Boolean *ast);

    void call(bool /*canAssign*/);
    void dot(bool canAssign);

    void super_(bool /*canAssign*/);
    void this_(bool /*canAssign*/);

    Chunk         *currentChunk() { return &current->function->chunk; }
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

    const_index_t parseVariable(const std::string &var);
    void          declareVariable(const std::string &name);
    void          addLocal(const std::string &name);
    const_index_t identifierConstant(const std::string &name);
    void          defineVariable(const_index_t global);
    void          markInitialized();
    void          beginScope();
    void          endScope();
    void          namedVariable(const std::string &name, bool canAssign);
    void          adjust_locals(int depth);
    int           resolveLocal(Context *compiler, const std::string &name);
    int           addUpvalue(Context *compiler, uint8_t index, bool isLocal);
    int           resolveUpvalue(Context *compiler, const std::string &name);

    uint8_t argumentList();

    void function(FunctionType type);
    void method();
    void classDeclaration();
    void funDeclaration();

    void error(size_t line, const std::string_view &);

    const Options &options;
    Error         &err;
    Parser        *parser{nullptr};

    Context      *current{nullptr};
    ClassContext *currentClass{nullptr};
};
