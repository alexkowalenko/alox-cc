//
// ALOX-CC
//

#include "object.hh"
#include "value.hh"
#include "vm.hh"

#include <fmt/core.h>

#include <cstdlib>
#include <iostream>
#include <string>

namespace alox {

inline constexpr auto debug_stdlib{false};
template <typename S, typename... Args>
static void debug(const S &format, const Args &...msg) {
    if constexpr (debug_stdlib) {
        std::cout << "stdlib: " << fmt::format(fmt::runtime(format), msg...) << '\n';
    }
}

// Native functions must return a value or there will be a crash.

[[noreturn]] Value lox_exit(int /*argCount*/, Value const *value) {
    auto sig = as<double>(*value);
    exit(int(sig));
}

Value clockNative(int /*argCount*/, Value const * /*args*/) {
    return value<double>((double)clock() / CLOCKS_PER_SEC);
}

Value getc(int /*argCount*/, Value const * /*args*/) {
    auto ch = std::getc(stdin);
    return value<double>(ch);
}

Value chr(int /*argCount*/, Value const *v) {
    auto ch = char(as<double>(*v));
    // weird to get one char string (not 0 terminated)
    std::string s{1, ch};
    s.erase(1);
    s[0] = ch;
    debug("chr: {}", s.size());
    return value<Obj *>(newString(s));
}

Value ord(int /*argCount*/, Value const *v) {
    auto *s = as<ObjString *>(*v);
    debug("ord: '{}'", s->str);
    return value<double>(s->str[0]);
}

Value print_error(int /*argCount*/, Value const *value) {
    printValue(std::cerr, *value);
    return NIL_VAL;
}

void VM::defineNative(const std::string &name, NativeFn function) {
    push(value<Obj *>(newString(name)));
    push(value<Obj *>(newNative(function)));
    globals.set(as<ObjString *>(stack[0]), stack[1]);
    pop();
    pop();
}

void VM::def_stdlib() {
    defineNative("clock", clockNative);
    defineNative("exit", lox_exit);
    defineNative("getc", getc);
    defineNative("chr", chr);
    defineNative("ord", ord);
    defineNative("print_error", print_error);

    // Define generic empty class Object
    auto *obj_class = newClass(newString("Object"));
    globals.set(obj_class->name, value<Obj *>(obj_class));
}

} // namespace alox