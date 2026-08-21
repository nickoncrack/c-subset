#pragma once

#include <stdint.h>
#include <stdbool.h>


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

enum data_type {
    TYPE_VOID,
    TYPE_INT,
    TYPE_CHAR,
    TYPE_POINTER,
    TYPE_FUNCTION,
    TYPE_ARRAY,
    TYPE_STRUCT
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
    OP_ADDRESS_OF,
    OP_SIZEOF
};


typedef struct Type {
    enum data_type type;

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
} Declarator;


typedef struct AST_node {
    enum AST_node_type type;

    union {
        struct {
            int value;
        } int_literal;

        struct {
            char *name;
        } var_ref;

        struct {
            Declarator *decl;
            struct AST_node *init;
        } var_decl;

        struct {
            enum binop_operator op;
            struct AST_node *left;
            struct AST_node *right;
        } binary_op;

        struct {
            enum unary_operator op;
            bool prefix; // true if the operator is placed before the operand (i.e. ++x) 

            union {
                struct AST_node *operand_ast;
                Type *operand_type;
            };
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
            Declarator *decl;
            struct AST_node *body;
        } function_decl;

        struct {
            struct AST_node *callee;
            struct AST_node **args;
            int count;
        } function_call;

        struct {
            char *name;
            Declarator **members;
            int count;
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
            struct AST_node **statements;
            int count;
        } block;

        /*
            A program is not treated as a big block for 2 reasons:
            1. On the root scope you can only place declarations
            2. parse_block() expects and consumes opening and closing braces
        */
        struct {
            struct AST_node **declarations;
            int count;
        } program;
    } as;
} AST_node;

AST_node *parse_block();
AST_node *parse_statement(bool);
AST_node *parse_logical_expression();
AST_node *parse_bitwise_operations();
AST_node *parse_comparison();
AST_node *parse_expression();
AST_node *parse_postfix_expression();
