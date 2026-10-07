#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <tokenizer.h>

#define isdigit(c) (c >= '0' && c <= '9')
#define isalpha(c) ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))

#define SET_BUFF_TYPE(lc, uc) else if (!strcmp(buff, lc)) dst->type = TOKEN_##uc
#define TOKENIZER_CHAR_CASE(c, uc) \
    if (*src == c) { \
        if (alpha || digit) break; \
        src++; \
        dst->type = TOKEN_##uc; \
        dst->line = line; \
        dst->column = column; \
        free(buff); \
        return cnt+1; \
    }

#define TOKENIZER_2CHAR_CASE(c, uc, c2, uc2) \
    if (*src == c) { \
        if (alpha || digit) break; \
        src++; \
        if (*src == c2) { \
            src++; \
            dst->type = TOKEN_##uc2; \
            dst->line = line; \
            dst->column = column; \
            free(buff); \
            return cnt + 2; \
        } \
        dst->type = TOKEN_##uc; \
        free(buff); \
        return cnt + 1; \
    }

#define TOKENIZER_3CHAR_CASE(c, uc, c2, uc2, c3, uc3) \
    if (*src == c) { \
        if (alpha || digit) break; \
        src++; \
        if (*src == c2) { \
            src++; \
            dst->type = TOKEN_##uc2; \
            dst->line = line; \
            dst->column = column; \
            free(buff); \
            return cnt + 2; \
        } else if (*src == c3) { \
            src++; \
            dst->type = TOKEN_##uc3; \
            dst->line = line; \
            dst->column = column; \
            free(buff); \
            return cnt + 2; \
        } \
        dst->type = TOKEN_##uc; \
        dst->line = line; \
        dst->column = column; \
        free(buff); \
        return cnt + 1; \
    }


int line = 1;
int column = 1;
char *filename;

extern const char *token_to_string(enum tokentype token);

int get_next_token(char *src, token_t *dst) {
    char *buff = calloc(64, 1);
    bool alpha = false;
    bool digit = false;
    int i = 0;
    int cnt = 0;

    while (1) {
        if (*src == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }

        if (isalpha(*src) || *src == '_') {
            if (digit) break;

            buff[i++] = *src++;
            cnt++;
            alpha = true;
            continue;
        }

        if (isdigit(*src)) {
            buff[i++] = *src++;
            cnt++;
            if (!alpha) digit = true;
            continue;
        }

        if (*src == ' ' || *src == '\n' || *src == '\r') {
            if (alpha || digit) break;
            src++;
            cnt++;
            continue;
        }

        TOKENIZER_3CHAR_CASE('>', GREATER, '>', RSH, '=', GE);
        TOKENIZER_3CHAR_CASE('<', LESS, '<', LSH, '=', LE);
        TOKENIZER_3CHAR_CASE('-', SUB, '-', DECREMENT, '>', PTR_MEMB_ACCESS);

        TOKENIZER_2CHAR_CASE('|', OR, '|', LOGICAL_OR);
        TOKENIZER_2CHAR_CASE('&', AND, '&', LOGICAL_AND);
        TOKENIZER_2CHAR_CASE('+', ADD, '+', INCREMENT);
        TOKENIZER_2CHAR_CASE('-', SUB, '-', DECREMENT);
        TOKENIZER_2CHAR_CASE('=', ASSIGN, '=', EQUAL);
        TOKENIZER_2CHAR_CASE('!', LOGICAL_NOT, '=', NE);

        TOKENIZER_CHAR_CASE(',', COMMA);
        TOKENIZER_CHAR_CASE('.', MEMB_ACCESS);
        TOKENIZER_CHAR_CASE('*', STAR);

        TOKENIZER_CHAR_CASE('~', NOT);
        TOKENIZER_CHAR_CASE('^', XOR);
        TOKENIZER_CHAR_CASE('(', LPAR);
        TOKENIZER_CHAR_CASE(')', RPAR);
        TOKENIZER_CHAR_CASE('{', LBRACE);
        TOKENIZER_CHAR_CASE('}', RBRACE);
        TOKENIZER_CHAR_CASE('[', LBRACKET);
        TOKENIZER_CHAR_CASE(']', RBRACKET);
        TOKENIZER_CHAR_CASE(';', SEMICOLON);

        TOKENIZER_CHAR_CASE(0, EOF);
    }

    if (!strcmp(buff, "int")) dst->type = TOKEN_INT;
    SET_BUFF_TYPE("if", IF);
    SET_BUFF_TYPE("else", ELSE);
    SET_BUFF_TYPE("while", WHILE);
    SET_BUFF_TYPE("for", FOR);
    SET_BUFF_TYPE("return", RETURN);
    SET_BUFF_TYPE("void", VOID);
    SET_BUFF_TYPE("char", CHAR);
    SET_BUFF_TYPE("sizeof", SIZEOF);
    SET_BUFF_TYPE("break", BREAK);
    SET_BUFF_TYPE("continue", CONTINUE);
    SET_BUFF_TYPE("struct", STRUCT);
    else {
        if (digit) {
            dst->type = TOKEN_NUM;
            dst->val = atoi(buff);
        }
        else {
            dst->type = TOKEN_IDENT;
            strcpy(dst->text, buff);
        }
    }

    free(buff);
    return cnt;
}

uint32_t tokenize(char *p, token_t *dst) {
    token_t crt;

    int i = 0;
    while (crt.type != TOKEN_EOF) {
        memset(&crt, 0, sizeof(token_t));
        int c = get_next_token(p, &crt);
        
        dst[i++] = crt;

        p += c;
    }

    return i;
}

token_t *arr;

token_t get_current_token() {
    return *arr;
}

void consume() {
    // printf("DEBUG: Consuming %s\n", token_to_string(CRT_TYPE));
    arr++;
    return;
}

void expect(enum tokentype token) {
    if ((*arr).type != token) {
        printf(
            "%s:%d:%d: Syntax error: Expected %s, got %s\n",
            filename, CRT_LINE, CRT_COLUMN,
            token_to_string(token),
            token_to_string(CRT_TYPE)
        );
    }

    return;
}

void expect_and_consume(enum tokentype token) {
    expect(token);
    consume();
}