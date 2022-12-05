//
// ALOX-CC
//

#include <iostream>

#include <fmt/core.h>
#include <map>

#include "ast/boolean.hh"
#include "ast/includes.hh"
#include "ast_base.hh"
#include "parser.hh"
#include "scanner.hh"

inline constexpr auto debug_parse{false};
template <typename S, typename... Args>
static void debug(const S &format, const Args &...msg) {
    if constexpr (debug_parse) {
        std::cout << "parse: " << fmt::format(fmt::runtime(format), msg...) << '\n';
    }
}

Declaration *Parser::parse() {
    auto *ast = newDeclaration();

    advance();
    while (!match(TokenType::EOFS)) {
        declaration(ast);
    }
    return ast;
}

void Parser::declaration(Declaration *ast) {
    if (match(TokenType::CLASS)) {
        // classDeclaration();
    } else if (match(TokenType::FUN)) {
        // funDeclaration();
    } else if (match(TokenType::VAR)) {
        // varDeclaration();
    } else {
        ast->stats.push_back(statement());
    }

    if (panicMode) {
        synchronize();
    }
}

Statement *Parser::statement() {
    auto *ast = newStatement();
    if (match(TokenType::PRINT)) {
        ast->stat = OBJ_AST(printStatement());
    } else if (match(TokenType::FOR)) {
        // forStatement();
    } else if (match(TokenType::IF)) {
        // ifStatement();
    } else if (match(TokenType::RETURN)) {
        // returnStatement();
    } else if (match(TokenType::WHILE)) {
        // whileStatement();
    } else if (match(TokenType::BREAK)) {
        // breakStatement(TokenType::BREAK);
    } else if (match(TokenType::CONTINUE)) {
        // breakStatement(TokenType::CONTINUE);
    } else if (match(TokenType::LEFT_BRACE)) {
        // beginScope();
        // block();
        // endScope();
    } else {
        ast->stat = OBJ_AST(exprStatement());
    }
    return ast;
}

Print *Parser::printStatement() {
    auto *ast = newPrint();
    ast->expr = expr();
    consume(TokenType::SEMICOLON, "Expect ';' after value.");
    return ast;
}

Expr *Parser::exprStatement() {
    auto *ast = expr();
    consume(TokenType::SEMICOLON, "Expect ';' after expression.");
    return ast;
}

inline const std::map<TokenType, Precedence> precedence_map{
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
     {std::mem_fn(&Parser::grouping), nullptr, // std::mem_fn(&Compiler::call),
      Precedence::CALL}},
    {TokenType::RIGHT_PAREN, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::LEFT_BRACE, {nullptr, nullptr, Precedence::NONE}}, // [big]
    {TokenType::RIGHT_BRACE, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::COMMA, {nullptr, nullptr, Precedence::NONE}},
    // {TokenType::DOT, {nullptr, std::mem_fn(&Compiler::dot), Precedence::CALL}},
    {TokenType::MINUS,
     {std::mem_fn(&Parser::unary), std::mem_fn(&Parser::binary), Precedence::TERM}},
    {TokenType::PLUS, {nullptr, std::mem_fn(&Parser::binary), Precedence::TERM}},
    {TokenType::SEMICOLON, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::SLASH, {nullptr, std::mem_fn(&Parser::binary), Precedence::FACTOR}},
    {TokenType::ASTÉRIX, {nullptr, std::mem_fn(&Parser::binary), Precedence::FACTOR}},
    {TokenType::BANG, {std::mem_fn(&Parser::unary), nullptr, Precedence::NONE}},
    {TokenType::BANG_EQUAL,
     {nullptr, std::mem_fn(&Parser::binary), Precedence::EQUALITY}},
    // {TokenType::EQUAL, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::EQUAL_EQUAL,
     {nullptr, std::mem_fn(&Parser::binary), Precedence::EQUALITY}},
    {TokenType::GREATER, {nullptr, std::mem_fn(&Parser::binary), Precedence::COMPARISON}},
    {TokenType::GREATER_EQUAL,
     {nullptr, std::mem_fn(&Parser::binary), Precedence::COMPARISON}},
    {TokenType::LESS, {nullptr, std::mem_fn(&Parser::binary), Precedence::COMPARISON}},
    {TokenType::LESS_EQUAL,
     {nullptr, std::mem_fn(&Parser::binary), Precedence::COMPARISON}},
    // {TokenType::IDENTIFIER,
    //  {std::mem_fn(&Compiler::variable), nullptr, Precedence::NONE}},
    // {TokenType::STRING, {std::mem_fn(&Compiler::string), nullptr,
    // Precedence::NONE}},
    {TokenType::NUMBER, {std::mem_fn(&Parser::number), nullptr}},
    {TokenType::AND, {nullptr, std::mem_fn(&Parser::binary), Precedence::AND}},
    {TokenType::CLASS, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::ELSE, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::FALSE, {std::mem_fn(&Parser::primary), nullptr, Precedence::NONE}},
    {TokenType::FOR, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::FUN, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::IF, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::NIL, {std::mem_fn(&Parser::primary), nullptr, Precedence::NONE}},
    {TokenType::OR, {nullptr, std::mem_fn(&Parser::binary), Precedence::OR}},
    {TokenType::PRINT, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::RETURN, {nullptr, nullptr, Precedence::NONE}},
    // {TokenType::SUPER, {std::mem_fn(&Compiler::super_), nullptr,
    // Precedence::NONE}}, {TokenType::THIS, {std::mem_fn(&Compiler::this_),
    // nullptr, Precedence::NONE}},
    {TokenType::TRUE, {std::mem_fn(&Parser::primary), nullptr, Precedence::NONE}},
    {TokenType::VAR, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::WHILE, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::ERROR, {nullptr, nullptr, Precedence::NONE}},
    {TokenType::EOFS, {nullptr, nullptr, Precedence::NONE}},
};

