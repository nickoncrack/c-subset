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
    AST_RETURN,
    AST_FUNCTION_DECL,
    AST_FUNCTION_CALL,
    AST_BREAK,
    AST_CONTINUE,
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
    OP_POINTER,
    OP_SIZEOF
};


typedef struct Type {
    enum data_type type;

    union {
        struct Type *pointee;

        struct {
            struct Type *return_type;
            struct Type **params;
            int count;
        } function;

        struct {
            struct Type *memb_type;
            int count;
        } array;

        struct {
            struct Type **memb_types;
            char **memb_names;
            int count;
        } structure;
    };
} Type;


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
            Type *type;
            char *name;
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

            struct AST_node *operand;
        } unary_op;

        struct {
            struct AST_node *condition;
            struct AST_node *body;
            struct AST_node *else_branch;
        } if_statement;

        struct {
            struct AST_node *condition;
            struct AST_node *body;
        } while_statement;

        struct {
            struct AST_node *expr;
        } return_statement;

        struct {
            Type *type;
            char *name;
            
            struct AST_node **args;
            int count;

            struct AST_node *body;
        } function_decl;

        struct {
            char *name;
            struct AST_node **args;
            int count;
        } function_call;

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
