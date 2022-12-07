//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#include "printer.hh"
#include "ast/block.hh"
#include "ast/declaration.hh"
#include "ast/expr.hh"
#include "ast/vardec.hh"
#include "ast_base.hh"
#include "value.hh"

void AST_Printer::print(Declaration *ast) {
    declaration(ast);
}

void AST_Printer::declaration(Declaration *ast) {
    for (auto *s : ast->stats) {
        decs_statement(s);
    }
}

void AST_Printer::decs_statement(Obj *s) {
    if (IS_VarDec(s)) {
        varDec(AS_VarDec(s));
    } else {
        statement(AS_Statement(s));
    }
    os << NL;
}

void AST_Printer::varDec(VarDec *ast) {
    os << "var ";
    identifier(ast->var);
    if (ast->expr) {
        os << " = ";
        expr(ast->expr);
    }
    os << ';';
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
    } else if (IS_Block(s->stat)) {
        block(AS_Block(s->stat));
    } else {
        exprStatement(AS_Expr(s->stat));
    }
}

void AST_Printer::block(Block *s) {
    os << '{' << NL;
    for (auto *s : s->stats) {
        os << std::string(indent, ' ');
        decs_statement(s);
    }
    os << '}';
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
    } else if (IS_Identifier(ast->expr)) {
        identifier(AS_Identifier(ast->expr));
    } else if (IS_Number(ast->expr)) {
        number(AS_Number(ast->expr));
    } else if (IS_String(ast->expr)) {
        string(AS_String(ast->expr));
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

void AST_Printer::identifier(Identifier *ast) {
    printObject(os, OBJ_VAL(ast->name));
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

void AST_Printer::string(String *str) {
    os << '"';
    printObject(os, OBJ_VAL(str->value));
    os << '"';
}