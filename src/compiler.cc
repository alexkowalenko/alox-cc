//
// ALOX-CC
//

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>

#include <fmt/core.h>

#include "ast/expr.hh"
#include "chunk.hh"
#include "common.hh"
#include "compiler.hh"
#include "debug.hh"
#include "memory.hh"
#include "scanner.hh"

inline constexpr auto debug_compile{false};
template <typename S, typename... Args>
static void debug(const S &format, const Args &...msg) {
    if constexpr (debug_compile) {
        std::cout << "compiler: " << fmt::format(fmt::runtime(format), msg...) << '\n';
    }
}

constexpr auto MAX_ARGS = UINT8_MAX;
constexpr auto MAX_CONSTANTS = UINT16_MAX;

// Code emission

void Compiler::emitByte(uint8_t byte) {
    currentChunk()->write(byte, parser->previous.line);
}

void Compiler::emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

void Compiler::emitByteConst(OpCode byte1, const_index_t c) {
    emitByte(uint8_t(byte1));
    emitByte((c >> UINT8_WIDTH) & 0xff);
    emitByte(c & 0xff);
}

void Compiler::emitLoop(int loopStart) {
    emitByte(OpCode::LOOP);

    auto offset = currentChunk()->get_count() - loopStart + 2;
    if (offset > UINT16_MAX) {
        parser->error("Loop body too large.");
    }

    emitByte((offset >> UINT8_WIDTH) & 0xff);
    emitByte(offset & 0xff);
}

int Compiler::emitJump(OpCode instruction) {
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk()->get_count() - 2;
}

void Compiler::emitReturn() {
    if (current->type == TYPE_INITIALIZER) {
        emitBytes(OpCode::GET_LOCAL, 0);
    } else {
        emitByte(OpCode::NIL);
    }

    emitByte(OpCode::RETURN);
}

const_index_t Compiler::makeConstant(Value value) {
    auto constant = currentChunk()->add_constant(value);
    if (constant > MAX_CONSTANTS) {
        parser->error("Too many constants in one chunk.");
        return 0;
    }
    return constant;
}

void Compiler::emitConstant(Value value) {
    emitByteConst(OpCode::CONSTANT, makeConstant(value));
}

void Compiler::patchJump(int offset) {
    // -2 to adjust for the bytecode for the jump offset itself.
    auto jump = currentChunk()->get_count() - offset - 2;

    if (jump > UINT16_MAX) {
        parser->error("Too much code to jump over.");
    }

    currentChunk()->get_code(offset) = (jump >> UINT8_WIDTH) & 0xff;
    currentChunk()->get_code(offset + 1) = jump & 0xff;
}

// Context manipulation

void Compiler::initCompiler(Context *compiler, FunctionType type) {
    compiler->init(current, type);
    current = compiler;
    if (type != TYPE_SCRIPT) {
        current->function->name = newString(parser->previous.text);
    }

    Local *local = &current->locals[current->localCount++];
    local->depth = 0;
    local->isCaptured = false;
    if (type != TYPE_FUNCTION) {
        local->name.text = "this";
    } else {
        local->name.text = "";
    }
}

ObjFunction *Compiler::endCompiler() {
    emitReturn();
    ObjFunction *function = current->function;

    if (options.debug_code) {
        if (!parser->hadError) {
            disassembleChunk(currentChunk(), function->name != nullptr
                                                 ? function->name->str
                                                 : "<script>");
        }
    }

    current = current->enclosing;
    return function;
}

void Compiler::beginScope() {
    current->scopeDepth++;
}

void Compiler::endScope() {
    current->scopeDepth--;
    adjust_locals(current->scopeDepth);
}

void Compiler::adjust_locals(int depth) {
    debug("adjust locals {}", depth);
    while (current->localCount > 0 &&
           current->locals[current->localCount - 1].depth > depth) {
        debug("pop");
        if (current->locals[current->localCount - 1].isCaptured) {
            emitByte(OpCode::CLOSE_UPVALUE);
        } else {
            emitByte(OpCode::POP);
        }
        current->localCount--;
    }
}

