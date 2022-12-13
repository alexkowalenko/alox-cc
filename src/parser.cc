//
// ALOX-CC
//

#include <iostream>

#include <fmt/core.h>
#include <map>
#include <string_view>

#include "ast/dot.hh"
#include "ast/includes.hh"
#include "parser.hh"
#include "scanner.hh"

inline constexpr auto debug_parse{false};
template <typename S, typename... Args>
static void debug(const S &format, const Args &...msg) {
    if constexpr (debug_parse) {
        std::cout << "parse: " << fmt::format(fmt::runtime(format), msg...) << '\n';
    }
}

constexpr auto MAX_ARGS = UINT8_MAX;

Declaration *Parser::parse() {
    advance();
    auto *ast = newDeclaration(current.line);

    while (!match(TokenType::EOFS)) {
        auto *s = declaration();
        ast->stats.push_back(s);
    }
    return ast;
}

Obj *Parser::declaration() {
    // if (err.panicMode) {
    //     synchronize();
    // }
    if (match(TokenType::CLASS)) {
        return OBJ_AST(classDeclaration());
    }
    if (match(TokenType::FUN)) {
        return OBJ_AST(funDeclaration());
    }
    if (match(TokenType::VAR)) {
        auto *v = varDeclaration();
        consume(TokenType::SEMICOLON, "Expect ';' after variable declaration.");
        return OBJ_AST(v);
    }
    return OBJ_AST(statement());
}

VarDec *Parser::varDeclaration() {
    auto *ast = newVarDec(current.line);
    consume(TokenType::IDENTIFIER, "Expect variable name.");
    ast->var = ident();

    if (match(TokenType::EQUAL)) {
        ast->expr = expr();
    } else {
        ast->expr = nullptr;
    }
    return ast;
}

FunctDec *Parser::funDeclaration(FunctionType type) {
    debug("fun");
    auto  type_name = (type == TYPE_METHOD) ? "method" : "function";
    auto *ast = newFunctDec(current.line);
    consume(TokenType::IDENTIFIER, fmt::format("Expect {} name.", type_name));
    ast->name = ident();

    consume(TokenType::LEFT_PAREN, fmt::format("Expect '(' after {} name.", type_name));
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            if (ast->parameters.size() > MAX_ARGS) {
                errorAtCurrent(
                    fmt::format("Can't have more than {} parameters.", MAX_ARGS));
            }
            consume(TokenType::IDENTIFIER, "Expect parameter name.");
            auto *p = ident();
            ast->parameters.push_back(p);
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RIGHT_PAREN, "Expect ')' after parameters.");
    consume(TokenType::LEFT_BRACE, fmt::format("Expect '{{' before {} body.", type_name));
    ast->body = block();
    return ast;
}

ClassDec *Parser::classDeclaration() {
    debug("class");
    auto *ast = newClassDec(current.line);
    consume(TokenType::IDENTIFIER, "Expect class name.");
    ast->name = previous.text;
    Token className = previous;

    if (match(TokenType::LESS)) {
        consume(TokenType::IDENTIFIER, "Expect superclass name.");
        ast->super = previous.text;

        if (className.text == previous.text) {
            error("A class can't inherit from itself.");
        }
    }
    consume(TokenType::LEFT_BRACE, "Expect '{' before class body.");
    while (!check(TokenType::RIGHT_BRACE) && !check(TokenType::EOFS)) {
        ast->methods.push_back(funDeclaration(TYPE_METHOD));
    }
    consume(TokenType::RIGHT_BRACE, "Expect '}' after class body.");
    return ast;
}

Statement *Parser::statement() {
    auto *ast = newStatement(current.line);
    if (match(TokenType::PRINT)) {
        ast->stat = OBJ_AST(printStatement());
    } else if (match(TokenType::FOR)) {
        ast->stat = OBJ_AST(for_stat());
    } else if (match(TokenType::IF)) {
        ast->stat = OBJ_AST(if_stat());
    } else if (match(TokenType::RETURN)) {
        ast->stat = OBJ_AST(return_stat());
    } else if (match(TokenType::WHILE)) {
        ast->stat = OBJ_AST(while_stat());
    } else if (match(TokenType::BREAK)) {
        ast->stat = OBJ_AST(break_stat(TokenType::BREAK));
    } else if (match(TokenType::CONTINUE)) {
        ast->stat = OBJ_AST(break_stat(TokenType::CONTINUE));
    } else if (match(TokenType::LEFT_BRACE)) {
        ast->stat = OBJ_AST(block());
    } else {
        ast->stat = OBJ_AST(exprStatement());
    }
    return ast;
}

