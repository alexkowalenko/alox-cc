//
// ALOX-CC
//

#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.hh"
#include "compiler.hh"
#include "memory.hh"
#include "parser.hh"
#include "scanner.hh"

#ifdef DEBUG_PRINT_CODE
#include "debug.hh"
#endif

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

using ParseFn = void (*)(bool);

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

struct Compiler {
    struct Compiler *enclosing;
    ObjFunction     *function;
    FunctionType     type;

    Local   locals[UINT8_COUNT];
    int     localCount;
    Upvalue upvalues[UINT8_COUNT];
    int     scopeDepth;
};

struct ClassCompiler {
    struct ClassCompiler *enclosing;
    bool                  hasSuperclass;
};

inline std::unique_ptr<Parser> parser;

inline Compiler      *current{nullptr};
inline ClassCompiler *currentClass{nullptr};

static Chunk *currentChunk() {
    return &current->function->chunk;
}

static void emitByte(uint8_t byte) {
    currentChunk()->write(byte, parser->previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

static void emitLoop(int loopStart) {
    emitByte(OP_LOOP);

    int offset = currentChunk()->get_count() - loopStart + 2;
    if (offset > UINT16_MAX) {
        parser->error("Loop body too large.");
    }

    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
}

static int emitJump(uint8_t instruction) {
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk()->get_count() - 2;
}

static void emitReturn() {
    if (current->type == TYPE_INITIALIZER) {
        emitBytes(OP_GET_LOCAL, 0);
    } else {
        emitByte(OP_NIL);
    }

    emitByte(OP_RETURN);
}

static uint8_t makeConstant(Value value) {
    int constant = currentChunk()->addConstant(value);
    if (constant > UINT8_MAX) {
        parser->error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

static void patchJump(int offset) {
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = currentChunk()->get_count() - offset - 2;

    if (jump > UINT16_MAX) {
        parser->error("Too much code to jump over.");
    }

    currentChunk()->get_code(offset) = (jump >> 8) & 0xff;
    currentChunk()->get_code(offset + 1) = jump & 0xff;
}

static void initCompiler(Compiler *compiler, FunctionType type) {
    compiler->enclosing = current;
    compiler->function = nullptr;
    compiler->type = type;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->function = newFunction();
    current = compiler;
    if (type != TYPE_SCRIPT) {
        current->function->name =
            copyString(parser->previous.start, parser->previous.length);
    }

    Local *local = &current->locals[current->localCount++];
    local->depth = 0;
    local->isCaptured = false;
    if (type != TYPE_FUNCTION) {
        local->name.start = "this";
        local->name.length = 4;
    } else {
        local->name.start = "";
        local->name.length = 0;
    }
}

static ObjFunction *endCompiler() {
    emitReturn();
    ObjFunction *function = current->function;

#ifdef DEBUG_PRINT_CODE
    if (!parser->hadError) {
        disassembleChunk(currentChunk(),
                         function->name != nullptr ? function->name->chars : "<script>");
    }
#endif

    current = current->enclosing;
    return function;
}

static void beginScope() {
    current->scopeDepth++;
}

static void endScope() {
    current->scopeDepth--;

    while (current->localCount > 0 &&
           current->locals[current->localCount - 1].depth > current->scopeDepth) {
        if (current->locals[current->localCount - 1].isCaptured) {
            emitByte(OP_CLOSE_UPVALUE);
        } else {
            emitByte(OP_POP);
        }
        current->localCount--;
    }
}

static void             expression();
static void             statement();
static void             declaration();
static ParseRule const *getRule(TokenType type);
static void             parsePrecedence(Precedence precedence);

static uint8_t identifierConstant(Token *name) {
    return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

static bool identifiersEqual(Token *a, Token *b) {
    if (a->length != b->length)
        return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(Compiler *compiler, Token *name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local *local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1) {
                parser->error("Can't read local variable in its own initializer.");
            }
            return i;
        }
    }

    return -1;
}

static int addUpvalue(Compiler *compiler, uint8_t index, bool isLocal) {
    int upvalueCount = compiler->function->upvalueCount;

    for (int i = 0; i < upvalueCount; i++) {
        Upvalue *upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) {
            return i;
        }
    }

    if (upvalueCount == UINT8_COUNT) {
        parser->error("Too many closure variables in function.");
        return 0;
    }

    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;
    return compiler->function->upvalueCount++;
}

static int resolveUpvalue(Compiler *compiler, Token *name) {
    if (compiler->enclosing == nullptr)
        return -1;

    int local = resolveLocal(compiler->enclosing, name);
    if (local != -1) {
        compiler->enclosing->locals[local].isCaptured = true;
        return addUpvalue(compiler, (uint8_t)local, true);
    }

    int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1) {
        return addUpvalue(compiler, (uint8_t)upvalue, false);
    }

    return -1;
}

static void addLocal(Token name) {
    if (current->localCount == UINT8_COUNT) {
        parser->error("Too many local variables in function.");
        return;
    }

    Local *local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
    local->isCaptured = false;
}

static void declareVariable() {
    if (current->scopeDepth == 0) {
        return;
    }

    Token *name = &parser->previous;
    for (int i = current->localCount - 1; i >= 0; i--) {
        Local *local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break; // [negative]
        }

        if (identifiersEqual(name, &local->name)) {
            parser->error("Already a variable with this name in this scope.");
        }
    }

    addLocal(*name);
}

