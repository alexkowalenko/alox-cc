# ALOX-CC

LOX converted to C++

Additions:

* [x] `break`, `continue`.
* [ ] Unicode strings and identifiers.
* [ ] ++, --.
* [ ] I/O read, write, stderr.
* [ ] `lambda` functions. lambda () { }
* [ ] Lists [1,2,3], Maps, Arrays, Sets.
* [ ] Conditional expressions: `print x == 5 ? "5" : nil;`
* [ ] file inclusion `include`.
* [ ] module system `import`, `export`. Namespaces.

Modifications:

* [/] more than 255 constants in a function. Re-using constants.
* [x] Opcodes ZERO, ONE for number constants.
* [x] Change from realloc to new/delete - objects are not initialised : ObjBoundMethod, ObjClass, ObjClosure (Not ObjUpvalue array), ObjFunction, ObjInstance, ObjNative, ObjUpvalue, ObjString, Chunk, Table.
* [ ] Move init() functions to constructors, default initialisers.
