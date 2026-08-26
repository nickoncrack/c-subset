# c-subset
A compiler for a subset of the C language that compiles to casm machine code

## Usage
```
gcc src/* -I./include -o compiler.bin
./compiler.bin tests/main_test.c
```

[Sample output](tests/sample_output.txt)


## Todos

+ ~~AST free function~~
+ ~~Improve distinction code for variable assignment and function declaration~~
+ ~~Fix nested function calls~~
+ ~~Fix function calls inside if/while condition~~
+ ~~Add function type parsing and arrays~~
+ ~~Add unary operators: increment, decrement, bitwise/logical not, pointer dereference~~
+ ~~Add greater/less or equal, not equal tokens~~
+ ~~Add augmented assignment (i.e. x += 2)~~
+ ~~Add prefix/suffix increment and decrement~~
+ ~~Add else if/else branch parsing and single line branch that does not require braces~~
+ ~~Add for loops, break and continue keywords~~
+ ~~Add struct type parsing and struct member access~~
+ ~~Add type casts and sizeof()~~
+ Add error reporting
+ Analyze the AST semantically and populate it with symbols and scopes, along with type and name resolution 
+ Type checking
+ Code emission stage

### Low priority todos

+ typedef, extern and static keywords
+ Inline assembly
+ Preprocessor directives (include and define)