static uint8_t parseVariable(const char *errorMessage) {
    parser->consume(TOKEN_IDENTIFIER, errorMessage);

    declareVariable();
    if (current->scopeDepth > 0) {
        return 0;
    }

    return identifierConstant(&parser->previous);
}

static void markInitialized() {
    if (current->scopeDepth == 0) {
        return;
    }
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(uint8_t global) {
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }

    emitBytes(OP_DEFINE_GLOBAL, global);
}

static uint8_t argumentList() {
    uint8_t argCount = 0;
    if (!parser->check(TOKEN_RIGHT_PAREN)) {
        do {
            expression();
            if (argCount == 255) {
                parser->error("Can't have more than 255 arguments.");
            }
            argCount++;
        } while (parser->match(TOKEN_COMMA));
    }
    parser->consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return argCount;
}

static void and_(bool /*canAssign*/) {
    int endJump = emitJump(OP_JUMP_IF_FALSE);

    emitByte(OP_POP);
    parsePrecedence(PREC_AND);

    patchJump(endJump);
}

static void binary(bool /*canAssign*/) {
    TokenType operatorType = parser->previous.type;
    auto      rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
    case TOKEN_BANG_EQUAL:
        emitBytes(OP_EQUAL, OP_NOT);
        break;
    case TOKEN_EQUAL_EQUAL:
        emitByte(OP_EQUAL);
        break;
    case TOKEN_GREATER:
        emitByte(OP_GREATER);
        break;
    case TOKEN_GREATER_EQUAL:
        emitBytes(OP_LESS, OP_NOT);
        break;
    case TOKEN_LESS:
        emitByte(OP_LESS);
        break;
    case TOKEN_LESS_EQUAL:
        emitBytes(OP_GREATER, OP_NOT);
        break;
    case TOKEN_PLUS:
        emitByte(OP_ADD);
        break;
    case TOKEN_MINUS:
        emitByte(OP_SUBTRACT);
        break;
    case TOKEN_STAR:
        emitByte(OP_MULTIPLY);
        break;
    case TOKEN_SLASH:
        emitByte(OP_DIVIDE);
        break;
    default:
        return; // Unreachable.
    }
}

static void call(bool /*canAssign*/) {
    uint8_t argCount = argumentList();
    emitBytes(OP_CALL, argCount);
}

static void dot(bool canAssign) {
    parser->consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
    uint8_t name = identifierConstant(&parser->previous);

    if (canAssign && parser->match(TOKEN_EQUAL)) {
        expression();
        emitBytes(OP_SET_PROPERTY, name);
    } else if (parser->match(TOKEN_LEFT_PAREN)) {
        uint8_t argCount = argumentList();
        emitBytes(OP_INVOKE, name);
        emitByte(argCount);
    } else {
        emitBytes(OP_GET_PROPERTY, name);
    }
}

static void literal(bool /*canAssign*/) {
    switch (parser->previous.type) {
    case TOKEN_FALSE:
        emitByte(OP_FALSE);
        break;
    case TOKEN_NIL:
        emitByte(OP_NIL);
        break;
    case TOKEN_TRUE:
        emitByte(OP_TRUE);
        break;
    default:
        return; // Unreachable.
    }
}

