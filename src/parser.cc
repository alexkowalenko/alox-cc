//
// ALOX-CC
//

#include <iostream>

#include <fmt/core.h>

#include "ast/expr.hh"
#include "ast/includes.hh"
#include "ast/number.hh"
#include "ast/primary.hh"
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
        // printStatement();
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
        ast->stat = OBJ_AST(expr());
    }
    return ast;
}

Expr *Parser::expr() {
    auto *ast = newExpr();
    ast->expr = primary();
    consume(TokenType::SEMICOLON, "Expect ';' after expression.");
    return ast;
}

Primary *Parser::primary() {
    auto *ast = newPrimary();
    ast->value = number();
    return ast;
}

Number *Parser::number() {
    auto *ast = newNumber();
    consume(TokenType::NUMBER, "expecting number");
    double value = strtod(previous.text.data(), nullptr);
    debug("number {}", value);
    ast->value = value;
    return ast;
}

void Parser::errorAt(Token *token, std::string_view message) {
    if (panicMode) {
        return;
    }
    panicMode = true;
    std::cerr << fmt::format("[line {}] Error", token->line);

    if (token->type == TokenType::EOFS) {
        std::cerr << " at end";
    } else if (token->type == TokenType::ERROR) {
        // Nothing.
    } else {
        std::cerr << fmt::format(" at '{:s}'", token->text);
    }
    std::cerr << fmt::format(": {}\n", message);
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
