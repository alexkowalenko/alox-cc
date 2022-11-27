//
// ALOX-CC
//

#pragma once

#include "object.hh"
#include "options.hh"
#include "parser.hh"
#include "scanner.hh"

enum Precedence {
    PREC_NONE,
    PREC_ASSIGNMENT, // =
    PREC_OR,         // or
    PREC_AND,        // and
    PREC_EQUALITY,   // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_TERM,       // + -
    PREC_FACTOR,     // * /
    PREC_UNARY,      // ! -
    PREC_CALL,       // . ()
    PREC_PRIMARY
};

class Lox_Compiler;
using ParseFn = std::__mem_fn<void (Lox_Compiler::*)(bool)>;

struct ParseRule {
    ParseFn    prefix;
    ParseFn    infix;
    Precedence precedence;
};
struct Local {
    Token name;
    int   depth;
    bool  isCaptured;
};

struct Upvalue {
    uint8_t index;
    bool    isLocal;
};

enum FunctionType { TYPE_FUNCTION, TYPE_INITIALIZER, TYPE_METHOD, TYPE_SCRIPT };

/**
 * @brief This has all the info for the current function being compiled.
 *
 */
class Compiler {
  public:
    void init(Compiler *enclosing, FunctionType type);

    Compiler    *enclosing{nullptr};
    ObjFunction *function{nullptr};
    FunctionType type;

    std::array<Local, UINT8_COUNT>   locals;
    int                              localCount{0};
    std::array<Upvalue, UINT8_COUNT> upvalues;
    int                              scopeDepth{0};

    int last_continue{0};
    int last_break{0};
    int enclosing_loop{0};
};

struct ClassCompiler {
    struct ClassCompiler *enclosing;
    bool                  hasSuperclass;
};

class Lox_Compiler {
  public:
    Lox_Compiler(const Options &opt) : options(opt){};
    ~Lox_Compiler() = default;

    ObjFunction *compile(const char *source);
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

    void    emitByte(uint8_t byte);
    void    emitBytes(uint8_t byte1, uint8_t byte2);
    void    emitLoop(int loopStart);
    int     emitJump(uint8_t instruction);
    void    emitReturn();
    uint8_t makeConstant(Value value);
    void    emitConstant(Value value);
    void    patchJump(int offset);

    void         initCompiler(Compiler *compiler, FunctionType type);
    ObjFunction *endCompiler();

    void beginScope();
    void endScope();

    void             expression();
    void             statement();
    void             declaration();
    ParseRule const *getRule(TokenType type);
    void             parsePrecedence(Precedence precedence);

    uint8_t identifierConstant(Token *name);
    bool    identifiersEqual(Token *a, Token *b);
    int     resolveLocal(Compiler *compiler, Token *name);
    int     addUpvalue(Compiler *compiler, uint8_t index, bool isLocal);
    int     resolveUpvalue(Compiler *compiler, Token *name);
    void    addLocal(Token name);
    void    declareVariable();
    uint8_t parseVariable(const char *errorMessage);
    void    markInitialized();
    void    defineVariable(uint8_t global);

    uint8_t argumentList();

    void namedVariable(Token name, bool canAssign);

    Token syntheticToken(const char *text);

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

    Compiler      *current{nullptr};
    ClassCompiler *currentClass{nullptr};
};
