#pragma once

#include <stdint.h>

enum tokentype {
    TOKEN_VOID,
    TOKEN_INT,
    TOKEN_CHAR,
    TOKEN_IDENT,
    TOKEN_NUM,
    TOKEN_IF,
    TOKEN_WHILE,
    TOKEN_RETURN,
    TOKEN_ADD,
    TOKEN_SUB,
    TOKEN_STAR,
    TOKEN_LESS,
    TOKEN_GREATER,
    TOKEN_EQUAL,
    TOKEN_NOT,
    TOKEN_LSH,
    TOKEN_RSH,
    TOKEN_AND,
    TOKEN_XOR,
    TOKEN_OR,
    TOKEN_LOGICAL_NOT,
    TOKEN_LOGICAL_AND,
    TOKEN_LOGICAL_OR,
    TOKEN_INCREMENT,
    TOKEN_DECREMENT,
    TOKEN_ASSIGN,
    TOKEN_LPAR,
    TOKEN_RPAR,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_EOF
};

typedef struct {
    enum tokentype type;
    int val;
    char text[64];
} token_t;


int get_next_token(char *src, token_t *dst);