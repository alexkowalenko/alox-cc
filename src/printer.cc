//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#include "printer.hh"
#include "ast/expr.hh"

void AST_Printer::print(Declaration *ast) {
    declaration(ast);
}

void AST_Printer::declaration(Declaration *ast) {
    for (auto *s : ast->stats) {
        statement(s);
        os << NL;
    }
}

void AST_Printer::statement(Statement *s) {
    if (IS_Print(s->stat)) {
        printStatement(AS_Print(s->stat));
        //} else if (parser->match(TokenType::FOR)) {
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
    } else {
        exprStatement(AS_Expr(s->stat));
    }
}

void AST_Printer::printStatement(Print *s) {
    os << "print ";
    expr(s->expr);
    os << ';';
}

void AST_Printer::exprStatement(Expr *s) {
    expr(s);
    os << ";";
}

void AST_Printer::expr(Expr *ast) {
    if (IS_Expr(ast->expr)) {
        expr(AS_Expr(ast->expr));
    } else if (IS_Unary(ast->expr)) {
        unary(AS_Unary(ast->expr));
    } else if (IS_Binary(ast->expr)) {
        binary(AS_Binary(ast->expr));
    } else if (IS_Number(ast->expr)) {
        number(AS_Number(ast->expr));
    } else if (IS_Boolean(ast->expr)) {
        boolean(AS_Boolean(ast->expr));
    } else if (IS_Nil(ast->expr)) {
        os << "nil";
    }
}

void AST_Printer::binary(Binary *ast) {
    os << '(';
    expr(ast->left);
    switch (ast->token) {
    case TokenType::BANG_EQUAL:
        os << " != ";
        break;
    case TokenType::EQUAL_EQUAL:
        os << " == ";
        break;
    case TokenType::GREATER:
        os << " > ";
        break;
    case TokenType::GREATER_EQUAL:
        os << " >= ";
        break;
    case TokenType::LESS:
        os << " < ";
        break;
    case TokenType::LESS_EQUAL:
        os << " <= ";
        break;
    case TokenType::PLUS:
        os << " + ";
        break;
    case TokenType::MINUS:
        os << " - ";
        break;
    case TokenType::ASTÉRIX:
        os << " * ";
        break;
    case TokenType::SLASH:
        os << " / ";
        break;
    case TokenType::AND:
        os << " and ";
        break;
    case TokenType::OR:
        os << " or ";
        break;
    }
    expr(ast->right);
    os << ')';
}

void AST_Printer::unary(Unary *ast) {
    switch (ast->token) {
    case TokenType::BANG:
        os << "!";
        break;
    case TokenType::MINUS:
        os << "-";
        break;
    default:
        return; // Unreachable.
    }
    expr(ast->expr);
}

void AST_Printer::boolean(Boolean *expr) {
    if (expr->value) {
        os << "true";
    } else {
        os << "false";
    }
}

void AST_Printer::number(Number *num) {
    os << num->value;
}