If *Parser::if_stat() {
    auto *ast = newIf(current.line);
    consume(TokenType::LEFT_PAREN, "Expect '(' after 'if'.");
    ast->cond = expr();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after condition.");
    ast->then_stat = statement();
    if (match(TokenType::ELSE)) {
        ast->else_stat = statement();
    } else {
        ast->else_stat = nullptr;
    }
    return ast;
}

While *Parser::while_stat() {
    auto *ast = newWhile(current.line);
    consume(TokenType::LEFT_PAREN, "Expect '(' after 'while'.");
    ast->cond = expr();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after condition.");
    ast->body = statement();
    return ast;
}

For *Parser::for_stat() {
    debug("for");
    auto *ast = newFor(current.line);

    //  initialiser expression
    consume(TokenType::LEFT_PAREN, "Expect '(' after 'for'.");
    if (match(TokenType::SEMICOLON)) {
        // No initializer.
        ast->init = nullptr;
    } else if (match(TokenType::VAR)) {
        ast->init = OBJ_AST(varDeclaration());
        consume(TokenType::SEMICOLON, "Expect ';' after loop initialiser.");
    } else {
        ast->init = OBJ_AST(expr());
        consume(TokenType::SEMICOLON, "Expect ';' after loop initialiser.");
    }

    // condition
    debug("for: cond");
    if (!match(TokenType::SEMICOLON)) {
        ast->cond = expr();
        consume(TokenType::SEMICOLON, "Expect ';' after loop condition.");
    } else {
        ast->cond = nullptr;
    }

    // iterate
    debug("for: iter");
    if (!match(TokenType::RIGHT_PAREN)) {
        ast->iter = expr();
        consume(TokenType::RIGHT_PAREN, "Expect ')' after for clauses.");
    } else {
        ast->iter = nullptr;
    }

    // statement
    debug("for: statement");
    ast->body = statement();
    return ast;
}

Return *Parser::return_stat() {
    auto *ast = newReturn(current.line);
    if (!match(TokenType::SEMICOLON)) {
        ast->expr = expr();
        consume(TokenType::SEMICOLON, "Expect ';' after value.");
    } else {
        ast->expr = nullptr;
    }
    return ast;
}

Break *Parser::break_stat(TokenType t) {
    auto *ast = newBreak(current.line);
    auto  name = "break";
    if (t == TokenType::CONTINUE) {
        name = "break";
    }
    consume(TokenType::SEMICOLON, fmt::format("Expect ';' after {}.", name));
    ast->tok = t;
    return ast;
}

Print *Parser::printStatement() {
    auto *ast = newPrint(current.line);
    ast->expr = expr();
    consume(TokenType::SEMICOLON, "Expect ';' after value.");
    return ast;
}

Block *Parser::block() {
    auto *ast = newBlock(current.line);
    while (!check(TokenType::RIGHT_BRACE) && !check(TokenType::EOFS)) {
        auto *s = declaration();
        ast->stats.push_back(s);
    }

    consume(TokenType::RIGHT_BRACE, "Expect '}' after block.");
    return ast;
}

Expr *Parser::exprStatement() {
    auto *ast = expr();
    consume(TokenType::SEMICOLON, "Expect ';' after expression.");
    return ast;
}

inline const std::map<TokenType, Precedence> precedence_map{
    {TokenType::EQUAL, Precedence::ASSIGNMENT},
    {TokenType::OR, Precedence::OR},
    {TokenType::AND, Precedence::AND},
    {TokenType::EQUAL_EQUAL, Precedence::EQUALITY},
    {TokenType::BANG_EQUAL, Precedence::EQUALITY},
    {TokenType::LESS, Precedence::COMPARISON},
    {TokenType::LESS_EQUAL, Precedence::COMPARISON},
    {TokenType::GREATER, Precedence::COMPARISON},
    {TokenType::GREATER_EQUAL, Precedence::COMPARISON},
    {TokenType::PLUS, Precedence::TERM},
    {TokenType::MINUS, Precedence::TERM},
    {TokenType::SLASH, Precedence::FACTOR},
    {TokenType::ASTÉRIX, Precedence::FACTOR},
    {TokenType::LEFT_PAREN, Precedence::CALL},
    {TokenType::DOT, Precedence::CALL}};

inline auto get_precedence(TokenType t) -> Precedence {
    if (precedence_map.contains(t)) {
        return precedence_map.at(t);
    }
    return Precedence::NONE;
}

