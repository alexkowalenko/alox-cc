#!env ts-node
//
// ALOX interpreter
//
// Copyright © Alex Kowalenko 2022.
//

var Mustache = require('mustache');
import * as fs from "fs";

//const dest_dir = "./x"
const dest_dir = "../src/ast"

type Fields = {
    name: string,
    type: string,
}

type Class = {
    name: string,
    name_lower?: string,
    instances: Array<Fields>;
    index?: number;
}

let classes: Array<Class> = [
    {
        name: "Declaration",
        instances: [{ type: "std::vector<Obj *>", name: "stats" }]
    },
    {
        name: "Statement",
        instances: [{ type: "Obj *", name: "stat" }]
    },
    {
        name: "Expr",
        instances: [{ type: "Obj *", name: "expr" }]
    },
    {
        name: "Primary",
        instances: [{ type: "Obj *", name: "expr" }]
    },
    {
        name: "Unary",
        instances: [{ type: "Expr *", name: "expr" }, { name: "token", type: "TokenType" }]
    },
    {
        name: "Binary",
        instances: [{ name: "left", type: "Expr*" }, { name: "right", type: "Expr*" }, { name: "token", type: "TokenType" },]
    },
    {
        name: "Number",
        instances: [{ name: "value", type: "double" }]
    },
    {
        name: "String",
        instances: [{ name: "value", type: "ObjString *" }]
    },
    {
        name: "Boolean",
        instances: [{ name: "value", type: "bool" }]
    },
    {
        name: "Nil",
        instances: []
    },
    {
        name: "Print",
        instances: [{ name: "expr", type: "Expr*" }]
    },
    {
        name: "VarDec",
        instances: [{ name: "var", type: "Identifier*" }, { type: "Expr*", name: "expr" }]
    },
    {
        name: "Identifier",
        instances: [{ type: "ObjString*", name: "name" }]
    },
    {
        name: "Assign",
        instances: [{ type: "Expr *", name: "left" }, { type: "Expr *", name: "right" },]
    },
    {
        name: "Block",
        instances: [{ type: "std::vector<Obj *>", name: "stats" }]
    },
    {
        name: "If",
        instances: [{ type: "Expr *", name: "cond" }, { type: "Statement *", name: "then_stat" }, { type: "Statement *", name: "else_stat" }]
    },
    {
        name: "While",
        instances: [{ type: "Expr *", name: "cond" }, { type: "Statement *", name: "body" }]
    },
    {
        name: "For",
        instances: [{ type: "Obj *", name: "init" }, { type: "Expr *", name: "cond" }, { type: "Expr *", name: "iter" }, { type: "Statement *", name: "body" }]
    },
    {
        name: "Break",
        instances: [{ type: "TokenType", name: "tok" }]
    },
    {
        name: "Return",
        instances: [{ type: "Expr *", name: "expr" }]
    },
    {
        name: "FunctDec",
        instances: [{ type: "Identifier*", name: "name" }, { type: "std::vector<Identifier*>", name: "parameters" }, { type: "Block *", name: "body" }]
    },
    {
        name: "Call",
        instances: [{ type: "Expr*", name: "fname" }, { type: "std::vector<Expr*>", name: "args" }]
    },
    {
        name: "ClassDec",
        instances: [{ type: "std::string", name: "name" }, { type: "std::string", name: "super" }, { type: "std::vector<FunctDec*>", name: "methods" }]
    },
];

function render(descript: Class, i: number) {
    const template = fs.readFileSync("ast.hh", { encoding: "utf8" });
    const rendered = Mustache.render(template, descript);
    const file = dest_dir + "/" + descript.name_lower + ".hh";
    console.log(file);
    fs.writeFileSync(file, rendered);
}

function render_include(classes: Array<Class>) {
    const template = fs.readFileSync("includes.hh", { encoding: "utf8" });
    const rendered = Mustache.render(template, { classes: classes });
    const file = dest_dir + "/includes.hh";
    console.log(file);
    fs.writeFileSync(file, rendered);
}

(function main() {

    if (!fs.existsSync(dest_dir)) {
        fs.mkdirSync(dest_dir);
    }

    // Do classes
    var index = 100;
    for (var x of classes) {
        x.index = index;
        x.name_lower = x.name.toLowerCase();
        render(x, index);
        index++;
    }

    // Do include file
    render_include(classes);
})();