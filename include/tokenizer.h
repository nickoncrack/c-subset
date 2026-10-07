#pragma once

#include <stdint.h>

#define CRT_TYPE            get_current_token().type
#define CRT_VAL             get_current_token().val
#define CRT_TEXT            get_current_token().text
#define CRT_LINE            get_current_token().line
#define CRT_COLUMN          get_current_token().column
#define OFFSET_CRT_TYPE(n)  (*(arr + n)).type

#define IS_CRT_TYPE         ((CRT_TYPE == TOKEN_VOID) || (CRT_TYPE == TOKEN_INT) || (CRT_TYPE == TOKEN_CHAR))


enum tokentype {
    TOKEN_VOID,
    TOKEN_INT,
    TOKEN_CHAR,
    TOKEN_IDENT,
    TOKEN_NUM,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_RETURN,
    TOKEN_ADD,
    TOKEN_SUB,
    TOKEN_STAR,
    TOKEN_LESS,
    TOKEN_GREATER,
    TOKEN_EQUAL,
    TOKEN_LE,
    TOKEN_GE,
    TOKEN_NE,
    TOKEN_LSH,
    TOKEN_RSH,
    TOKEN_AND,
    TOKEN_XOR,
    TOKEN_OR,
    TOKEN_LOGICAL_AND,
    TOKEN_LOGICAL_OR,
    TOKEN_ASSIGN,
    TOKEN_NOT,
    TOKEN_LOGICAL_NOT,
    TOKEN_INCREMENT,
    TOKEN_DECREMENT,
    TOKEN_LPAR,
    TOKEN_RPAR,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_SIZEOF,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_STRUCT,
    TOKEN_MEMB_ACCESS,
    TOKEN_PTR_MEMB_ACCESS,
    TOKEN_EOF
};

typedef struct {
    enum tokentype type;
    int val;
    char text[64];

    int line;
    int column;
} token_t;


extern token_t *arr;

token_t get_current_token();
int get_next_token(char *src, token_t *dst);
uint32_t tokenize(char *p, token_t *dst);

void consume();
void expect(enum tokentype token);
void expect_and_consume(enum tokentype token);