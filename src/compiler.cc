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

namespace alox {

inline constexpr auto debug_compile{false};
template <typename S, typename... Args>
static void debug(const S &format, const Args &...msg) {
    if constexpr (debug_compile) {
        std::cout << "compiler: " << fmt::format(fmt::runtime(format), msg...) << '\n';
    }
}

constexpr auto MAX_ARGS = UINT8_MAX;

constexpr auto sym_this = "this";
constexpr auto sym_super = "super";

// Context manipulation

void Compiler::initCompiler(Context *compiler, const std::string &name,
                            FunctionType type) {
    compiler->init(current, type);
    current = compiler;
    gen.set_chunk(&current->function->chunk);
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
    gen.emitReturn(current->type);
    ObjFunction *function = current->function;

    if (options.debug_code) {
        if (!err.hadError) {
            disassembleChunk(&current->function->chunk, function->name != nullptr
                                                            ? function->name->str
                                                            : "<script>");
        }
    }
    current = current->enclosing;
    if (current) {
        gen.set_chunk(&current->function->chunk);
    }
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
            gen.emitByte(OpCode::CLOSE_UPVALUE);
        } else {
            gen.emitByte(OpCode::POP);
        }
        current->localCount--;
    }
}

int Compiler::resolveLocal(Context *compiler, const std::string &name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local *local = &compiler->locals[i];
        if (name == local->name) {
            if (local->depth == -1) {
                err.errorAt(gen.get_linenumber(),
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
        err.errorAt(gen.get_linenumber(), "Too many closure variables in function.");
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
        err.errorAt(gen.get_linenumber(), "Too many local variables in function.");
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
            err.errorAt(gen.get_linenumber(),
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
    gen.emitByteConst(OpCode::DEFINE_GLOBAL, global);
}

const_index_t Compiler::parseVariable(const std::string &var) {
    declareVariable(var);
    if (current->scopeDepth > 0) {
        return 0;
    }
    return identifierConstant(var);
}

const_index_t Compiler::identifierConstant(const std::string &name) {
    return gen.makeConstant(OBJ_VAL(newString(name)));
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
            gen.emitByteConst(setOp, (const_index_t)arg);
        } else {
            gen.emitBytes(setOp, (uint8_t)arg);
        }
    } else {
        if (is_16) {
            gen.emitByteConst(getOp, (const_index_t)arg);
        } else {
            gen.emitBytes(getOp, (uint8_t)arg);
        }
    }
}

void Compiler::function(FunctDec *ast, FunctionType type) {
    Context compiler;
    initCompiler(&compiler, ast->name->name, type);
    beginScope();

    for (auto p : ast->parameters) {
        current->function->arity++;
        const uint8_t constant = parseVariable(p->name);
        defineVariable(constant);
    }
    block(ast->body);

    ObjFunction *function = endCompiler();
    gen.emitByteConst(OpCode::CLOSURE, gen.makeConstant(OBJ_VAL(function)));

    for (int i = 0; i < function->upvalueCount; i++) {
        gen.emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
        gen.emitByte(compiler.upvalues[i].index);
    }
}

uint8_t Compiler::argumentList(const std::vector<Expr *> &args) {
    for (auto *arg : args) {
        expr(arg);
    }
    return args.size();
}

void Compiler::method(FunctDec *ast) {
    auto         constant = identifierConstant(ast->name->name);
    FunctionType type = TYPE_METHOD;
    if (ast->name->name == "init") {
        type = TYPE_INITIALIZER;
    }

    function(ast, type);
    gen.emitByteConst(OpCode::METHOD, constant);
}

void Compiler::super_(This *ast, bool /*canAssign*/) {
    if (currentClass == nullptr) {
        error(ast->line, "Can't use 'super' outside of a class.");
    } else if (!currentClass->hasSuperclass) {
        error(ast->line, "Can't use 'super' in a class with no superclass.");
    }

    auto name = identifierConstant(ast->id);
    namedVariable(sym_this, false);
    if (ast->has_args) {
        const uint8_t argCount = argumentList(ast->args);
        namedVariable(sym_super, false);
        gen.emitByteConst(OpCode::SUPER_INVOKE, name);
        gen.emitByte(argCount);
    } else {
        namedVariable(sym_super, false);
        gen.emitByteConst(OpCode::GET_SUPER, name);
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
    gen.set_linenumber(ast->line);

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
    gen.set_linenumber(ast->line);
    const uint8_t global = parseVariable(ast->var->name);
    if (ast->expr) {
        expr(ast->expr);
    } else {
        gen.emitByte(OpCode::NIL);
    }
    defineVariable(global);
}

void Compiler::funDeclaration(FunctDec *ast) {
    gen.set_linenumber(ast->line);
    const uint8_t global = parseVariable(ast->name->name);
    markInitialized();
    function(ast, TYPE_FUNCTION);
    defineVariable(global);
}

void Compiler::classDeclaration(ClassDec *ast) {
    gen.set_linenumber(ast->line);
    auto nameConstant = identifierConstant(ast->name);
    declareVariable(ast->name); // replace

    gen.emitByteConst(OpCode::CLASS, nameConstant);
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
        gen.emitByte(OpCode::INHERIT);
        classCompiler.hasSuperclass = true;
    }

    namedVariable(ast->name, false);
    for (auto *m : ast->methods) {
        method(m);
    }
    gen.emitByte(OpCode::POP);

    if (classCompiler.hasSuperclass) {
        endScope();
    }

    currentClass = currentClass->enclosing;
}

void Compiler::statement(Statement *ast) {
    gen.set_linenumber(ast->line);
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
        exprStatement(AS_Expr(ast->stat));
    }
}

void Compiler::ifStatement(If *ast) {
    expr(ast->cond);
    int thenJump = gen.emitJump(OpCode::JUMP_IF_FALSE);
    gen.emitByte(OpCode::POP);
    statement(ast->then_stat);

    int elseJump = gen.emitJump(OpCode::JUMP);
    gen.patchJump(thenJump);
    gen.emitByte(OpCode::POP);

    if (ast->else_stat) {
        statement(ast->else_stat);
    }
    gen.patchJump(elseJump);
}

void Compiler::whileStatement(While *ast) {
    debug("whileStatement");

    auto context = current->save_break_context();

    const auto loopStart = gen.get_position();
    current->last_continue = loopStart;

    expr(ast->cond);

    const auto exitJump = gen.emitJump(OpCode::JUMP_IF_FALSE);
    gen.emitByte(OpCode::POP);
    current->enclosing_loop++;
    statement(ast->body);
    current->enclosing_loop--;
    gen.emitLoop(loopStart);

    gen.patchJump(exitJump);
    // handle break
    if (current->last_break) {
        gen.patchJump(current->last_break);
        current->last_break = 0;
    }
    gen.emitByte(OpCode::POP);

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
    debug("forStatement init: {}", gen.get_position());
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
    auto loopStart = gen.get_position();
    debug("forStatement condition: {}", gen.get_position());
    int exitJump = -1;
    if (ast->cond) {
        expr(ast->cond);

        // Jump out of the loop if the condition is false.
        exitJump = gen.emitJump(OpCode::JUMP_IF_FALSE);
        gen.emitByte(OpCode::POP); // Condition.
    }

    // increment
    debug("forStatement increment: {}", gen.get_position());
    if (ast->iter) {
        const auto bodyJump = gen.emitJump(OpCode::JUMP);
        const auto incrementStart = gen.get_position();
        current->last_continue = gen.get_position();
        expr(ast->iter);
        gen.emitByte(OpCode::POP);

        gen.emitLoop(loopStart);
        loopStart = incrementStart;
        gen.patchJump(bodyJump);
    }

    // statement
    current->enclosing_loop++;
    statement(ast->body);
    current->enclosing_loop--;
    gen.emitLoop(loopStart);

    if (exitJump != -1) {
        gen.patchJump(exitJump);
        if (current->last_break) {
            gen.patchJump(current->last_break);
            current->last_break = 0;
        }
        gen.emitByte(OpCode::POP); // Condition.
    }

    current->restore_break_context(context);
    endScope();
}

void Compiler::returnStatement(Return *ast) {
    if (current->type == TYPE_SCRIPT) {
        error(ast->line, "Can't return from top-level code.");
    }
    if (!ast->expr) {
        gen.emitReturn(current->type);
    } else {
        if (current->type == TYPE_INITIALIZER) {
            error(ast->line, "Can't return a value from an initializer.");
        }
        expr(ast->expr);
        gen.emitByte(OpCode::RETURN);
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
        current->last_break = gen.emitJump(OpCode::JUMP);
        return;
    }
    // continue
    if (current->last_continue) {
        // take off local variables
        debug("last scope depth {} current {}", current->last_scope_depth,
              current->scopeDepth);
        adjust_locals(current->last_scope_depth);

        gen.emitLoop(current->last_continue);
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
    gen.emitByte(OpCode::PRINT);
}

void Compiler::exprStatement(Expr *ast) {
    expr(ast);
    gen.emitByte(OpCode::POP);
}

void Compiler::expr(Expr *ast, bool canAssign) {
    gen.set_linenumber(ast->line);
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
        gen.emitByte(OpCode::NIL);
    }
}

void Compiler::assign(Assign *ast) {
    gen.set_linenumber(ast->line);
    expr(ast->right, false);
    expr(ast->left, true);
}

void Compiler::call(Call *ast) {
    expr(ast->fname, false);
    const uint8_t argCount = argumentList(ast->args);
    gen.emitBytes(OpCode::CALL, argCount);
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
        gen.emitByte(OpCode::NOT_EQUAL);
        break;
    case TokenType::EQUAL_EQUAL:
        gen.emitByte(OpCode::EQUAL);
        break;
    case TokenType::GREATER:
        gen.emitByte(OpCode::GREATER);
        break;
    case TokenType::GREATER_EQUAL:
        gen.emitByte(OpCode::NOT_LESS);
        break;
    case TokenType::LESS:
        gen.emitByte(OpCode::LESS);
        break;
    case TokenType::LESS_EQUAL:
        gen.emitByte(OpCode::NOT_GREATER);
        break;
    case TokenType::PLUS:
        gen.emitByte(OpCode::ADD);
        break;
    case TokenType::MINUS:
        gen.emitByte(OpCode::SUBTRACT);
        break;
    case TokenType::ASTÉRIX:
        gen.emitByte(OpCode::MULTIPLY);
        break;
    case TokenType::SLASH:
        gen.emitByte(OpCode::DIVIDE);
        break;
    default:
        return; // Unreachable.
    }
}