ParseRule const *Parser::getRule(TokenType type) {
    return &rules.find(type)->second;
}

Expr *Parser::expr() {
    return parsePrecedence(Precedence::ASSIGNMENT);
}

Expr *Parser::parsePrecedence(Precedence precedence) {
    auto *left = newExpr();
    advance();
    auto prefixRule = getRule(previous.type)->prefix;
    if (prefixRule == nullptr) {
        error("Expect expression.");
        return left;
    }

    bool canAssign = precedence <= Precedence::ASSIGNMENT;
    left->expr = OBJ_AST(prefixRule(this, canAssign));

    while (precedence <= getRule(current.type)->precedence) {
        advance();
        auto infixRule = getRule(previous.type)->infix;
        left = infixRule(this, left, canAssign);
    }

    if (canAssign && match(TokenType::EQUAL)) {
        error("Invalid assignment target.");
    }
    return left;
}

Expr *Parser::unary(bool /*canAssign*/) {
    auto *ast = newUnary();
    ast->token = previous.type;
    ast->expr = parsePrecedence(Precedence::UNARY);
    auto *e = newExpr();
    e->expr = OBJ_AST(ast);
    return e;
}

Expr *Parser::binary(Expr *left, bool /*canAssign*/) {
    auto *binary = newBinary();
    binary->left = left;
    binary->token = previous.type;
    const auto precedence = get_precedence(previous.type);
    binary->right = parsePrecedence((Precedence)(int(precedence) + 1));
    auto *e = newExpr();
    e->expr = OBJ_AST(binary);
    return e;
}

Expr *Parser::grouping(bool /*canAssign*/) {
    auto *e = expr();
    consume(TokenType::RIGHT_PAREN, "Expect ')' after expression.");
    return e;
}

Expr *Parser::number(bool /*canAssign*/) {
    auto  *ast = newNumber();
    double value = strtod(previous.text.data(), nullptr);
    debug("number {}", value);
    ast->value = value;
    auto *e = newExpr();
    e->expr = OBJ_AST(ast);
    return e;
}

Expr *Parser::primary(bool /*canAssign*/) {
    auto *e = newExpr();
    switch (previous.type) {
    case TokenType::FALSE: {
        auto b = newBoolean();
        b->value = false;
        e->expr = OBJ_AST(b);
        return e;
    }
    case TokenType::NIL: {
        auto b = newNil();
        e->expr = OBJ_AST(b);
        return e;
    }
    case TokenType::TRUE: {
        auto b = newBoolean();
        b->value = true;
        e->expr = OBJ_AST(b);
        return e;
    }
    default:
        return nullptr; // Unreachable.
    }
}

void Parser::errorAt(Token *token, std::string_view message) {
    if (panicMode) {
        return;
    }
    panicMode = true;
    err << fmt::format("[line {}] Error", token->line);

    if (token->type == TokenType::EOFS) {
        err << " at end";
    } else if (token->type == TokenType::ERROR) {
        // Nothing.
    } else {
        err << fmt::format(" at '{:s}'", token->text);
    }
    err << fmt::format(": {}\n", message);
    hadError = true;
}

void Parser::synchronize() {
    panicMode = false;

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
