//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#include "printer.hh"
#include "ast/assign.hh"
#include "ast/block.hh"
#include "ast/break.hh"
#include "ast/declaration.hh"
#include "ast/expr.hh"
#include "ast/functdec.hh"
#include "ast/if.hh"
#include "ast/return.hh"
#include "ast/vardec.hh"
#include "ast_base.hh"
#include "context.hh"
#include "scanner.hh"
#include "value.hh"

namespace alox {

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
        os << ';';
    } else if (IS_FunctDec(s)) {
        funDec(AS_FunctDec(s));
    } else if (IS_ClassDec(s)) {
        classDec(AS_ClassDec(s));
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
}

void AST_Printer::funDec(FunctDec *ast, FunctionType type) {
    if (type == TYPE_FUNCTION) {
        os << "fun ";
    }
    identifier(ast->name);
    os << "(";
    for (auto i = 0; i < ast->parameters.size(); i++) {
        identifier(ast->parameters[i]);
        if (i < ast->parameters.size() - 1) {
            os << ", ";
        }
    }
    os << ") ";
    block(ast->body);
}

void AST_Printer::classDec(ClassDec *ast) {
    os << "class " << ast->name;
    if (!ast->super.empty()) {
        os << " < " << ast->super;
    }
    os << " {" << NL;
    for (auto *m : ast->methods) {
        funDec(m, TYPE_METHOD);
        os << NL;
    }
    os << "}" << NL;
}

void AST_Printer::statement(Statement *s) {
    if (IS_Print(s->stat)) {
        printStatement(AS_Print(s->stat));
    } else if (IS_For(s->stat)) {
        for_stat(AS_For(s->stat));
    } else if (IS_If(s->stat)) {
        if_stat(AS_If(s->stat));
    } else if (IS_Return(s->stat)) {
        return_stat(AS_Return(s->stat));
    } else if (IS_While(s->stat)) {
        while_stat(AS_While(s->stat));
    } else if (IS_Break(s->stat)) {
        break_stat(AS_Break(s->stat));
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

void AST_Printer::if_stat(If *s) {
    os << "if (";
    expr(s->cond);
    os << ')' << NL;
    statement(s->then_stat);
    if (s->else_stat) {
        os << NL << "else ";
        statement(s->else_stat);
    }
}

void AST_Printer::for_stat(For *s) {
    os << "for (";
    if (s->init) {
        if (IS_VarDec(s->init)) {
            varDec(AS_VarDec(s->init));
        } else if (IS_Expr(s->init)) {
            expr(AS_Expr(s->init));
        }
    }
    os << ';';
    if (s->cond) {
        expr(s->cond);
    }
    os << ';';
    if (s->iter) {
        expr(s->iter);
    }
    os << ')' << NL;
    statement(s->body);
}

void AST_Printer::while_stat(While *s) {
    os << "while (";
    expr(s->cond);
    os << ')' << NL;
    statement(s->body);
}

void AST_Printer::return_stat(Return *s) {
    os << "return";
    if (s->expr) {
        os << ' ';
        expr(s->expr);
    }
    os << ';';
}

void AST_Printer::break_stat(Break *s) {
    if (s->tok == TokenType::BREAK) {
        os << "break;";
        return;
    }
    os << "continue;";
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
    } else if (IS_Assign(ast->expr)) {
        assign(AS_Assign(ast->expr));
    } else if (IS_Call(ast->expr)) {
        call(AS_Call(ast->expr));
    } else if (IS_Dot(ast->expr)) {
        dot(AS_Dot(ast->expr));
    } else if (IS_This(ast->expr)) {
        this_(AS_This(ast->expr));
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
    case TokenType::DOT:
        os << " . ";
        break;
    default:;
    }

    expr(ast->right);
    os << ')';
}

void AST_Printer::assign(Assign *ast) {
    expr(ast->left);
    os << " = ";
    expr(ast->right);
}

void AST_Printer::call(Call *ast) {
    expr(ast->fname);
    args(ast->args);
}

void AST_Printer::dot(Dot *ast) {
    expr(ast->left);
    os << '.' << ast->id;
    switch (ast->token) {
    case TokenType::EQUAL:
        os << " = ";
        expr(ast->args[0]);
        break;
    case TokenType::LEFT_PAREN:
        args(ast->args);
        break;
    default:;
    };
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
    os << ast->name;
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
    os << '"' << str->value << '"';
}

void AST_Printer::this_(This *ast) {
    if (ast->token == TokenType::THIS) {
        os << "this";
        return;
    }
    os << "super." << ast->id;
    if (ast->has_args) {
        args(ast->args);
    }
}

void AST_Printer::args(const std::vector<Expr *> &args) {
    os << '(';
    for (auto i = 0; i < args.size(); i++) {
        expr(args[i]);
        if (i < args.size() - 1) {
            os << ", ";
        }
    }
    os << ')';
}

} // namespace lox