const_index_t Compiler::identifierConstant(Token *name) {
    return makeConstant(OBJ_VAL(newString(name->text)));
}

bool Compiler::identifiersEqual(Token *a, Token *b) {
    if (a->text.size() != b->text.size()) {
        return false;
    }
    return memcmp(a->text.data(), b->text.data(), a->text.size()) == 0;
}

int Compiler::resolveLocal(Context *compiler, Token *name) {
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

int Compiler::addUpvalue(Context *compiler, uint8_t index, bool isLocal) {
    const int upvalueCount = compiler->function->upvalueCount;

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

int Compiler::resolveUpvalue(Context *compiler, Token *name) {
    if (compiler->enclosing == nullptr) {
        return -1;
    }

    const int local = resolveLocal(compiler->enclosing, name);
    if (local != -1) {
        compiler->enclosing->locals[local].isCaptured = true;
        return addUpvalue(compiler, (uint8_t)local, true);
    }

    const int upvalue = resolveUpvalue(compiler->enclosing, name);
    if (upvalue != -1) {
        return addUpvalue(compiler, (uint8_t)upvalue, false);
    }

    return -1;
}

void Compiler::addLocal(Token name) {
    if (current->localCount == UINT8_COUNT) {
        parser->error("Too many local variables in function.");
        return;
    }

    Local *local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
    local->isCaptured = false;
}

void Compiler::declareVariable() {
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

// Compiler functions

ObjFunction *Compiler::compile(Declaration *ast, Parser *p) {
    parser = p;
    Context compiler{};
    initCompiler(&compiler, TYPE_SCRIPT);

    declaration(ast);

    ObjFunction *function = endCompiler();
    return function;
}

void Compiler::declaration(Declaration *ast) {
    debug("declaration");

    for (auto d : ast->stats) {
        // if (parser->match(TokenType::CLASS)) {
        //     classDeclaration();
        // } else if (parser->match(TokenType::FUN)) {
        //     funDeclaration();
        // } else if (parser->match(TokenType::VAR)) {
        //     varDeclaration();
        // } else {
        statement(d);
        // }
    }
}

void Compiler::statement(Statement *ast) {
    debug("statement");
    // if (parser->match(TokenType::PRINT)) {
    //     printStatement();
    // } else if (parser->match(TokenType::FOR)) {
    //     forStatement();
    // } else if (parser->match(TokenType::IF)) {
    //     ifStatement();
    // } else if (parser->match(TokenType::RETURN)) {
    //     returnStatement();
    // } else if (parser->match(TokenType::WHILE)) {
    //     whileStatement();
    // } else if (parser->match(TokenType::BREAK)) {
    //     breakStatement(TokenType::BREAK);
    // } else if (parser->match(TokenType::CONTINUE)) {
    //     breakStatement(TokenType::CONTINUE);
    // } else if (parser->match(TokenType::LEFT_BRACE)) {
    //     beginScope();
    //     block();
    //     endScope();
    // } else {
    expr(AS_Expr(ast->stat));
    //}
}

void Compiler::expr(Expr *ast) {
    debug("expr");
    primary(ast->expr);
    emitByte(OpCode::POP);
}

void Compiler::primary(Primary *ast) {
    number(ast->value);
}

void Compiler::number(Number *ast) {
    if (ast->value == 0) {
        emitByte(OpCode::ZERO);
        return;
    }
    if (ast->value == 1) {
        emitByte(OpCode::ONE);
        return;
    }
    emitConstant(NUMBER_VAL(ast->value));
}

const_index_t Compiler::parseVariable(const char *errorMessage) {
    parser->consume(TokenType::IDENTIFIER, errorMessage);

    declareVariable();
    if (current->scopeDepth > 0) {
        return 0;
    }

    return identifierConstant(&parser->previous);
}

void Compiler::markInitialized() {
    if (current->scopeDepth == 0) {
        return;
    }
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

void Compiler::defineVariable(const_index_t global) {
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }

    emitByteConst(OpCode::DEFINE_GLOBAL, global);
}

uint8_t Compiler::argumentList() {
    uint8_t argCount = 0;
    if (!parser->check(TokenType::RIGHT_PAREN)) {
        do {
            expression();
            if (argCount == MAX_ARGS) {
                parser->error("Can't have more than 255 arguments.");
            }
            argCount++;
        } while (parser->match(TokenType::COMMA));
    }
    parser->consume(TokenType::RIGHT_PAREN, "Expect ')' after arguments.");
    return argCount;
}

void Compiler::and_(bool /*canAssign*/) {
    const int endJump = emitJump(OpCode::JUMP_IF_FALSE);

    emitByte(OpCode::POP);
    parsePrecedence(Precedence::AND);

    patchJump(endJump);
}

void Compiler::binary(bool /*canAssign*/) {
    const TokenType operatorType = parser->previous.type;
    const auto     *rule = getRule(operatorType);
    parsePrecedence((Precedence)(int(rule->precedence) + 1));

    switch (operatorType) {
    case TokenType::BANG_EQUAL:
        emitByte(OpCode::NOT_EQUAL);
        break;
    case TokenType::EQUAL_EQUAL:
        emitByte(OpCode::EQUAL);
        break;
    case TokenType::GREATER:
        emitByte(OpCode::GREATER);
        break;
    case TokenType::GREATER_EQUAL:
        emitByte(OpCode::NOT_LESS);
        break;
    case TokenType::LESS:
        emitByte(OpCode::LESS);
        break;
    case TokenType::LESS_EQUAL:
        emitByte(OpCode::NOT_GREATER);
        break;
    case TokenType::PLUS:
        emitByte(OpCode::ADD);
        break;
    case TokenType::MINUS:
        emitByte(OpCode::SUBTRACT);
        break;
    case TokenType::ASTÉRIX:
        emitByte(OpCode::MULTIPLY);
        break;
    case TokenType::SLASH:
        emitByte(OpCode::DIVIDE);
        break;
    default:
        return; // Unreachable.
    }
}

void Compiler::call(bool /*canAssign*/) {
    const uint8_t argCount = argumentList();
    emitBytes(OpCode::CALL, argCount);
}

void Compiler::dot(bool canAssign) {
    parser->consume(TokenType::IDENTIFIER, "Expect property name after '.'.");
    auto name = identifierConstant(&parser->previous);

    if (canAssign && parser->match(TokenType::EQUAL)) {
        expression();
        emitByteConst(OpCode::SET_PROPERTY, name);
    } else if (parser->match(TokenType::LEFT_PAREN)) {
        const uint8_t argCount = argumentList();
        emitByteConst(OpCode::INVOKE, name);
        emitByte(argCount);
    } else {
        emitByteConst(OpCode::GET_PROPERTY, name);
    }
}

void Compiler::literal(bool /*canAssign*/) {
    switch (parser->previous.type) {
    case TokenType::FALSE:
        emitByte(OpCode::FALSE);
        break;
    case TokenType::NIL:
        emitByte(OpCode::NIL);
        break;
    case TokenType::TRUE:
        emitByte(OpCode::TRUE);
        break;
    default:
        return; // Unreachable.
    }
}

void Compiler::grouping(bool /*canAssign*/) {
    expression();
    parser->consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
}

void Compiler::or_(bool /*canAssign*/) {
    int elseJump = emitJump(OpCode::JUMP_IF_FALSE);
    int endJump = emitJump(OpCode::JUMP);

    patchJump(elseJump);
    emitByte(OpCode::POP);

    parsePrecedence(Precedence::OR);
    patchJump(endJump);
}

void Compiler::string(bool /*canAssign*/) {
    emitConstant(OBJ_VAL(newString(parser->previous.text)));
}

void Compiler::namedVariable(Token name, bool canAssign) {
    OpCode getOp, setOp;
    bool   is_16{false};
    int    arg = resolveLocal(current, &name);
    if (arg != -1) {
        getOp = OpCode::GET_LOCAL;
        setOp = OpCode::SET_LOCAL;
    } else if ((arg = resolveUpvalue(current, &name)) != -1) {
        getOp = OpCode::GET_UPVALUE;
        setOp = OpCode::SET_UPVALUE;
    } else {
        arg = identifierConstant(&name);
        getOp = OpCode::GET_GLOBAL;
        setOp = OpCode::SET_GLOBAL;
        is_16 = true;
    }

    if (canAssign && parser->match(TokenType::EQUAL)) {
        expression();
        if (is_16) {
            emitByteConst(setOp, (const_index_t)arg);
        } else {
            emitBytes(setOp, (uint8_t)arg);
        }
    } else {
        if (is_16) {
            emitByteConst(getOp, (const_index_t)arg);
        } else {
            emitBytes(getOp, (uint8_t)arg);
        }
    }
}

void Compiler::variable(bool canAssign) {
    namedVariable(parser->previous, canAssign);
}

Token Compiler::syntheticToken(const char *text) {
    Token token{};
    token.text = {text, static_cast<size_t>((int)strlen(text))};
    return token;
}

void Compiler::super_(bool /*canAssign*/) {
    if (currentClass == nullptr) {
        parser->error("Can't use 'super' outside of a class.");
    } else if (!currentClass->hasSuperclass) {
        parser->error("Can't use 'super' in a class with no superclass.");
    }

    parser->consume(TokenType::DOT, "Expect '.' after 'super'.");
    parser->consume(TokenType::IDENTIFIER, "Expect superclass method name.");
    auto name = identifierConstant(&parser->previous);

    namedVariable(syntheticToken("this"), false);
    if (parser->match(TokenType::LEFT_PAREN)) {
        const uint8_t argCount = argumentList();
        namedVariable(syntheticToken("super"), false);
        emitByteConst(OpCode::SUPER_INVOKE, name);
        emitByte(argCount);
    } else {
        namedVariable(syntheticToken("super"), false);
        emitByteConst(OpCode::GET_SUPER, name);
    }
}

void Compiler::this_(bool /*canAssign*/) {
    if (currentClass == nullptr) {
        parser->error("Can't use 'this' outside of a class.");
        return;
    }

    variable(false);
} // [this]

void Compiler::unary(bool /*canAssign*/) {
    const TokenType operatorType = parser->previous.type;

    // Compile the operand.
    parsePrecedence(Precedence::UNARY);

    // Emit the operator instruction.
    switch (operatorType) {
    case TokenType::BANG:
        emitByte(OpCode::NOT);
        break;
    case TokenType::MINUS:
        emitByte(OpCode::NEGATE);
        break;
    default:
        return; // Unreachable.
    }
}

inline const std::map<TokenType, ParseRule> rules{
    {TokenType::LEFT_PAREN,
     {std::mem_fn(&Compiler::grouping), std::mem_fn(&Compiler::call), Precedence::CALL}},
    {TokenType::RIGHT_PAREN, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::LEFT_BRACE, {nullptr, nullptr, Precedence::NONE}}, // [big]
    {TokenType::RIGHT_BRACE, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::COMMA, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::DOT, {nullptr, std::mem_fn(&Compiler::dot), Precedence::CALL}},
    {TokenType::MINUS,
     {std::mem_fn(&Compiler::unary), std::mem_fn(&Compiler::binary), Precedence::TERM}},
    {TokenType::PLUS, {nullptr, std::mem_fn(&Compiler::binary), Precedence::TERM}},
    {TokenType::SEMICOLON, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::SLASH, {nullptr, std::mem_fn(&Compiler::binary), Precedence::FACTOR}},
    {TokenType::ASTÉRIX, {nullptr, std::mem_fn(&Compiler::binary), Precedence::FACTOR}},
    {TokenType::BANG, {std::mem_fn(&Compiler::unary), nullptr, Precedence::NONE}},
    {TokenType::BANG_EQUAL,
     {nullptr, std::mem_fn(&Compiler::binary), Precedence::EQUALITY}},
    {TokenType::EQUAL, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::EQUAL_EQUAL,
     {nullptr, std::mem_fn(&Compiler::binary), Precedence::EQUALITY}},
    {TokenType::GREATER,
     {nullptr, std::mem_fn(&Compiler::binary), Precedence::COMPARISON}},
    {TokenType::GREATER_EQUAL,
     {nullptr, std::mem_fn(&Compiler::binary), Precedence::COMPARISON}},
    {TokenType::LESS, {nullptr, std::mem_fn(&Compiler::binary), Precedence::COMPARISON}},
    {TokenType::LESS_EQUAL,
     {nullptr, std::mem_fn(&Compiler::binary), Precedence::COMPARISON}},
    {TokenType::IDENTIFIER,
     {std::mem_fn(&Compiler::variable), nullptr, Precedence::NONE}},
    {TokenType::STRING, {std::mem_fn(&Compiler::string), nullptr, Precedence::NONE}},
    //{TokenType::NUMBER, {std::mem_fn(&Compiler::number), nullptr, Precedence::NONE}},
    {TokenType::AND, {nullptr, std::mem_fn(&Compiler::and_), Precedence::AND}},
    {TokenType::CLASS, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::ELSE, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::FALSE, {std::mem_fn(&Compiler::literal), nullptr, Precedence::NONE}},
    {TokenType::FOR, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::FUN, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::IF, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::NIL, {std::mem_fn(&Compiler::literal), nullptr, Precedence::NONE}},
    {TokenType::OR, {nullptr, std::mem_fn(&Compiler::or_), Precedence::OR}},
    {TokenType::PRINT, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::RETURN, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::SUPER, {std::mem_fn(&Compiler::super_), nullptr, Precedence::NONE}},
    {TokenType::THIS, {std::mem_fn(&Compiler::this_), nullptr, Precedence::NONE}},
    {TokenType::TRUE, {std::mem_fn(&Compiler::literal), nullptr, Precedence::NONE}},
    {TokenType::VAR, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::WHILE, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::ERROR, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::EOFS, {nullptr, nullptr, Precedence::NONE}},
};

void Compiler::parsePrecedence(Precedence precedence) {
    parser->advance();
    auto prefixRule = getRule(parser->previous.type)->prefix;
    if (prefixRule == nullptr) {
        parser->error("Expect expression.");
        return;
    }

    bool canAssign = precedence <= Precedence::ASSIGNMENT;
    prefixRule(this, canAssign);

    while (precedence <= getRule(parser->current.type)->precedence) {
        parser->advance();
        const ParseFn infixRule = getRule(parser->previous.type)->infix;
        infixRule(this, canAssign);
    }

    if (canAssign && parser->match(TokenType::EQUAL)) {
        parser->error("Invalid assignment target.");
    }
}

ParseRule const *Compiler::getRule(TokenType type) {
    return &rules.find(type)->second;
}

void Compiler::expression() {
    parsePrecedence(Precedence::ASSIGNMENT);
}

void Compiler::block() {
    while (!parser->check(TokenType::RIGHT_BRACE) && !parser->check(TokenType::EOFS)) {
        declaration(nullptr);
    }

    parser->consume(TokenType::RIGHT_BRACE, "Expect '}' after block.");
}

void Compiler::function(FunctionType type) {
    Context compiler;
    initCompiler(&compiler, type);
    beginScope(); // [no-end-scope]

    parser->consume(TokenType::LEFT_PAREN, "Expect '(' after function name.");
    if (!parser->check(TokenType::RIGHT_PAREN)) {
        do {
            current->function->arity++;
            if (current->function->arity > MAX_ARGS) {
                parser->errorAtCurrent(
                    fmt::format("Can't have more than {} parameters.", MAX_ARGS));
            }
            const uint8_t constant = parseVariable("Expect parameter name.");
            defineVariable(constant);
        } while (parser->match(TokenType::COMMA));
    }
    parser->consume(TokenType::RIGHT_PAREN, "Expect ')' after parameters.");
    parser->consume(TokenType::LEFT_BRACE, "Expect '{' before function body.");
    block();

    ObjFunction *function = endCompiler();
    emitByteConst(OpCode::CLOSURE, makeConstant(OBJ_VAL(function)));

    for (int i = 0; i < function->upvalueCount; i++) {
        emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
        emitByte(compiler.upvalues[i].index);
    }
}

void Compiler::method() {
    parser->consume(TokenType::IDENTIFIER, "Expect method name.");
    auto constant = identifierConstant(&parser->previous);

    FunctionType type = TYPE_METHOD;
    if (parser->previous.text.size() == 4 &&
        memcmp(parser->previous.text.data(), "init", 4) == 0) {
        type = TYPE_INITIALIZER;
    }

    function(type);
    emitByteConst(OpCode::METHOD, constant);
}

void Compiler::classDeclaration() {
    parser->consume(TokenType::IDENTIFIER, "Expect class name.");
    Token className = parser->previous;
    auto  nameConstant = identifierConstant(&parser->previous);
    declareVariable();

    emitByteConst(OpCode::CLASS, nameConstant);
    defineVariable(nameConstant);

    ClassContext classCompiler{};
    classCompiler.hasSuperclass = false;
    classCompiler.enclosing = currentClass;
    currentClass = &classCompiler;

    if (parser->match(TokenType::LESS)) {
        parser->consume(TokenType::IDENTIFIER, "Expect superclass name.");
        variable(false);

        if (identifiersEqual(&className, &parser->previous)) {
            parser->error("A class can't inherit from itself.");
        }

        beginScope();
        addLocal(syntheticToken("super"));
        defineVariable(0);

        namedVariable(className, false);
        emitByte(OpCode::INHERIT);
        classCompiler.hasSuperclass = true;
    }

    namedVariable(className, false);
    parser->consume(TokenType::LEFT_BRACE, "Expect '{' before class body.");
    while (!parser->check(TokenType::RIGHT_BRACE) && !parser->check(TokenType::EOFS)) {
        method();
    }
    parser->consume(TokenType::RIGHT_BRACE, "Expect '}' after class body.");
    emitByte(OpCode::POP);

    if (classCompiler.hasSuperclass) {
        endScope();
    }

    currentClass = currentClass->enclosing;
}

void Compiler::funDeclaration() {
    const uint8_t global = parseVariable("Expect function name.");
    markInitialized();
    function(TYPE_FUNCTION);
    defineVariable(global);
}

void Compiler::varDeclaration() {
    const uint8_t global = parseVariable("Expect variable name.");

    if (parser->match(TokenType::EQUAL)) {
        expression();
    } else {
        emitByte(OpCode::NIL);
    }
    parser->consume(TokenType::SEMICOLON, "Expect ';' after variable declaration.");

    defineVariable(global);
}

/**
 * @brief forStatement - this is out of order, as there is no AST.
 *
 */
void Compiler::forStatement() {
    debug("forStatement");
    beginScope();

    //  initialiser expression
    debug("forStatement init: {}", currentChunk()->get_count());
    auto context = current->save_break_context();
    parser->consume(TokenType::LEFT_PAREN, "Expect '(' after 'for'.");
    if (parser->match(TokenType::SEMICOLON)) {
        // No initializer.
    } else if (parser->match(TokenType::VAR)) {
        varDeclaration();
    } else {
        expr(nullptr);
    }

    // condition

    auto loopStart = currentChunk()->get_count();
    debug("forStatement condition: {}", currentChunk()->get_count());
    int exitJump = -1;
    if (!parser->match(TokenType::SEMICOLON)) {
        expression();
        parser->consume(TokenType::SEMICOLON, "Expect ';' after loop condition.");

        // Jump out of the loop if the condition is false.
        exitJump = emitJump(OpCode::JUMP_IF_FALSE);
        emitByte(OpCode::POP); // Condition.
    }

    // increment
    debug("forStatement increment: {}", currentChunk()->get_count());
    if (!parser->match(TokenType::RIGHT_PAREN)) {
        const auto bodyJump = emitJump(OpCode::JUMP);
        const auto incrementStart = currentChunk()->get_count();
        current->last_continue = currentChunk()->get_count();
        expression();
        emitByte(OpCode::POP);
        parser->consume(TokenType::RIGHT_PAREN, "Expect ')' after for clauses.");

        emitLoop(loopStart);
        loopStart = incrementStart;
        patchJump(bodyJump);
    }

    // statement
    current->enclosing_loop++;
    statement(nullptr);
    current->enclosing_loop--;
    emitLoop(loopStart);

    if (exitJump != -1) {
        patchJump(exitJump);
        if (current->last_break) {
            patchJump(current->last_break);
            current->last_break = 0;
        }
        emitByte(OpCode::POP); // Condition.
    }

    current->restore_break_context(context);

    endScope();
}

void Compiler::ifStatement() {
    parser->consume(TokenType::LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    parser->consume(TokenType::RIGHT_PAREN,
                    "Expect ')' after condition."); // [paren]

    int thenJump = emitJump(OpCode::JUMP_IF_FALSE);
    emitByte(OpCode::POP);
    statement(nullptr);

    int elseJump = emitJump(OpCode::JUMP);

    patchJump(thenJump);
    emitByte(OpCode::POP);

    if (parser->match(TokenType::ELSE)) {
        statement(nullptr);
    }
    patchJump(elseJump);
}

void Compiler::printStatement() {
    expression();
    parser->consume(TokenType::SEMICOLON, "Expect ';' after value.");
    emitByte(OpCode::PRINT);
}

void Compiler::returnStatement() {
    if (current->type == TYPE_SCRIPT) {
        parser->error("Can't return from top-level code.");
    }

    if (parser->match(TokenType::SEMICOLON)) {
        emitReturn();
    } else {
        if (current->type == TYPE_INITIALIZER) {
            parser->error("Can't return a value from an initializer.");
        }

        expression();
        parser->consume(TokenType::SEMICOLON, "Expect ';' after return value.");
        emitByte(OpCode::RETURN);
    }
}

void Compiler::whileStatement() {
    debug("whileStatement");

    auto context = current->save_break_context();

    const auto loopStart = currentChunk()->get_count();
    current->last_continue = loopStart;

    parser->consume(TokenType::LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    parser->consume(TokenType::RIGHT_PAREN, "Expect ')' after condition.");

    const auto exitJump = emitJump(OpCode::JUMP_IF_FALSE);
    emitByte(OpCode::POP);
    current->enclosing_loop++;
    statement(nullptr);
    current->enclosing_loop--;
    emitLoop(loopStart);

    patchJump(exitJump);
    // handle break
    if (current->last_break) {
        patchJump(current->last_break);
        current->last_break = 0;
    }
    emitByte(OpCode::POP);

    current->restore_break_context(context);
}

void Compiler::breakStatement(TokenType t) {
    debug("breakStatement");
    const auto *name = t == TokenType::BREAK ? "break" : "continue";
    if (current->enclosing_loop == 0) {
        parser->error(fmt::format("{} must be used in a loop.", name));
    }
    parser->consume(TokenType::SEMICOLON, fmt::format("Expect ';' after {}.", name));

    // take off local variables
    if (t == TokenType::BREAK) {
        // take off local variables
        debug("last scope depth {} current {}", current->last_scope_depth,
              current->scopeDepth);
        adjust_locals(current->scopeDepth);
        current->last_break = emitJump(OpCode::JUMP);
        return;
    }
    // continue
    if (current->last_continue) {
        // take off local variables
        debug("last scope depth {} current {}", current->last_scope_depth,
              current->scopeDepth);
        adjust_locals(current->last_scope_depth);

        emitLoop(current->last_continue);
        current->last_continue = 0;
    }
}

void Compiler::markCompilerRoots() {
    Context *compiler = current;
    while (compiler != nullptr) {
        gc.markObject((Obj *)compiler->function);
        compiler = compiler->enclosing;
    }
}
