#pragma once

#include <stdint.h>

#define POINTER_OR_INT(a, b) \
    ((is_integer_type(a) && b->type == TYPE_POINTER) || \
    (is_integer_type(b) && a->type == TYPE_POINTER))


enum data_type {
    TYPE_VOID,
    TYPE_INT,
    TYPE_CHAR,
    TYPE_POINTER,
    TYPE_FUNCTION,
    TYPE_ARRAY,
    TYPE_STRUCT
};

typedef struct Type {
    enum data_type type;
    
    // cache size in order to calculate it faster
    uint32_t size;

    union {
        struct Type *pointee;

        struct {
            struct Type *return_type;
            struct Declarator **params;
            int count;
        } function;

        struct {
            struct Type *memb_type;
            int count;
        } array;

        struct {
            char *name;
            struct Declarator **members;
            int count;
        } structure;
    };
} Type;

/*
    A declarator struct which contains both a type and identifier
    makes it significantly easier to parse complex declarations like
    a function pointer, where the identifier is in between parts of 
    the declaration (void (*fptr)(int), "fptr", the identifier is in
    between void and (int), both of which contribute to the declaration)
*/
typedef struct Declarator {
    Type *type;
    char *ident;
    int offset;
} Declarator;


bool is_assignable(Type *dst, Type *src);
bool is_castable(Type *cast_type, Type *operand);
bool is_addable(Type *a, Type *b);
bool is_integer_type(Type *t);
bool integer_compatible(Type *a, Type *b);
bool pointer_compatible(Type *a, Type *b);
bool is_pointer_type(Type *t);

bool typecmp(Type *a, Type *b);