static void grouping(bool /*canAssign*/) {
    expression();
    parser->consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(bool /*canAssign*/) {
    double value = strtod(parser->previous.start, nullptr);
    emitConstant(NUMBER_VAL(value));
}

static void or_(bool /*canAssign*/) {
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);

    patchJump(elseJump);
    emitByte(OP_POP);

    parsePrecedence(PREC_OR);
    patchJump(endJump);
}
static void string(bool /*canAssign*/) {
    emitConstant(
        OBJ_VAL(copyString(parser->previous.start + 1, parser->previous.length - 2)));
}

static void namedVariable(Token name, bool canAssign) {
    uint8_t getOp, setOp;
    int     arg = resolveLocal(current, &name);
    if (arg != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else if ((arg = resolveUpvalue(current, &name)) != -1) {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    } else {
        arg = identifierConstant(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    if (canAssign && parser->match(TOKEN_EQUAL)) {
        expression();
        emitBytes(setOp, (uint8_t)arg);
    } else {
        emitBytes(getOp, (uint8_t)arg);
    }
}

static void variable(bool canAssign) {
    namedVariable(parser->previous, canAssign);
}

static Token syntheticToken(const char *text) {
    Token token{};
    token.start = text;
    token.length = (int)strlen(text);
    return token;
}

static void super_(bool /*canAssign*/) {
    if (currentClass == nullptr) {
        parser->error("Can't use 'super' outside of a class.");
    } else if (!currentClass->hasSuperclass) {
        parser->error("Can't use 'super' in a class with no superclass.");
    }

    parser->consume(TOKEN_DOT, "Expect '.' after 'super'.");
    parser->consume(TOKEN_IDENTIFIER, "Expect superclass method name.");
    uint8_t name = identifierConstant(&parser->previous);

    namedVariable(syntheticToken("this"), false);
    if (parser->match(TOKEN_LEFT_PAREN)) {
        uint8_t argCount = argumentList();
        namedVariable(syntheticToken("super"), false);
        emitBytes(OP_SUPER_INVOKE, name);
        emitByte(argCount);
    } else {
        namedVariable(syntheticToken("super"), false);
        emitBytes(OP_GET_SUPER, name);
    }
}

static void this_(bool /*canAssign*/) {
    if (currentClass == nullptr) {
        parser->error("Can't use 'this' outside of a class.");
        return;
    }

    variable(false);
} // [this]

static void unary(bool /*canAssign*/) {
    TokenType operatorType = parser->previous.type;

    // Compile the operand.
    parsePrecedence(PREC_UNARY);

    // Emit the operator instruction.
    switch (operatorType) {
    case TOKEN_BANG:
        emitByte(OP_NOT);
        break;
    case TOKEN_MINUS:
        emitByte(OP_NEGATE);
        break;
    default:
        return; // Unreachable.
    }
}

const ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, call, PREC_CALL},
    [TOKEN_RIGHT_PAREN] = {nullptr, nullptr, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {nullptr, nullptr, PREC_NONE}, // [big]
    [TOKEN_RIGHT_BRACE] = {nullptr, nullptr, PREC_NONE},
    [TOKEN_COMMA] = {nullptr, nullptr, PREC_NONE},
    [TOKEN_DOT] = {nullptr, dot, PREC_CALL},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {nullptr, binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {nullptr, nullptr, PREC_NONE},
    [TOKEN_SLASH] = {nullptr, binary, PREC_FACTOR},
    [TOKEN_STAR] = {nullptr, binary, PREC_FACTOR},
    [TOKEN_BANG] = {unary, nullptr, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {nullptr, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {nullptr, nullptr, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {nullptr, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {nullptr, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {nullptr, binary, PREC_COMPARISON},
    [TOKEN_LESS] = {nullptr, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {nullptr, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {variable, nullptr, PREC_NONE},
    [TOKEN_STRING] = {string, nullptr, PREC_NONE},
    [TOKEN_NUMBER] = {number, nullptr, PREC_NONE},
    [TOKEN_AND] = {nullptr, and_, PREC_AND},
    [TOKEN_CLASS] = {nullptr, nullptr, PREC_NONE},
    [TOKEN_ELSE] = {nullptr, nullptr, PREC_NONE},
    [TOKEN_FALSE] = {literal, nullptr, PREC_NONE},
    [TOKEN_FOR] = {nullptr, nullptr, PREC_NONE},
    [TOKEN_FUN] = {nullptr, nullptr, PREC_NONE},
    [TOKEN_IF] = {nullptr, nullptr, PREC_NONE},
    [TOKEN_NIL] = {literal, nullptr, PREC_NONE},
    [TOKEN_OR] = {nullptr, or_, PREC_OR},
    [TOKEN_PRINT] = {nullptr, nullptr, PREC_NONE},
    [TOKEN_RETURN] = {nullptr, nullptr, PREC_NONE},
    [TOKEN_SUPER] = {super_, nullptr, PREC_NONE},
    [TOKEN_THIS] = {this_, nullptr, PREC_NONE},
    [TOKEN_TRUE] = {literal, nullptr, PREC_NONE},
    [TOKEN_VAR] = {nullptr, nullptr, PREC_NONE},
    [TOKEN_WHILE] = {nullptr, nullptr, PREC_NONE},
    [TOKEN_ERROR] = {nullptr, nullptr, PREC_NONE},
    [TOKEN_EOF] = {nullptr, nullptr, PREC_NONE},
};

static void parsePrecedence(Precedence precedence) {
    parser->advance();
    ParseFn prefixRule = getRule(parser->previous.type)->prefix;
    if (prefixRule == nullptr) {
        parser->error("Expect expression.");
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while (precedence <= getRule(parser->current.type)->precedence) {
        parser->advance();
        ParseFn infixRule = getRule(parser->previous.type)->infix;
        infixRule(canAssign);
    }

    if (canAssign && parser->match(TOKEN_EQUAL)) {
        parser->error("Invalid assignment target.");
    }
}

static ParseRule const *getRule(TokenType type) {
    return &rules[type];
}

static void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

static void block() {
    while (!parser->check(TOKEN_RIGHT_BRACE) && !parser->check(TOKEN_EOF)) {
        declaration();
    }

    parser->consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void function(FunctionType type) {
    Compiler compiler;
    initCompiler(&compiler, type);
    beginScope(); // [no-end-scope]

    parser->consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!parser->check(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255) {
                parser->errorAtCurrent("Can't have more than 255 parameters.");
            }
            uint8_t constant = parseVariable("Expect parameter name.");
            defineVariable(constant);
        } while (parser->match(TOKEN_COMMA));
    }
    parser->consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    parser->consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block();

    ObjFunction *function = endCompiler();
    emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));

    for (int i = 0; i < function->upvalueCount; i++) {
        emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
        emitByte(compiler.upvalues[i].index);
    }
}

static void method() {
    parser->consume(TOKEN_IDENTIFIER, "Expect method name.");
    uint8_t constant = identifierConstant(&parser->previous);

    FunctionType type = TYPE_METHOD;
    if (parser->previous.length == 4 && memcmp(parser->previous.start, "init", 4) == 0) {
        type = TYPE_INITIALIZER;
    }

    function(type);
    emitBytes(OP_METHOD, constant);
}

static void classDeclaration() {
    parser->consume(TOKEN_IDENTIFIER, "Expect class name.");
    Token   className = parser->previous;
    uint8_t nameConstant = identifierConstant(&parser->previous);
    declareVariable();

    emitBytes(OP_CLASS, nameConstant);
    defineVariable(nameConstant);

    ClassCompiler classCompiler;
    classCompiler.hasSuperclass = false;
    classCompiler.enclosing = currentClass;
    currentClass = &classCompiler;

    if (parser->match(TOKEN_LESS)) {
        parser->consume(TOKEN_IDENTIFIER, "Expect superclass name.");
        variable(false);

        if (identifiersEqual(&className, &parser->previous)) {
            parser->error("A class can't inherit from itself.");
        }

        beginScope();
        addLocal(syntheticToken("super"));
        defineVariable(0);

        namedVariable(className, false);
        emitByte(OP_INHERIT);
        classCompiler.hasSuperclass = true;
    }

    namedVariable(className, false);
    parser->consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
    while (!parser->check(TOKEN_RIGHT_BRACE) && !parser->check(TOKEN_EOF)) {
        method();
    }
    parser->consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
    emitByte(OP_POP);

    if (classCompiler.hasSuperclass) {
        endScope();
    }

    currentClass = currentClass->enclosing;
}

static void funDeclaration() {
    uint8_t global = parseVariable("Expect function name.");
    markInitialized();
    function(TYPE_FUNCTION);
    defineVariable(global);
}

static void varDeclaration() {
    uint8_t global = parseVariable("Expect variable name.");

    if (parser->match(TOKEN_EQUAL)) {
        expression();
    } else {
        emitByte(OP_NIL);
    }
    parser->consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    defineVariable(global);
}

static void expressionStatement() {
    expression();
    parser->consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}

static void forStatement() {
    beginScope();
    parser->consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    if (parser->match(TOKEN_SEMICOLON)) {
        // No initializer.
    } else if (parser->match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        expressionStatement();
    }

    int loopStart = currentChunk()->get_count();
    int exitJump = -1;
    if (!parser->match(TOKEN_SEMICOLON)) {
        expression();
        parser->consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        // Jump out of the loop if the condition is false.
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP); // Condition.
    }

    if (!parser->match(TOKEN_RIGHT_PAREN)) {
        int bodyJump = emitJump(OP_JUMP);
        int incrementStart = currentChunk()->get_count();
        expression();
        emitByte(OP_POP);
        parser->consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        emitLoop(loopStart);
        loopStart = incrementStart;
        patchJump(bodyJump);
    }

    statement();
    emitLoop(loopStart);

    if (exitJump != -1) {
        patchJump(exitJump);
        emitByte(OP_POP); // Condition.
    }

    endScope();
}

static void ifStatement() {
    parser->consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    parser->consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition."); // [paren]

    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();

    int elseJump = emitJump(OP_JUMP);

    patchJump(thenJump);
    emitByte(OP_POP);

    if (parser->match(TOKEN_ELSE)) {
        statement();
    }
    patchJump(elseJump);
}

static void printStatement() {
    expression();
    parser->consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
}

static void returnStatement() {
    if (current->type == TYPE_SCRIPT) {
        parser->error("Can't return from top-level code.");
    }

    if (parser->match(TOKEN_SEMICOLON)) {
        emitReturn();
    } else {
        if (current->type == TYPE_INITIALIZER) {
            parser->error("Can't return a value from an initializer.");
        }

        expression();
        parser->consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
        emitByte(OP_RETURN);
    }
}

static void whileStatement() {
    int loopStart = currentChunk()->get_count();
    parser->consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    parser->consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();
    emitLoop(loopStart);

    patchJump(exitJump);
    emitByte(OP_POP);
}

static void synchronize() {
    parser->panicMode = false;

    while (parser->current.type != TOKEN_EOF) {
        if (parser->previous.type == TOKEN_SEMICOLON) {
            return;
        }
        switch (parser->current.type) {
        case TOKEN_CLASS:
        case TOKEN_FUN:
        case TOKEN_VAR:
        case TOKEN_FOR:
        case TOKEN_IF:
        case TOKEN_WHILE:
        case TOKEN_PRINT:
        case TOKEN_RETURN:
            return;

        default:; // Do nothing.
        }

        parser->advance();
    }
}

static void declaration() {
    if (parser->match(TOKEN_CLASS)) {
        classDeclaration();
    } else if (parser->match(TOKEN_FUN)) {
        funDeclaration();
    } else if (parser->match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement();
    }

    if (parser->panicMode) {
        synchronize();
    }
}

static void statement() {
    if (parser->match(TOKEN_PRINT)) {
        printStatement();
    } else if (parser->match(TOKEN_FOR)) {
        forStatement();
    } else if (parser->match(TOKEN_IF)) {
        ifStatement();
    } else if (parser->match(TOKEN_RETURN)) {
        returnStatement();
    } else if (parser->match(TOKEN_WHILE)) {
        whileStatement();
    } else if (parser->match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    } else {
        expressionStatement();
    }
}

ObjFunction *compile(const char *source) {
    auto scanner = std::make_unique<Scanner>(source);
    parser = std::make_unique<Parser>(scanner);
    Compiler compiler{};
    initCompiler(&compiler, TYPE_SCRIPT);

    parser->advance();

    while (!parser->match(TOKEN_EOF)) {
        declaration();
    }

    ObjFunction *function = endCompiler();
    return parser->hadError ? nullptr : function;
}

void markCompilerRoots() {
    Compiler *compiler = current;
    while (compiler != nullptr) {
        markObject((Obj *)compiler->function);
        compiler = compiler->enclosing;
    }
}