inline const std::map<TokenType, ParseRule> rules{
    {TokenType::LEFT_PAREN,
     {
         std::mem_fn(&Parser::grouping),
         std::mem_fn(&Parser::call),
     }},
    {TokenType::RIGHT_PAREN, {nullptr, nullptr}},
    {TokenType::LEFT_BRACE, {nullptr, nullptr}}, // [big]
    {TokenType::RIGHT_BRACE, {nullptr, nullptr}},
    {TokenType::COMMA, {nullptr, nullptr}},
    {TokenType::DOT, {nullptr, std::mem_fn(&Parser::dot)}},
    {TokenType::MINUS, {std::mem_fn(&Parser::unary), std::mem_fn(&Parser::binary)}},
    {TokenType::PLUS, {nullptr, std::mem_fn(&Parser::binary)}},
    {TokenType::SEMICOLON, {nullptr, nullptr}},
    {TokenType::SLASH, {nullptr, std::mem_fn(&Parser::binary)}},
    {TokenType::ASTÉRIX, {nullptr, std::mem_fn(&Parser::binary)}},
    {TokenType::BANG, {std::mem_fn(&Parser::unary), nullptr}},
    {TokenType::BANG_EQUAL, {nullptr, std::mem_fn(&Parser::binary)}},
    {TokenType::EQUAL, {nullptr, std::mem_fn(&Parser::assign)}},
    {TokenType::EQUAL_EQUAL, {nullptr, std::mem_fn(&Parser::binary)}},
    {TokenType::GREATER, {nullptr, std::mem_fn(&Parser::binary)}},
    {TokenType::GREATER_EQUAL, {nullptr, std::mem_fn(&Parser::binary)}},
    {TokenType::LESS, {nullptr, std::mem_fn(&Parser::binary)}},
    {TokenType::LESS_EQUAL, {nullptr, std::mem_fn(&Parser::binary)}},
    {TokenType::IDENTIFIER, {std::mem_fn(&Parser::identifier), nullptr}},
    {TokenType::STRING, {std::mem_fn(&Parser::string), nullptr}},
    {TokenType::NUMBER, {std::mem_fn(&Parser::number), nullptr}},
    {TokenType::AND, {nullptr, std::mem_fn(&Parser::binary)}},
    {TokenType::CLASS, {nullptr, nullptr}},
    {TokenType::ELSE, {nullptr, nullptr}},
    {TokenType::FALSE, {std::mem_fn(&Parser::primary), nullptr}},
    {TokenType::FOR, {nullptr, nullptr}},
    {TokenType::FUN, {nullptr, nullptr}},
    {TokenType::IF, {nullptr, nullptr}},
    {TokenType::NIL, {std::mem_fn(&Parser::primary), nullptr}},
    {TokenType::OR, {nullptr, std::mem_fn(&Parser::binary)}},
    {TokenType::PRINT, {nullptr, nullptr}},
    {TokenType::RETURN, {nullptr, nullptr}},
    {TokenType::SUPER, {std::mem_fn(&Parser::super_), nullptr}},
    {TokenType::THIS, {std::mem_fn(&Parser::this_), nullptr}},
    {TokenType::TRUE, {std::mem_fn(&Parser::primary), nullptr}},
    {TokenType::VAR, {nullptr, nullptr}},
    {TokenType::WHILE, {nullptr, nullptr}},
    {TokenType::ERROR, {nullptr, nullptr}},
    {TokenType::EOFS, {nullptr, nullptr}},
};

ParseRule const *Parser::getRule(TokenType type) {
    return &rules.find(type)->second;
}

Expr *Parser::expr() {
    return parsePrecedence(Precedence::ASSIGNMENT);
}

Expr *Parser::parsePrecedence(Precedence precedence) {
    debug("parsePrecedence {}", int(precedence));
    auto *left = newExpr(current.line);
    advance();
    auto prefixRule = getRule(previous.type)->prefix;
    if (prefixRule == nullptr) {
        error("Expect expression.");
        return left;
    }

    bool canAssign = precedence <= Precedence::ASSIGNMENT;
    left->expr = OBJ_AST(prefixRule(this, canAssign));

    while (precedence <= get_precedence(current.type)) {
        advance();
        auto infixRule = getRule(previous.type)->infix;
        left = infixRule(this, left, canAssign);
    }

    debug("parsePrecedence can assign {}", canAssign);
    if (/* canAssign && */ match(TokenType::EQUAL)) {
        error("Invalid assignment target.");
    }
    return left;
}

Expr *Parser::unary(bool /*canAssign*/) {
    auto *ast = newUnary(current.line);
    ast->token = previous.type;
    ast->expr = parsePrecedence(Precedence::UNARY);
    auto *e = newExpr(current.line);
    e->expr = OBJ_AST(ast);
    return e;
}

Expr *Parser::binary(Expr *left, bool /*canAssign*/) {
    auto *binary = newBinary(current.line);
    binary->left = left;
    binary->token = previous.type;
    const auto precedence = get_precedence(previous.type);
    binary->right = parsePrecedence((Precedence)(int(precedence) + 1));
    auto *e = newExpr(current.line);
    e->expr = OBJ_AST(binary);
    return e;
}

