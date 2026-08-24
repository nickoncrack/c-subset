#pragma once

#include <ast.h>


enum symbol_type {
    SYMBOL_VAR,
    SYMBOL_FUNC,
    SYMBOL_STRUCT_TAG
};

typedef struct Symbol {
    enum symbol_type type;
    struct Declarator *decl;
    
    uint32_t scope_depth;
    int stack_offset;

    struct Symbol *next;
} Symbol;

typedef struct Scope {
    int depth;
    Symbol *head;
    struct Scope *parent;
} Scope;

typedef struct SymbolTable {
    Scope *crt_scope;
    Scope *global_scope;

    /*
        Struct tags and normal variable/function identifiers live
        on a different namespace. Also this will only allow for
        struct declarations in the global scope.
    */
    Scope *tag_scope;
} SymbolTable;
