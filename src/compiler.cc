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

constexpr auto sym_this = "this";
constexpr auto sym_super = "super";

// Code emission

void Compiler::emitByte(uint8_t byte) {
    currentChunk()->write(byte, currentChunk()->line_last());
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
        err.errorAt(currentChunk()->line_last(), "Loop body too large.");
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
        err.errorAt(currentChunk()->line_last(), "Too many constants in one chunk.");
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
        err.errorAt(currentChunk()->line_last(), "Too much code to jump over.");
    }

    currentChunk()->get_code(offset) = (jump >> UINT8_WIDTH) & 0xff;
    currentChunk()->get_code(offset + 1) = jump & 0xff;
}

// Context manipulation

void Compiler::initCompiler(Context *compiler, const std::string &name,
                            FunctionType type) {
    compiler->init(current, type);
    current = compiler;
    if (type != TYPE_SCRIPT) {
        current->function->name = newString(name);
    }

    Local *local = &current->locals[current->localCount++];
    local->depth = 0;
    local->isCaptured = false;
    if (type != TYPE_FUNCTION) {
        local->name = "this";
    } else {
        local->name = "";
    }
}

ObjFunction *Compiler::endCompiler() {
    emitReturn();
    ObjFunction *function = current->function;

    if (options.debug_code) {
        if (!err.hadError) {
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

int Compiler::resolveLocal(Context *compiler, const std::string &name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local *local = &compiler->locals[i];
        if (name == local->name) {
            if (local->depth == -1) {
                err.errorAt(currentChunk()->line_last(),
                            "Can't read local variable in its own initializer.");
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
        err.errorAt(currentChunk()->line_last(),
                    "Too many closure variables in function.");
        return 0;
    }

    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;
    return compiler->function->upvalueCount++;
}

int Compiler::resolveUpvalue(Context *compiler, const std::string &name) {
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

void Compiler::addLocal(const std::string &name) {
    if (current->localCount == UINT8_COUNT) {
        err.errorAt(currentChunk()->line_last(), "Too many local variables in function.");
        return;
    }

    Local *local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
    local->isCaptured = false;
}

void Compiler::declareVariable(const std::string &name) {
    if (current->scopeDepth == 0) {
        return;
    }

    for (int i = current->localCount - 1; i >= 0; i--) {
        Local *local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break; // [negative]
        }

        if (name == local->name) {
            err.errorAt(currentChunk()->line_last(),
                        "Already a variable with this name in this scope.");
        }
    }
    addLocal(name);
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

const_index_t Compiler::parseVariable(const std::string &var) {
    declareVariable(var);
    if (current->scopeDepth > 0) {
        return 0;
    }
    return identifierConstant(var);
}

const_index_t Compiler::identifierConstant(const std::string &name) {
    return makeConstant(OBJ_VAL(newString(name)));
}

void Compiler::namedVariable(const std::string &name, bool canAssign) {
    OpCode getOp, setOp;
    bool   is_16{false};
    int    arg = resolveLocal(current, name);
    if (arg != -1) {
        getOp = OpCode::GET_LOCAL;
        setOp = OpCode::SET_LOCAL;
    } else if ((arg = resolveUpvalue(current, name)) != -1) {
        getOp = OpCode::GET_UPVALUE;
        setOp = OpCode::SET_UPVALUE;
    } else {
        arg = identifierConstant(name);
        getOp = OpCode::GET_GLOBAL;
        setOp = OpCode::SET_GLOBAL;
        is_16 = true;
    }

    if (canAssign) {
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

void Compiler::function(FunctDec *ast, FunctionType type) {
    Context compiler;
    initCompiler(&compiler, ast->name->name->str, type);
    beginScope();

    for (auto p : ast->parameters) {
        current->function->arity++;
        const uint8_t constant = parseVariable(p->name->str);
        defineVariable(constant);
    }
    block(ast->body);

    ObjFunction *function = endCompiler();
    emitByteConst(OpCode::CLOSURE, makeConstant(OBJ_VAL(function)));

    for (int i = 0; i < function->upvalueCount; i++) {
        emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
        emitByte(compiler.upvalues[i].index);
    }
}

uint8_t Compiler::argumentList(const std::vector<Expr *> &args) {
    for (auto *arg : args) {
        expr(arg);
    }
    return args.size();
}

void Compiler::method(FunctDec *ast) {
    auto         constant = identifierConstant(ast->name->name->str);
    FunctionType type = TYPE_METHOD;
    if (ast->name->name->str == "init") {
        type = TYPE_INITIALIZER;
    }

    function(ast, type);
    emitByteConst(OpCode::METHOD, constant);
}

void Compiler::super_(This *ast, bool /*canAssign*/) {
    if (currentClass == nullptr) {
        error(ast->line, "Can't use 'super' outside of a class.");
    } else if (!currentClass->hasSuperclass) {
        error(ast->line, "Can't use 'super' in a class with no superclass.");
    }

    auto name = identifierConstant(ast->id);
    namedVariable(sym_this, false);
    if (ast->token == TokenType::LEFT_PAREN) {
        const uint8_t argCount = argumentList(ast->args);
        namedVariable(sym_super, false);
        emitByteConst(OpCode::SUPER_INVOKE, name);
        emitByte(argCount);
    } else {
        namedVariable(sym_super, false);
        emitByteConst(OpCode::GET_SUPER, name);
    }
}

void Compiler::this_(This *ast, bool canAssign) {
    if (ast->token == TokenType::SUPER) {
        super_(ast, canAssign);
        return;
    }

    if (currentClass == nullptr) {
        error(ast->line, "Can't use 'this' outside of a class.");
        return;
    }
    namedVariable(sym_this, false); // variable(sym_this, false);
}

// Compiler functions

ObjFunction *Compiler::compile(Declaration *ast) {
    Context compiler{};
    initCompiler(&compiler, "script>", TYPE_SCRIPT);

    declaration(ast);

    ObjFunction *function = endCompiler();
    return function;
}

void Compiler::declaration(Declaration *ast) {
    debug("declaration");

    for (auto *d : ast->stats) {
        decs_statement(d);
    }
}

void Compiler::decs_statement(Obj *s) {
    if (IS_ClassDec(s)) {
        classDeclaration(AS_ClassDec(s));
    } else if (IS_FunctDec(s)) {
        funDeclaration(AS_FunctDec(s));
    } else if (IS_VarDec(s)) {
        varDeclaration(AS_VarDec(s));
    } else {
        statement(AS_Statement(s));
    }
}

void Compiler::varDeclaration(VarDec *ast) {
    const uint8_t global = parseVariable(ast->var->name->str);
    if (ast->expr) {
        expr(ast->expr);
    } else {
        emitByte(OpCode::NIL);
    }
    defineVariable(global);
}

void Compiler::funDeclaration(FunctDec *ast) {
    const uint8_t global = parseVariable(ast->name->name->str);
    markInitialized();
    function(ast, TYPE_FUNCTION);
    defineVariable(global);
}

void Compiler::classDeclaration(ClassDec *ast) {
    auto nameConstant = identifierConstant(ast->name);
    declareVariable(ast->name); // replace

    emitByteConst(OpCode::CLASS, nameConstant);
    defineVariable(nameConstant);

    ClassContext classCompiler{};
    classCompiler.hasSuperclass = false;
    classCompiler.enclosing = currentClass;
    currentClass = &classCompiler;

    if (!ast->super.empty()) {
        namedVariable(ast->super, false); // variable(nullptr, false);

        beginScope();
        addLocal(sym_super);
        defineVariable(0);

        namedVariable(ast->name, false);
        emitByte(OpCode::INHERIT);
        classCompiler.hasSuperclass = true;
    }

    namedVariable(ast->name, false);
    for (auto *m : ast->methods) {
        method(m);
    }
    emitByte(OpCode::POP);

    if (classCompiler.hasSuperclass) {
        endScope();
    }

    currentClass = currentClass->enclosing;
}

void Compiler::statement(Statement *ast) {
    debug("statement");
    if (IS_Print(ast->stat)) {
        printStatement(AS_Print(ast->stat));
    } else if (IS_For(ast->stat)) {
        forStatement(AS_For(ast->stat));
    } else if (IS_If(ast->stat)) {
        ifStatement(AS_If(ast->stat));
    } else if (IS_Return(ast->stat)) {
        returnStatement(AS_Return(ast->stat));
    } else if (IS_While(ast->stat)) {
        whileStatement(AS_While(ast->stat));
    } else if (IS_Break(ast->stat)) {
        breakStatement(AS_Break(ast->stat));
    } else if (IS_Block(ast->stat)) {
        beginScope();
        block(AS_Block(ast->stat));
        endScope();
    } else {
        expr(AS_Expr(ast->stat));
    }
}

void Compiler::ifStatement(If *ast) {
    expr(ast->cond);
    int thenJump = emitJump(OpCode::JUMP_IF_FALSE);
    emitByte(OpCode::POP);
    statement(ast->then_stat);

    int elseJump = emitJump(OpCode::JUMP);
    patchJump(thenJump);
    emitByte(OpCode::POP);

    if (ast->else_stat) {
        statement(ast->else_stat);
    }
    patchJump(elseJump);
}

void Compiler::whileStatement(While *ast) {
    debug("whileStatement");

    auto context = current->save_break_context();

    const auto loopStart = currentChunk()->get_count();
    current->last_continue = loopStart;

    expr(ast->cond);

    const auto exitJump = emitJump(OpCode::JUMP_IF_FALSE);
    emitByte(OpCode::POP);
    current->enclosing_loop++;
    statement(ast->body);
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

/**
 * @brief forStatement - this is out of order, as originally there was no AST.
 *
 */
void Compiler::forStatement(For *ast) {
    debug("forStatement");
    beginScope();

    //  initialiser expression
    debug("forStatement init: {}", currentChunk()->get_count());
    auto context = current->save_break_context();
    if (ast->init) {
        // No initializer.
        if (IS_VarDec(ast->init)) {
            varDeclaration(AS_VarDec(ast->init));
        } else {
            expr(AS_Expr(ast->init));
        }
    }

    // condition
    auto loopStart = currentChunk()->get_count();
    debug("forStatement condition: {}", currentChunk()->get_count());
    int exitJump = -1;
    if (ast->cond) {
        expr(ast->cond);

        // Jump out of the loop if the condition is false.
        exitJump = emitJump(OpCode::JUMP_IF_FALSE);
        emitByte(OpCode::POP); // Condition.
    }

    // increment
    debug("forStatement increment: {}", currentChunk()->get_count());
    if (ast->iter) {
        const auto bodyJump = emitJump(OpCode::JUMP);
        const auto incrementStart = currentChunk()->get_count();
        current->last_continue = currentChunk()->get_count();
        expr(ast->iter);
        emitByte(OpCode::POP);

        emitLoop(loopStart);
        loopStart = incrementStart;
        patchJump(bodyJump);
    }

    // statement
    current->enclosing_loop++;
    statement(ast->body);
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

void Compiler::returnStatement(Return *ast) {
    if (current->type == TYPE_SCRIPT) {
        error(ast->line, "Can't return from top-level code.");
    }
    if (!ast->expr) {
        emitReturn();
    } else {
        if (current->type == TYPE_INITIALIZER) {
            error(ast->line, "Can't return a value from an initializer.");
        }
        expr(ast->expr);
        emitByte(OpCode::RETURN);
    }
}

void Compiler::breakStatement(Break *ast) {
    debug("breakStatement");
    const auto *name = ast->tok == TokenType::BREAK ? "break" : "continue";
    if (current->enclosing_loop == 0) {
        error(ast->line, fmt::format("{} must be used in a loop.", name));
    }

    // take off local variables
    if (ast->tok == TokenType::BREAK) {
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

void Compiler::block(Block *ast) {
    for (auto *s : ast->stats) {
        decs_statement(s);
    }
}

void Compiler::printStatement(Print *ast) {
    expr(ast->expr);
    emitByte(OpCode::PRINT);
}

void Compiler::exprStatement(Expr *ast) {
    expr(ast);
    emitByte(OpCode::POP);
}

void Compiler::expr(Expr *ast, bool canAssign) {
    debug("expr");
    if (IS_Expr(ast->expr)) {
        expr(AS_Expr(ast->expr), canAssign);
    } else if (IS_Assign(ast->expr)) {
        assign(AS_Assign(ast->expr));
    } else if (IS_Unary(ast->expr)) {
        unary(AS_Unary(ast->expr), canAssign);
    } else if (IS_Binary(ast->expr)) {
        binary(AS_Binary(ast->expr), canAssign);
    } else if (IS_Call(ast->expr)) {
        call(AS_Call(ast->expr));
    } else if (IS_Dot(ast->expr)) {
        dot(AS_Dot(ast->expr), canAssign);
    } else if (IS_Number(ast->expr)) {
        number(AS_Number(ast->expr));
    } else if (IS_Identifier(ast->expr)) {
        variable(AS_Identifier(ast->expr), canAssign);
    } else if (IS_String(ast->expr)) {
        string(AS_String(ast->expr));
    } else if (IS_Boolean(ast->expr)) {
        boolean(AS_Boolean(ast->expr));
    } else if (IS_This(ast->expr)) {
        this_(AS_This(ast->expr), canAssign);
    } else if (IS_Nil(ast->expr)) {
        emitByte(OpCode::NIL);
    }
}

void Compiler::assign(Assign *ast) {
    expr(ast->right, false);
    expr(ast->left, true);
}

void Compiler::call(Call *ast) {
    expr(ast->fname, false);
    const uint8_t argCount = argumentList(ast->args);
    emitBytes(OpCode::CALL, argCount);
}

void Compiler::binary(Binary *ast, bool canAssign) {
    switch (ast->token) {
    case TokenType::AND:
        and_(ast, canAssign);
        return;
    case TokenType::OR:
        or_(ast, canAssign);
        return;
    default:;
    }

    expr(ast->left, canAssign);
    expr(ast->right, canAssign);

    switch (ast->token) {
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

void Compiler::and_(Binary *ast, bool canAssign) {
    expr(ast->left, canAssign);
    const int endJump = emitJump(OpCode::JUMP_IF_FALSE);
    emitByte(OpCode::POP);

    expr(ast->right, canAssign);
    patchJump(endJump);
}

void Compiler::or_(Binary *ast, bool canAssign) {
    expr(ast->left, canAssign);
    int elseJump = emitJump(OpCode::JUMP_IF_FALSE);
    int endJump = emitJump(OpCode::JUMP);

    patchJump(elseJump);
    emitByte(OpCode::POP);

    expr(ast->right, canAssign);
    patchJump(endJump);
}

void Compiler::dot(Dot *ast, bool canAssign) {
    expr(ast->left, canAssign);
    auto name = identifierConstant(ast->id);
    if (ast->token == TokenType::EQUAL) {
        expr(ast->args[0]);
        emitByteConst(OpCode::SET_PROPERTY, name);
    } else if (ast->token == TokenType::LEFT_PAREN) {
        const uint8_t argCount = argumentList(ast->args);
        emitByteConst(OpCode::INVOKE, name);
        emitByte(argCount);
    } else {
        emitByteConst(OpCode::GET_PROPERTY, name);
    }
}

void Compiler::unary(Unary *ast, bool canAssign) {
    expr(ast->expr, canAssign);

    // Emit the operator instruction.
    switch (ast->token) {
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

void Compiler::variable(Identifier *ast, bool canAssign) {
    namedVariable(ast->name->str, canAssign);
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

void Compiler::string(String *ast) {
    emitConstant(OBJ_VAL(ast->value));
}

void Compiler::boolean(Boolean *ast) {
    if (ast->value) {
        emitByte(OpCode::TRUE);
    } else {
        emitByte(OpCode::FALSE);
    }
}

/////////////////////////////////////////////////////////////////////////

void Compiler::markCompilerRoots() {
    Context *compiler = current;
    while (compiler != nullptr) {
        gc.markObject((Obj *)compiler->function);
        compiler = compiler->enclosing;
    }
}

void Compiler::error(size_t line, const std::string_view &message) {
    err.errorAt(line, message);
}