Expr *Parser::assign(Expr *left, bool /*canAssign*/) {
    debug("assign");
    auto *a = newAssign(current.line);
    a->left = left;
    a->right = parsePrecedence(Precedence::ASSIGNMENT);
    auto *e = newExpr(current.line);
    e->expr = OBJ_AST(a);
    return e;
}

Expr *Parser::call(Expr *left, bool /*canAssign*/) {
    auto *call = newCall(current.line);
    call->fname = left;
    argumentList(call->args);
    auto *e = newExpr(current.line);
    e->expr = OBJ_AST(call);
    return e;
}

Expr *Parser::dot(Expr *left, bool canAssign) {
    auto *dot = newDot(current.line);
    dot->left = left;
    consume(TokenType::IDENTIFIER, "Expect property name after '.'.");
    dot->id = previous.text;

    if (canAssign && match(TokenType::EQUAL)) {
        dot->token = TokenType::EQUAL;
        dot->args.push_back(expr());
    } else if (match(TokenType::LEFT_PAREN)) {
        dot->token = TokenType::LEFT_PAREN;
        argumentList(dot->args);
    } else {
        // Get - collect nothing
    }
    auto *e = newExpr(current.line);
    e->expr = OBJ_AST(dot);
    return e;
}

Expr *Parser::grouping(bool /*canAssign*/) {
    auto *e = expr();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
    return e;
}

Expr *Parser::identifier(bool /*canAssign*/) {
    auto *e = newExpr(current.line);
    e->expr = OBJ_AST(ident());
    return e;
}

Expr *Parser::number(bool /*canAssign*/) {
    auto  *ast = newNumber(current.line);
    double value = strtod(previous.text.data(), nullptr);
    debug("number {}", value);
    ast->value = value;
    auto *e = newExpr(current.line);
    e->expr = OBJ_AST(ast);
    return e;
}

Expr *Parser::string(bool /*canAssign*/) {
    auto *ast = newString(current.line);
    ast->value = newString(previous.text);
    auto *e = newExpr(current.line);
    e->expr = OBJ_AST(ast);
    return e;
}

Expr *Parser::primary(bool /*canAssign*/) {
    auto *e = newExpr(current.line);
    switch (previous.type) {
    case TokenType::FALSE: {
        auto b = newBoolean(current.line);
        b->value = false;
        e->expr = OBJ_AST(b);
        return e;
    }
    case TokenType::NIL: {
        auto b = newNil(current.line);
        e->expr = OBJ_AST(b);
        return e;
    }
    case TokenType::TRUE: {
        auto b = newBoolean(current.line);
        b->value = true;
        e->expr = OBJ_AST(b);
        return e;
    }
    default:
        return nullptr; // Unreachable.
    }
}

Identifier *Parser::ident() {
    auto *id = newIdentifier(current.line);
    id->name = newString(previous.text);
    return id;
}

Expr *Parser::super_(bool /*canAssign*/) {
    auto *ast = newThis(current.line);
    ast->token = TokenType::SUPER;
    consume(TokenType::DOT, "Expect '.' after 'super'.");
    consume(TokenType::IDENTIFIER, "Expect superclass method name.");
    ast->id = previous.text;
    ast->has_args = false;
    if (match(TokenType::LEFT_PAREN)) {
        ast->has_args = true;
        argumentList(ast->args);
    }
    auto *e = newExpr(current.line);
    e->expr = OBJ_AST(ast);
    return e;
}

Expr *Parser::this_(bool /*canAssign*/) {
    auto *t = newThis(current.line);
    t->token = TokenType::THIS;
    auto *e = newExpr(current.line);
    e->expr = OBJ_AST(t);
    return e;
}

void Parser::argumentList(std::vector<Expr *> &args) {
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            if (args.size() == MAX_ARGS) {
                error("Can't have more than 255 arguments.");
            }
            auto *e = expr();
            args.push_back(e);
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RIGHT_PAREN, "Expect ')' after arguments.");
}

void Parser::synchronize() {
    err.panicMode = false;

    while (current.type != TokenType::EOFS) {
        if (previous.type == TokenType::SEMICOLON) {
            return;
        }
        switch (current.type) {
        case TokenType::CLASS:
        case TokenType::FUN:
        case TokenType::VAR:
        case TokenType::FOR:
        case TokenType::IF:
        case TokenType::WHILE:
        case TokenType::PRINT:
        case TokenType::RETURN:
            return;

        default:; // Do nothing.
        }

        advance();
    }
}

void Parser::advance() {
    previous = current;

    for (;;) {
        current = scanner.scanToken();
        if (current.type != TokenType::ERROR) {
            break;
        }
        errorAtCurrent(current.text);
    }
}

void Parser::consume(TokenType type, std::string_view message) {
    if (current.type == type) {
        advance();
        return;
    }
    errorAtCurrent(message);
}

bool Parser::match(TokenType type) {
    if (!check(type)) {
        return false;
    }
    advance();
    return true;
}
