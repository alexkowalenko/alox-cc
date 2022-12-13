//
// ALOX-CC
//
// Copyright © Alex Kowalenko 2022.
//

#include "ast_base.hh"
#include "ast/includes.hh"

void blackenASTObject(Obj *object) {
    switch (object->type) {
    case AST_Declaration: {
        auto *ast = AS_Declaration(object);
        for (auto *d : ast->stats) {
            gc.markObject(d);
        }
        break;
    }
    case AST_Statement: {
        auto *ast = AS_Statement(object);
        gc.markObject(ast->stat);
        break;
    }
    case AST_Expr: {
        auto *ast = AS_Expr(object);
        gc.markObject(ast->expr);
        break;
    }
    case AST_Primary: {
        auto *ast = AS_Primary(object);
        gc.markObject(ast->expr);
        break;
    }
    case AST_Unary: {
        auto *ast = AS_Unary(object);
        gc.markObject(OBJ_AST(ast->expr));
        break;
    }
    case AST_Binary: {
        auto *ast = AS_Binary(object);
        gc.markObject(OBJ_AST(ast->left));
        gc.markObject(OBJ_AST(ast->right));
        break;
    }
    case AST_String: {
        auto *ast = AS_String(object);
        gc.markObject(OBJ_AST(ast->value));
        break;
    }
    case AST_Print: {
        auto *ast = AS_Print(object);
        gc.markObject(OBJ_AST(ast->expr));
        break;
    }
    case AST_Identifier: {
        auto *ast = AS_Identifier(object);
        gc.markObject(OBJ_AST(ast->name));
        break;
    }
    case AST_Assign: {
        auto *ast = AS_Assign(object);
        gc.markObject(OBJ_AST(ast->left));
        gc.markObject(OBJ_AST(ast->right));
        break;
    }
    case AST_Block: {
        auto *ast = AS_Block(object);
        for (auto *d : ast->stats) {
            gc.markObject(d);
        }
        break;
    }
    case AST_If: {
        auto *ast = AS_If(object);
        gc.markObject(OBJ_AST(ast->cond));
        gc.markObject(OBJ_AST(ast->then_stat));
        if (ast->else_stat) {
            gc.markObject(OBJ_AST(ast->else_stat));
        }
        break;
    }
    case AST_While: {
        auto *ast = AS_While(object);
        gc.markObject(OBJ_AST(ast->cond));
        gc.markObject(OBJ_AST(ast->body));
        break;
    }
    case AST_For: {
        auto *ast = AS_For(object);
        if (ast->init) {
            gc.markObject(OBJ_AST(ast->init));
        }
        if (ast->cond) {
            gc.markObject(OBJ_AST(ast->cond));
        }
        if (ast->iter) {
            gc.markObject(OBJ_AST(ast->iter));
        }
        gc.markObject(OBJ_AST(ast->body));
        break;
    }
    case AST_Return: {
        auto *ast = AS_Return(object);
        gc.markObject(OBJ_AST(ast->expr));
        break;
    }
    case AST_FunctDec: {
        auto *ast = AS_FunctDec(object);
        gc.markObject(OBJ_AST(ast->name));
        for (auto *d : ast->parameters) {
            gc.markObject(OBJ_AST(d));
        }
        gc.markObject(OBJ_AST(ast->body));
        break;
    }
    case AST_Call: {
        auto *ast = AS_Call(object);
        gc.markObject(OBJ_AST(ast->fname));
        for (auto *d : ast->args) {
            gc.markObject(OBJ_AST(d));
        }
        break;
    }
    case AST_ClassDec: {
        auto *ast = AS_ClassDec(object);
        for (auto *d : ast->methods) {
            gc.markObject(OBJ_AST(d));
        }
        break;
    }
    case AST_Dot: {
        auto *ast = AS_Dot(object);
        gc.markObject(OBJ_AST(ast->left));
        for (auto *d : ast->args) {
            gc.markObject(OBJ_AST(d));
        }
        break;
    }
    case AST_This: {
        auto *ast = AS_This(object);
        for (auto *d : ast->args) {
            gc.markObject(OBJ_AST(d));
        }
        break;
    }
    }
}

