#pragma once

#include <ast.h>


enum symbol_type {
    SYMBOL_VAR,
    SYMBOL_FUNC,
    SYMBOL_FUNC_ARG,
    SYMBOL_STRUCT_TAG
};

enum scope_type {
    SCOPE_FUNCTION,
    SCOPE_CONDITION,
    SCOPE_LOOP,
    SCOPE_GLOBAL,
    SCOPE_TAG
};

typedef struct Symbol {
    enum symbol_type type;
    Declarator *decl;

    uint32_t scope_depth;
    int stack_offset;
    uint32_t frame_size;

    struct Symbol *next;
} Symbol;

typedef struct Scope {
    enum scope_type type;
    int depth;

    Symbol *owner;
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


Symbol *struct_tag_lookup(char *name, SymbolTable *tab);
Symbol *symbol_table_lookup(char *name, SymbolTable *tab);