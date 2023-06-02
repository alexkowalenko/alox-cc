//
// ALOX-CC
//

#pragma once

#include "ast/includes.hh"
#include "ast_base.hh"
#include "chunk.hh"
#include "codegen.hh"
#include "context.hh"
#include "object.hh"
#include "options.hh"

#include <string_view>

namespace alox {

class Compiler {
  public:
    Compiler(const Options &opt, ErrorManager &err) : options(opt), err(err), gen(err){};
    ~Compiler() = default;

    ObjFunction *compile(Declaration *ast);
    void         markCompilerRoots();

  private:
    // Compile the AST
    void declaration(Declaration *ast);
    void decs_statement(Obj *);
    void varDeclaration(VarDec *ast);
    void funDeclaration(FunctDec *ast);
    void classDeclaration(ClassDec *ast);

    void statement(Statement *ast);
    void ifStatement(If *ast);
    void forStatement(For *ast);
    void whileStatement(While *ast);
    void printStatement(Print *ast);
    void returnStatement(Return *ast);
    void breakStatement(Break *ast);
    void block(Block *);
    void exprStatement(Expr *ast);

    void expr(Expr *ast, bool canAssign = false);
    void binary(Binary *ast, bool canAssign);
    void assign(Assign *ast);
    void call(Call *ast);
    void dot(Dot *ast, bool canAssign);
    void and_(Binary *ast, bool canAssign);
    void or_(Binary *ast, bool canAssign);
    void unary(Unary *ast, bool canAssign);
    void variable(Identifier *ast, bool canAssign);
    void number(Number *ast);
    void string(String *ast);
    void boolean(Boolean *ast);

    void super_(This *ast, bool /*canAssign*/);
    void this_(This *ast, bool /*canAssign*/);

    void initCompiler(Context *compiler, const std::string &name, FunctionType type);
    ObjFunction *endCompiler();

    const_index_t parseVariable(const std::string &var);
    void          declareVariable(const std::string &name);
    void          addLocal(const std::string &name);
    const_index_t identifierConstant(const std::string &name);
    void          defineVariable(const_index_t global);
    void          markInitialized();
    void          beginScope();
    void          endScope();
    void          namedVariable(const std::string &name, bool canAssign);
    void          adjust_locals(int depth);
    int           resolveLocal(Context *compiler, const std::string &name);
    int           addUpvalue(Context *compiler, uint8_t index, bool isLocal);
    int           resolveUpvalue(Context *compiler, const std::string &name);

    void    function(FunctDec *ast, FunctionType type);
    void    method(FunctDec *ast);
    uint8_t argumentList(const std::vector<Expr *> &args);

    void error(size_t line, const std::string_view &);

    const Options &options;
    ErrorManager  &err;

    Context      *current{nullptr};
    ClassContext *currentClass{nullptr};
    CodeGen       gen;
};

} // namespace lox