void freeASTObject(Obj *object) {
    switch (object->type) {
    case AST_Declaration: {
        auto *ast = AS_Declaration(object);
        for (auto *d : ast->stats) {
            freeASTObject(d);
        }
        gc.deleteObject(ast);
        break;
    }
    case AST_Statement: {
        auto *ast = AS_Statement(object);
        gc.freeObject(ast->stat);
        gc.deleteObject(ast);
        break;
    }
    case AST_Expr: {
        auto *ast = AS_Expr(object);
        gc.deleteObject(ast->expr);
        gc.deleteObject(ast);
        break;
    }
    case AST_Primary: {
        auto *ast = AS_Primary(object);
        gc.freeObject(ast->expr);
        gc.deleteObject(ast);
        break;
    }
    case AST_Unary: {
        auto *ast = AS_Unary(object);
        gc.freeObject(OBJ_AST(ast->expr));
        gc.deleteObject(ast);
        break;
    }
    case AST_Binary: {
        auto *ast = AS_Binary(object);
        gc.freeObject(OBJ_AST(ast->left));
        gc.freeObject(OBJ_AST(ast->right));
        gc.deleteObject(ast);
        break;
    }
    case AST_String: {
        auto *ast = AS_String(object);
        gc.freeObject(OBJ_AST(ast->value));
        gc.deleteObject(ast);
        break;
    }
    case AST_Print: {
        auto *ast = AS_Print(object);
        gc.freeObject(OBJ_AST(ast->expr));
        gc.deleteObject(ast);
        break;
    }
    case AST_Identifier: {
        auto *ast = AS_Identifier(object);
        gc.freeObject(OBJ_AST(ast->name));
        gc.deleteObject(ast);
        break;
    }
    case AST_Assign: {
        auto *ast = AS_Assign(object);
        gc.freeObject(OBJ_AST(ast->left));
        gc.freeObject(OBJ_AST(ast->right));
        gc.deleteObject(ast);
        break;
    }
    case AST_Block: {
        auto *ast = AS_Block(object);
        for (auto *d : ast->stats) {
            gc.freeObject(d);
        }
        gc.deleteObject(ast);
        break;
    }
    case AST_If: {
        auto *ast = AS_If(object);
        gc.freeObject(OBJ_AST(ast->cond));
        gc.freeObject(OBJ_AST(ast->then_stat));
        if (ast->else_stat) {
            gc.freeObject(OBJ_AST(ast->else_stat));
        }
        gc.deleteObject(ast);
        break;
    }
    case AST_While: {
        auto *ast = AS_While(object);
        gc.freeObject(OBJ_AST(ast->cond));
        gc.freeObject(OBJ_AST(ast->body));
        gc.deleteObject(ast);
        break;
    }
    case AST_For: {
        auto *ast = AS_For(object);
        if (ast->init) {
            gc.freeObject(OBJ_AST(ast->init));
        }
        if (ast->cond) {
            gc.freeObject(OBJ_AST(ast->cond));
        }
        if (ast->iter) {
            gc.freeObject(OBJ_AST(ast->iter));
        }
        gc.freeObject(OBJ_AST(ast->body));
        gc.deleteObject(ast);
        break;
    }
    case AST_Return: {
        auto *ast = AS_Return(object);
        if (ast->expr) {
            gc.freeObject(OBJ_AST(ast->expr));
        }
        break;
    }
    case AST_FunctDec: {
        auto *ast = AS_FunctDec(object);
        gc.freeObject(OBJ_AST(ast->name));
        for (auto *d : ast->parameters) {
            gc.freeObject(OBJ_AST(d));
        }
        gc.freeObject(OBJ_AST(ast->body));
        gc.deleteObject(ast);
        break;
    }
    case AST_Call: {
        auto *ast = AS_Call(object);
        gc.freeObject(OBJ_AST(ast->fname));
        for (auto *d : ast->args) {
            gc.freeObject(OBJ_AST(d));
        }
        gc.deleteObject(ast);
        break;
    }
    case AST_ClassDec: {
        auto *ast = AS_ClassDec(object);
        for (auto *d : ast->methods) {
            gc.freeObject(OBJ_AST(d));
        }
        gc.deleteObject(ast);
        break;
    }
    case AST_Dot: {
        auto *ast = AS_Dot(object);
        gc.freeObject(OBJ_AST(ast->left));
        for (auto *d : ast->args) {
            gc.freeObject(OBJ_AST(d));
        }
        gc.deleteObject(ast);
        break;
    }
    case AST_This: {
        auto *ast = AS_This(object);
        for (auto *d : ast->args) {
            gc.freeObject(OBJ_AST(d));
        }
        break;
    }
    }
}

void markASTRoots(Declaration *ast) {
    for (auto *d : ast->stats) {
        gc.markObject(d);
    }
};