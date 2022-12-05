# ALOX-CC

LOX converted to C++ and refactored.

Additions:

* [x] `break`, `continue`.
* [x] Unicode strings and identifiers.
* [ ] ++, --.
* [x] `getc()`, `chr(ch)`, `exit(status)`, `print_error(message)`.
* [ ] I/O read, write, stderr.
* [ ] `lambda` functions. lambda () { }
* [ ] Lists [1,2,3], Maps, Arrays, Sets. 
* [ ] Object class, literal initialisers {s: value....};
* [ ] Conditional expressions: `print x == 5 ? "5" : nil;`
* [ ] file inclusion `include`.
* [ ] module system `import`, `export`. Namespaces.
* [ ] Exceptions.

Modifications:

* [x] more than 255 constants in a function. Re-using constants. Opcodes ZERO, ONE.
* [x] Opcodes ZERO, ONE for number constants.
* [x] Change from realloc to new/delete for Objects.
* [x] Move init() functions to constructors, default initialisers.
* [x] NOT_EQUAL, NOT_LESS, NOT_GREATER.
* [x] moved from char* to std::string.


Book modifications:

* [ ] More efficient line number storage (Chapter 14, Q.1)
* [ ] Conditional expressions: `print x == 5 ? "5" : nil;` (Chapter 17 Q.3)
* [ ] Generalised Table<Value, Value> (Chapter 20 Q.1).
* [ ] `case` statement (Chapter 23 Q.1)
* [x] `continue`, `break`. Check for popping locals off the stack. (Chapter 23 Q.2)
* [x] Speed up with ip in VM. (Chapter 24 Q.1)
* [ ] Validate native function calls. (Chapter 24 Q.2)
* [ ] Optimise Obj fields in all objects (Chapter 26 Q.1)
* [ ] Check for fields in objects (Chapter 27 Q.1)
* [ ] Set init method into the class object (Chapter 28 Q.1)