void Compiler::and_(Binary *ast, bool canAssign) {
    expr(ast->left, canAssign);
    const int endJump = gen.emitJump(OpCode::JUMP_IF_FALSE);
    gen.emitByte(OpCode::POP);

    expr(ast->right, canAssign);
    gen.patchJump(endJump);
}

void Compiler::or_(Binary *ast, bool canAssign) {
    expr(ast->left, canAssign);
    int elseJump = gen.emitJump(OpCode::JUMP_IF_FALSE);
    int endJump = gen.emitJump(OpCode::JUMP);

    gen.patchJump(elseJump);
    gen.emitByte(OpCode::POP);

    expr(ast->right, canAssign);
    gen.patchJump(endJump);
}

void Compiler::dot(Dot *ast, bool canAssign) {
    expr(ast->left, canAssign);
    auto name = identifierConstant(ast->id);
    if (ast->token == TokenType::EQUAL) {
        expr(ast->args[0]);
        gen.emitByteConst(OpCode::SET_PROPERTY, name);
    } else if (ast->token == TokenType::LEFT_PAREN) {
        const uint8_t argCount = argumentList(ast->args);
        gen.emitByteConst(OpCode::INVOKE, name);
        gen.emitByte(argCount);
    } else {
        gen.emitByteConst(OpCode::GET_PROPERTY, name);
    }
}

void Compiler::unary(Unary *ast, bool canAssign) {
    expr(ast->expr, canAssign);

    // Emit the operator instruction.
    switch (ast->token) {
    case TokenType::BANG:
        gen.emitByte(OpCode::NOT);
        break;
    case TokenType::MINUS:
        gen.emitByte(OpCode::NEGATE);
        break;
    default:
        return; // Unreachable.
    }
}

void Compiler::variable(Identifier *ast, bool canAssign) {
    namedVariable(ast->name, canAssign);
}

void Compiler::number(Number *ast) {
    if (ast->value == 0) {
        gen.emitByte(OpCode::ZERO);
        return;
    }
    if (ast->value == 1) {
        gen.emitByte(OpCode::ONE);
        return;
    }
    gen.emitConstant(NUMBER_VAL(ast->value));
}

void Compiler::string(String *ast) {
    gen.emitConstant(OBJ_VAL(newString(ast->value)));
}

void Compiler::boolean(Boolean *ast) {
    if (ast->value) {
        gen.emitByte(OpCode::TRUE);
    } else {
        gen.emitByte(OpCode::FALSE);
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

} // namespace lox