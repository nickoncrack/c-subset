/*
    TODO:
    1. function calling conventions
    2. inline assembly
    3. preprocessor directives
    4. typedef keyword
*/

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <typing.h>
#include <analysis.h>


enum AST_node_type {
    AST_INT_LITERAL,
    AST_VAR_REF,
    AST_VAR_DECL,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_RETURN,
    AST_FUNCTION_DECL,
    AST_FUNCTION_CALL,
    AST_BREAK,
    AST_CONTINUE,
    AST_STRUCT_DECL,
    AST_STRUCT_ACCESS,
    AST_ARRAY_ACCESS,
    AST_TYPE_CAST,
    AST_SIZEOF,
    AST_BLOCK,
    AST_PROGRAM
};

enum binop_operator {
    OP_ADD,
    OP_SUB,
    OP_STAR,
    OP_LESS,
    OP_GREATER,
    OP_EQUAL,
    OP_LE,
    OP_GE,
    OP_NE,
    OP_LSH,
    OP_RSH,
    OP_AND,
    OP_XOR,
    OP_OR,
    OP_LOGICAL_AND,
    OP_LOGICAL_OR,
    OP_ASSIGN
};

enum unary_operator {
    OP_INCREMENT,
    OP_DECREMENT,
    OP_NOT,
    OP_LOGICAL_NOT,
    OP_DEREFERENCE,
    OP_ADDRESS_OF
};

enum block_type {
    BLOCK_FUNCTION,
    BLOCK_CONDITION,
    BLOCK_LOOP
};

enum func_call_convention {
    CONV_CDECL,
    CONV_STDCALL
};

typedef struct AST_node {
    enum AST_node_type type;
    Type *expr_type;

    union {
        struct {
            int value;
        } int_literal;

        struct {
            char *name;
            struct Symbol *sym;
        } var_ref;

        struct {
            Declarator *decl;
            struct AST_node *init;
            struct Symbol *sym;
        } var_decl;

        struct {
            enum binop_operator op;
            struct AST_node *left;
            struct AST_node *right;
        } binary_op;

        struct {
            enum unary_operator op;
            bool prefix; // true if the operator is placed before the operand (i.e. ++x) 
            struct AST_node *operand;
        } unary_op;

        struct {
            // index 0 points to the if statement, the rest point to else if statements
            struct AST_node **conditions;
            struct AST_node **blocks;
            struct AST_node *else_branch;
            int count;
        } if_statement;

        struct {
            struct AST_node *condition;
            struct AST_node *body;
        } while_statement;

        struct {
            struct AST_node *init;
            struct AST_node *condition;
            struct AST_node *updation;
            struct AST_node *body;
        } for_statement;

        struct {
            struct AST_node *expr;
        } return_statement;

        struct {
            // enum func_call_convention conv;

            Declarator *decl;
            struct Symbol *sym;
            struct AST_node *body;
        } function_decl;

        struct {
            struct AST_node *callee;
            struct AST_node **args;
            int count;
        } function_call;

        struct {
            Declarator *decl;
            struct Symbol *sym;
        } struct_decl;

        struct {
            struct AST_node *src;
            char *member;
            bool pointer; // true if access symbol is ->
        } struct_access;

        struct {
            struct AST_node *array;
            struct AST_node *index;
        } array_access;

        struct {
            Type *type;
            struct AST_node *operand;
        } type_cast;

        struct {
            bool is_type;
            union {
                Type *operand_type;
                struct AST_node *operand_ast;
            };
        } sizeof_;

        struct {
            enum block_type type;
            struct AST_node **statements;
            int count;
            struct Scope *scope;
        } block;

        /*
            A program is not treated as a big block for 2 reasons:
            1. On the root scope you can only place declarations
            2. parse_block() expects and consumes opening and closing braces
        */
        struct {
            struct AST_node **declarations;
            int count;
            
            struct Scope *global;
        } program;
    } as;
} AST_node;


AST_node *parse_program();