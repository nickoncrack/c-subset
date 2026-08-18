#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ast.h>
#include <tokenizer.h>


#define ADDRESS_WIDTH   4


#define isdigit(c) (c >= '0' && c <= '9')
#define isalpha(c) ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))

#define SET_BUFF_TYPE(lc, uc) else if (!strcmp(buff, lc)) dst->type = TOKEN_##uc
#define TOKENIZER_CHAR_CASE(c, uc) \
    if (*src == c) { \
        if (alpha || digit) break; \
        src++; \
        dst->type = TOKEN_##uc; \
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
            free(buff); \
            return cnt + 2; \
        } else if (*src == c3) { \
            src++; \
            dst->type = TOKEN_##uc3; \
            free(buff); \
            return cnt + 2; \
        } \
        dst->type = TOKEN_##uc; \
        free(buff); \
        return cnt + 1; \
    }

#define PRINT_CASE(c) \
    case c: { \
        printf(#c "\n"); \
        break; \
    }

#define REALLOC(mem, size) mem = realloc(mem, size)


int get_next_token(char *src, token_t *dst) {
    char *buff = calloc(64, 1);
    bool alpha = false;
    bool digit = false;
    int i = 0;
    int cnt = 0;

    while (1) {
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

/*
    todo:
    implement a base pointer. Each function has it's own bp
    before calling the arguments are pushed to that bp value and right after
    the local variables are stored. Symbols have their own offset and IF they
    point to a local variable, during compilation it would be replaced by
    [bp - offset]

    Possibly, implement a .bss section for uninitialized global variables
    and move initialized global variables to .data
*/


void print_token(enum tokentype token);

char program[4096];
token_t *arr;

#define CRT_TYPE            get_current_token().type
#define CRT_VAL             get_current_token().val
#define CRT_TEXT            get_current_token().text
#define OFFSET_CRT_TYPE(n)  (*(arr + n)).type

#define IS_CRT_TYPE         ((CRT_TYPE == TOKEN_VOID) || (CRT_TYPE == TOKEN_INT) || (CRT_TYPE == TOKEN_CHAR))

token_t get_current_token() {
    return *arr;
}

void consume() {
    printf("DEBUG: Consuming ");
    print_token(CRT_TYPE);
    arr++;
    return;
}

void expect(enum tokentype token) {
    if ((*arr).type != token) {
        printf("Syntax error: Expected ");
        print_token(token);
    }

    return;
}

void expect_and_consume(enum tokentype token) {
    expect(token);
    consume();
}

Type *parse_type() {
    if ((int) CRT_TYPE >= 3 && CRT_TYPE != TOKEN_STRUCT) {
        fprintf(stderr, "Syntax error: Expected type declaration, got %d", CRT_TYPE);
        return NULL;
    }

    Type *type = malloc(sizeof(Type));
    Type *root_type = type;
    char *struct_name = NULL;

    enum data_type final;

    if (CRT_TYPE == TOKEN_STRUCT) {
        final = TYPE_STRUCT;
        
        consume(); // TOKEN_STRUCT
        expect(TOKEN_IDENT);
        struct_name = strdup(CRT_TEXT);
    }
    else final = (enum data_type) CRT_TYPE;

    consume();
    while (CRT_TYPE == TOKEN_STAR) {
        type->type = TYPE_POINTER;
        type->pointee = malloc(sizeof(Type));

        type = type->pointee;
        consume();
    }

    type->type = final;
    if (final == TYPE_STRUCT) {
        type->structure.name = struct_name;
    }

    return root_type;
}

uint32_t sizeof_type(Type *t) {
    if (t == NULL) return -1;

    switch (t->type) {
        case TYPE_VOID: {
            return -1; // void has no size
        }

        case TYPE_INT: {
            return 4;
        }

        case TYPE_CHAR: {
            return 1;
        }

        case TYPE_POINTER:
        case TYPE_FUNCTION: {
            return ADDRESS_WIDTH;
        }

        case TYPE_ARRAY: {
            return t->array.count * sizeof_type(t->array.memb_type);
        }

        case TYPE_STRUCT: {
            uint32_t ret = 0;
            for (int i = 0; i < t->structure.count; i++) {
                ret += sizeof_type(t->structure.memb_types[i]);
            }

            return ret;
        }
    }
}

AST_node *parse_factor();
AST_node *parse_term();
AST_node *parse_bitwise_not();
AST_node *parse_bitwise_shifts();
AST_node *parse_bitwise_and();
AST_node *parse_bitwise_xor();


AST_node *parse_logical_expression() {
    AST_node *left = parse_bitwise_operations();

    while (CRT_TYPE == TOKEN_LOGICAL_OR || CRT_TYPE == TOKEN_LOGICAL_AND) {
        enum binop_operator op = (enum binop_operator) (CRT_TYPE - TOKEN_ADD);
        consume();

        AST_node *right = parse_bitwise_operations();
        AST_node *bin_node = malloc(sizeof(AST_node));
        bin_node->type = AST_BINARY_OP;
        bin_node->as.binary_op.op = op;
        bin_node->as.binary_op.left = left;
        bin_node->as.binary_op.right = right;

        left = bin_node;
    }

    return left;
}

AST_node *parse_bitwise_operations() {
    AST_node *left = parse_bitwise_xor();

    while (CRT_TYPE == TOKEN_OR) {
        consume();

        AST_node *right = parse_bitwise_xor();
        AST_node *bin_node = malloc(sizeof(AST_node));
        bin_node->type = AST_BINARY_OP;
        bin_node->as.binary_op.op = OP_OR;
        bin_node->as.binary_op.left = left;
        bin_node->as.binary_op.right = right;

        left = bin_node;
    }

    return left;
}

AST_node *parse_bitwise_xor() {
    AST_node *left = parse_bitwise_and();

    while (CRT_TYPE == TOKEN_XOR) {
        consume();

        AST_node *right = parse_bitwise_and();
        AST_node *bin_node = malloc(sizeof(AST_node));
        bin_node->type = AST_BINARY_OP;
        bin_node->as.binary_op.op = OP_XOR;
        bin_node->as.binary_op.left = left;
        bin_node->as.binary_op.right = right;

        left = bin_node;
    }

    return left;
}

AST_node *parse_bitwise_and() {
    AST_node *left = parse_bitwise_shifts();

    while (CRT_TYPE == TOKEN_AND) {
        consume();

        AST_node *right = parse_bitwise_shifts();
        AST_node *bin_node = malloc(sizeof(AST_node));
        bin_node->type = AST_BINARY_OP;
        bin_node->as.binary_op.op = OP_AND;
        bin_node->as.binary_op.left = left;
        bin_node->as.binary_op.right = right;

        left = bin_node;
    }

    return left;
}

AST_node *parse_bitwise_shifts() {
    AST_node *left = parse_comparison();

    while (CRT_TYPE == TOKEN_LSH || CRT_TYPE == TOKEN_RSH) {
        enum binop_operator op = (enum binop_operator) (CRT_TYPE - TOKEN_ADD);
        consume();

        AST_node *right = parse_comparison();
        AST_node *bin_node = malloc(sizeof(AST_node));
        bin_node->type = AST_BINARY_OP;
        bin_node->as.binary_op.op = op;
        bin_node->as.binary_op.left = left;
        bin_node->as.binary_op.right = right;

        left = bin_node;
    }

    return left;
}


/*
    expressions are parsed recursively, each precedence layer has its own function,
    so when parsing an expression, the order of operations is automatically applied
*/

AST_node *parse_comparison() {
    AST_node *left = parse_expression();

    if (CRT_TYPE == TOKEN_LESS || CRT_TYPE == TOKEN_GREATER || CRT_TYPE == TOKEN_EQUAL || \
        CRT_TYPE == TOKEN_LE || CRT_TYPE == TOKEN_GE || CRT_TYPE == TOKEN_NE
    ) {
        enum binop_operator op = (enum binop_operator) (CRT_TYPE - TOKEN_ADD);
        consume();

        AST_node *right = parse_expression();

        AST_node *bin_node = malloc(sizeof(AST_node));
        bin_node->type = AST_BINARY_OP;
        bin_node->as.binary_op.op = op;
        bin_node->as.binary_op.left = left;
        bin_node->as.binary_op.right = right;

        left = bin_node;
    }

    return left;
}

// addition/subtraction
AST_node *parse_expression() {
    AST_node *left = parse_term();
    
    while (CRT_TYPE == TOKEN_ADD || CRT_TYPE == TOKEN_SUB) {
        enum binop_operator op = (enum binop_operator) (CRT_TYPE - TOKEN_ADD);
        consume();

        AST_node *right = parse_term();

        AST_node *bin_node = malloc(sizeof(AST_node));
        bin_node->type = AST_BINARY_OP;
        bin_node->as.binary_op.op = op;
        bin_node->as.binary_op.left = left;
        bin_node->as.binary_op.right = right;

        left = bin_node;
    }

    return left;
}

// multiplication
AST_node *parse_term() {
    AST_node *left = parse_factor();

    while (CRT_TYPE == TOKEN_STAR) {
        enum binop_operator op = OP_STAR;
        consume();

        AST_node *right = parse_factor();

        AST_node *bin_node = malloc(sizeof(AST_node));
        bin_node->type = AST_BINARY_OP;
        bin_node->as.binary_op.op = op;
        bin_node->as.binary_op.left = left;
        bin_node->as.binary_op.right = right;

        left = bin_node;
    }

    return left;
}

AST_node *parse_primary() {
    if (CRT_TYPE == TOKEN_NUM) {
        AST_node *node = malloc(sizeof(AST_node));
        node->type = AST_INT_LITERAL;
        node->as.int_literal.value = CRT_VAL;

        consume();
        return node;
    } else if (CRT_TYPE == TOKEN_IDENT) {
        AST_node *node = malloc(sizeof(AST_node));
        node->type = AST_VAR_REF;
        node->as.var_ref.name = strdup(CRT_TEXT);

        consume();
        return node;
    } else if (CRT_TYPE == TOKEN_LPAR) {
        consume();
        AST_node *node = parse_logical_expression();
        expect_and_consume(TOKEN_RPAR);

        return node;
    }

    return NULL;
}

AST_node *parse_factor() {
    // prefix operators
    if (CRT_TYPE == TOKEN_NOT || CRT_TYPE == TOKEN_LOGICAL_NOT) {
        AST_node *ret = malloc(sizeof(AST_node));
        ret->type = AST_UNARY_OP;
        ret->as.unary_op.prefix = true;
        ret->as.unary_op.op = (CRT_TYPE == TOKEN_NOT) ? OP_NOT : OP_LOGICAL_NOT;

        consume();
        ret->as.unary_op.operand = parse_factor(); // allows stuff like ~~x or !!x
        return ret;
    } else if (CRT_TYPE == TOKEN_SIZEOF) {
        consume();
        expect_and_consume(TOKEN_LPAR);

        AST_node *ret = malloc(sizeof(AST_node));
        ret->type = AST_UNARY_OP;
        ret->as.unary_op.op = OP_SIZEOF;
        ret->as.unary_op.prefix = true;
        ret->as.unary_op.operand = parse_primary();

        expect_and_consume(TOKEN_RPAR);
        return ret;
    }

    return parse_postfix_expression();
}

AST_node *parse_assignment() {
    if (IS_CRT_TYPE) {
        AST_node *node = malloc(sizeof(AST_node));
        node->type = AST_VAR_DECL;
        node->as.var_decl.type = parse_type();

        expect(TOKEN_IDENT);
        node->as.var_decl.name = strdup(CRT_TEXT);
        consume();

        if (CRT_TYPE == TOKEN_ASSIGN) {
            consume();
            node->as.var_decl.init = parse_logical_expression();
        } else {
            node->as.var_decl.init = NULL;
        }

        return node;
    }

    perror("Syntax error\n");
    return NULL;
}

// this function assumes the type has already been parsed
AST_node *__parse_function_decl_internal(Type *t) {
    AST_node *node = malloc(sizeof(AST_node));
    node->type = AST_FUNCTION_DECL;
    node->as.function_decl.type = t;

    expect(TOKEN_IDENT);
    node->as.function_decl.name = strdup(CRT_TEXT);
    consume();

    expect_and_consume(TOKEN_LPAR);
    
    int argc = 0;
    int argv_max = 8;
    AST_node **argv = (AST_node **) calloc(argv_max, sizeof(AST_node*));
    while (CRT_TYPE != TOKEN_RPAR) {
        if (IS_CRT_TYPE) {
            if (argc == argv_max) {
                argv_max += 2;
                REALLOC(argv, argv_max * sizeof(AST_node*));
            }

            argv[argc] = (AST_node *) malloc(sizeof(AST_node));

            argv[argc]->type = AST_VAR_DECL;
            argv[argc]->as.var_decl.type = parse_type();
            argv[argc]->as.var_decl.init = NULL;

            expect(TOKEN_IDENT);
            argv[argc]->as.var_decl.name = strdup(CRT_TEXT);
            consume(); // TOKEN_IDENT

            argc++;
            if (CRT_TYPE != TOKEN_RPAR && CRT_TYPE != TOKEN_COMMA) {
                perror("Syntax error\n");
                return NULL;
            }

            if (CRT_TYPE == TOKEN_COMMA) {
                consume(); // TOKEN_COMMA
                if (CRT_TYPE == TOKEN_RPAR) {
                    printf("Warning: trailing comma\n");
                }
            }
        } else {
            perror("Syntax error\n");
            return NULL;
        }
    }

    if (argc < argv_max && argc > 0) {
        REALLOC(argv, argc * sizeof(AST_node*));
    } else if (argc == 0) {
        free(argv);
        argv = NULL;
    }

    consume(); // TOKEN_RPAR

    node->as.function_decl.args = argv;
    node->as.function_decl.count = argc;
    node->as.function_decl.body = parse_block();

    return node;
}

AST_node *parse_function_decl() {
    return __parse_function_decl_internal(parse_type());
}

AST_node **parse_call_args(int *argc) {
    expect_and_consume(TOKEN_LPAR);

    *argc = 0;
    int argv_max = 8;
    AST_node **argv = (AST_node **) calloc(8, sizeof(AST_node*));
    while (CRT_TYPE != TOKEN_RPAR) {
        if (*argc == argv_max) {
            argv_max += 2;
            REALLOC(argv, argv_max * sizeof(AST_node*));
        }

        argv[(*argc)++] = parse_logical_expression();

        if (CRT_TYPE != TOKEN_RPAR && CRT_TYPE != TOKEN_COMMA) {
            fprintf(stderr, "Syntax error: Expected comma or closing parenthesis\n");
            return NULL;
        }

        if (CRT_TYPE == TOKEN_COMMA) consume();
    }

    if (*argc > 0 && *argc < argv_max) {
        REALLOC(argv, *argc * sizeof(AST_node*));
    } else if (*argc == 0) {
        free(argv);
        argv = NULL;
    }

    consume(); // TOKEN_RPAR

    return argv;
}

void print_child(AST_node *);
AST_node *parse_postfix_expression() {
    AST_node *node = parse_primary();

    while (1) {
        if (CRT_TYPE == TOKEN_LPAR) { // function call
            AST_node *call_node = malloc(sizeof(AST_node));
            call_node->type = AST_FUNCTION_CALL;
            call_node->as.function_call.callee = node;

            call_node->as.function_call.args = parse_call_args(
                &call_node->as.function_call.count
            );

            node = call_node; // allows for call chaining like f(x)()
        } else if (CRT_TYPE == TOKEN_MEMB_ACCESS || CRT_TYPE == TOKEN_PTR_MEMB_ACCESS) {
            AST_node *access_node = malloc(sizeof(AST_node));
            access_node->type = AST_STRUCT_ACCESS;
            access_node->as.struct_access.pointer = CRT_TYPE == TOKEN_PTR_MEMB_ACCESS;
            access_node->as.struct_access.src = node;
            
            consume();
            expect(TOKEN_IDENT);
            access_node->as.struct_access.member = strdup(CRT_TEXT);
            consume();

            node = access_node;
        } else if (CRT_TYPE == TOKEN_LBRACKET) {
            AST_node *array = malloc(sizeof(AST_node));
            array->type = AST_ARRAY_ACCESS;
            array->as.array_access.array = node;

            consume(); // TOKEN_LBRACKET
            array->as.array_access.index = parse_logical_expression();

            expect_and_consume(TOKEN_RBRACKET);
            node = array;
        } else if (CRT_TYPE == TOKEN_INCREMENT || CRT_TYPE == TOKEN_DECREMENT) {
            AST_node *op = malloc(sizeof(AST_node));
            op->type = AST_UNARY_OP;
            op->as.unary_op.op = (enum unary_operator) (CRT_TYPE - TOKEN_INCREMENT);
            op->as.unary_op.prefix = false;
            op->as.unary_op.operand = node;

            consume();
            node = op;
        } else {
            break;
        }
    }

    return node;
}


AST_node *parse_if_statement() {
    consume();
    expect_and_consume(TOKEN_LPAR);

    AST_node *node = malloc(sizeof(AST_node));
    node->type = AST_IF;
    node->as.if_statement.condition = parse_logical_expression();
    expect_and_consume(TOKEN_RPAR);

    if (CRT_TYPE != TOKEN_LBRACE) {
        node->as.if_statement.body = parse_statement(false);
    } else {
        node->as.if_statement.body = parse_block();
    }

    node->as.if_statement.else_branch = NULL;
    return node;
}

AST_node *parse_return() {
    expect_and_consume(TOKEN_RETURN);

    AST_node *node = malloc(sizeof(AST_node));
    node->type = AST_RETURN;

    if (CRT_TYPE != TOKEN_SEMICOLON) {
        node->as.return_statement.expr = parse_logical_expression();
    } else {
        node->as.return_statement.expr = NULL;
    }

    return node;
}

AST_node *parse_while() {
    expect_and_consume(TOKEN_WHILE);

    AST_node *node = malloc(sizeof(AST_node));
    node->type = AST_WHILE;
    
    expect_and_consume(TOKEN_LPAR);
    node->as.while_statement.condition = parse_logical_expression();
    expect_and_consume(TOKEN_RPAR);

    node->as.while_statement.body = parse_block();

    return node;
}

AST_node *parse_for() {
    expect_and_consume(TOKEN_FOR);
    expect_and_consume(TOKEN_LPAR);

    AST_node *node = malloc(sizeof(AST_node));
    node->type = AST_FOR;
    node->as.for_statement.init = parse_statement(true);
    node->as.for_statement.condition = parse_logical_expression();

    expect_and_consume(TOKEN_SEMICOLON);
    node->as.for_statement.updation = parse_statement(false);

    expect_and_consume(TOKEN_RPAR);
    node->as.for_statement.body = parse_block();

    return node;
}

AST_node *__parse_decl_internal(Type *t, bool is_struct) {
    AST_node *node = malloc(sizeof(AST_node));
    node->type = AST_VAR_DECL;
    node->as.var_decl.type = t;
    node->as.var_decl.name = strdup(CRT_TEXT);
    
    consume(); // TOKEN_IDENT

    if (CRT_TYPE == TOKEN_ASSIGN) {
        if (is_struct) {
            fprintf(stderr, "Syntax error: Assignments are not allowed inside a struct\n");
            free(node->as.var_decl.name);
            free(node);

            return NULL;
        }

        consume();
        node->as.var_decl.init = parse_logical_expression();
    } else {
        node->as.var_decl.init = NULL;
    }

    return node;
}

Type *__deep_copy_type(Type *t) {
    if (t == NULL) return NULL;

    Type *ret = malloc(sizeof(Type));
    ret->type = t->type;

    switch (t->type) {
        case TYPE_VOID:
        case TYPE_INT:
        case TYPE_CHAR: {
            break;
        }

        case TYPE_POINTER: {
            ret->pointee = __deep_copy_type(t->pointee);
            break;
        }

        case TYPE_FUNCTION: {
            ret->function.count = t->function.count;
            ret->function.return_type = __deep_copy_type(t->function.return_type);
            ret->function.params = calloc(t->function.count, sizeof(Type*));

            for (int i = 0; i < t->function.count; i++) {
                ret->function.params[i] = __deep_copy_type(t->function.params[i]);
            }
            break;
        }

        case TYPE_ARRAY: {
            ret->array.count = t->array.count;
            ret->array.memb_type = __deep_copy_type(t->array.memb_type);
            break;
        }

        case TYPE_STRUCT: {
            ret->structure.count = t->structure.count;
            ret->structure.name = strdup(t->structure.name);
            ret->structure.memb_names = calloc(t->structure.count, sizeof(char*));
            ret->structure.memb_types = calloc(t->structure.count, sizeof(Type*));

            for (int i = 0; i < t->structure.count; i++) {
                ret->structure.memb_names[i] = strdup(t->structure.memb_names[i]);
                ret->structure.memb_types[i] = __deep_copy_type(t->structure.memb_types[i]);
            }
            break;
        }
    }

    return ret;
}

AST_node *__deep_copy_node(AST_node *node) {
    if (node == NULL) return NULL;

    AST_node *ret = malloc(sizeof(AST_node));
    ret->type = node->type;

    switch (node->type) {
        case AST_INT_LITERAL: {
            ret->as.int_literal = node->as.int_literal;
            break;
        }

        case AST_VAR_REF: {
            ret->as.var_ref.name = strdup(node->as.var_ref.name);
            break;
        }

        case AST_VAR_DECL: {
            ret->as.var_decl.init = __deep_copy_node(node->as.var_decl.init);
            ret->as.var_decl.name = strdup(node->as.var_decl.name);
            ret->as.var_decl.type = __deep_copy_type(node->as.var_decl.type);
            break;
        }

        case AST_BINARY_OP: {
            ret->as.binary_op.op = node->as.binary_op.op;
            ret->as.binary_op.left = __deep_copy_node(node->as.binary_op.left);
            ret->as.binary_op.right = __deep_copy_node(node->as.binary_op.right);
            break;
        }

        case AST_UNARY_OP: {
            ret->as.unary_op.op = node->as.unary_op.op;
            ret->as.unary_op.prefix = node->as.unary_op.prefix;
            ret->as.unary_op.operand = __deep_copy_node(node->as.unary_op.operand);
            break;
        }

        case AST_FUNCTION_CALL: {
            ret->as.function_call.callee = __deep_copy_node(node->as.function_call.callee);
            ret->as.function_call.count = node->as.function_call.count;
            ret->as.function_call.args = calloc(node->as.function_call.count, sizeof(AST_node*));

            for (int i = 0; i < node->as.function_call.count; i++) {
                ret->as.function_call.args[i] = __deep_copy_node(node->as.function_call.args[i]);
            }
            break;
        }

        case AST_STRUCT_ACCESS: {
            ret->as.struct_access.pointer = node->as.struct_access.pointer;
            ret->as.struct_access.member = strdup(node->as.struct_access.member);
            ret->as.struct_access.src = __deep_copy_node(node->as.struct_access.src);
            break;
        }

        case AST_ARRAY_ACCESS: {
            ret->as.array_access.array = __deep_copy_node(node->as.array_access.array);
            ret->as.array_access.index = __deep_copy_node(node->as.array_access.index);
            break;
        }
    }

    return ret;
}

AST_node *parse_statement(bool expect_semicolon) {
    AST_node *node = NULL;

    switch (CRT_TYPE) {
        // variable or function declaration
        case TOKEN_VOID:
        case TOKEN_INT:
        case TOKEN_CHAR: {
            Type *t = parse_type(); // both cases start with a type declaration
            expect(TOKEN_IDENT);
            
            if (OFFSET_CRT_TYPE(1) == TOKEN_ASSIGN || OFFSET_CRT_TYPE(1) == TOKEN_SEMICOLON) {
                node = __parse_decl_internal(t, false);
            } else if (OFFSET_CRT_TYPE(1) == TOKEN_LPAR) {
                node = __parse_function_decl_internal(t);
            }

            break;
        }

        case TOKEN_IDENT: {
            /*
                The only cases where the first token in a line of code is an identifier would be:
                1. Direct reassignment (x = a + b)
                2. Augmented assignment (x += a + b)
                3. Array element modification (arr[a] = x)
                4. Function calling (func(x, y))
                5. Increment/decrement (x++)
                6. Struct member access combined with any of the above
            */

            AST_node *left = parse_postfix_expression();
            if (left->type == AST_FUNCTION_CALL || left->type == AST_UNARY_OP) {
                // function call or increment/decrement
                node = left;
                break;
            }

            if (CRT_TYPE == TOKEN_ASSIGN) {
                consume();

                node = malloc(sizeof(AST_node));
                node->type = AST_BINARY_OP;
                node->as.binary_op.op = OP_ASSIGN;
                node->as.binary_op.left = left;
                node->as.binary_op.right = parse_logical_expression();

                break;
            } else if (CRT_TYPE - TOKEN_ADD <= OP_ASSIGN) {
                /*
                    Augmented assignment will just be transformed into a reassignment.
                    i.e. x (op)= b is transformed into x = x (op) (b)
                */

                AST_node *node_right = __deep_copy_node(left);

                AST_node *right = malloc(sizeof(AST_node));
                right->type = AST_BINARY_OP;
                right->as.binary_op.op = CRT_TYPE - TOKEN_ADD;
                right->as.binary_op.left = node_right;
                
                consume(); // operator
                consume(); // TOKEN_ASSIGN
                right->as.binary_op.right = parse_logical_expression();

                node = malloc(sizeof(AST_node));
                node->type = AST_BINARY_OP;
                node->as.binary_op.op = OP_ASSIGN;
                node->as.binary_op.left = left;
                node->as.binary_op.right = right;

                break;
            }
        }

        case TOKEN_NUM:
        case TOKEN_NOT:
        case TOKEN_LOGICAL_NOT:
        case TOKEN_SIZEOF: {
            printf("Warning: Expression result unused\n");
            break;
        }

        case TOKEN_IF: {
            node = parse_if_statement();
            break;
        }

        case TOKEN_WHILE: {
            node = parse_while();
            break;
        }

        case TOKEN_FOR: {
            node = parse_for();
            break;
        }

        case TOKEN_RETURN: {
            node = parse_return();
            break;
        }

        // ++x
        case TOKEN_INCREMENT:
        case TOKEN_DECREMENT: {
            node = malloc(sizeof(AST_node));
            node->type = AST_UNARY_OP;
            node->as.unary_op.op = (enum unary_operator) (CRT_TYPE - TOKEN_INCREMENT);
            node->as.unary_op.prefix = true;

            consume(); // TOKEN_INCREMENT
            expect(TOKEN_IDENT);
            AST_node *var_ref = malloc(sizeof(AST_node));
            var_ref->type = AST_VAR_REF;
            var_ref->as.var_ref.name = strdup(CRT_TEXT);

            node->as.unary_op.operand = var_ref;
            consume(); // TOKEN_IDENT
            break;
        }

        case TOKEN_BREAK:
        case TOKEN_CONTINUE: {
            node = malloc(sizeof(AST_node));
            node->type = (CRT_TYPE == TOKEN_BREAK) ? AST_BREAK : AST_CONTINUE;

            consume();
            break;
        }

        case TOKEN_STRUCT: {
            /*
                variable declaration or struct definition

                variable declaration has 2 cases:
                1. struct s variable = ...
                2. anonymous struct as variable type, which will be implemented later

                The easy way to distinguish between the 2 is checking the third token, if its a left bracket
                then its a struct declaration: struct s { ... }
            */

            if (OFFSET_CRT_TYPE(2) == TOKEN_LBRACE) {
                node = malloc(sizeof(AST_node));
                node->type = AST_STRUCT_DECL;

                consume(); // TOKEN_STRUCT
                expect(TOKEN_IDENT);
                node->as.struct_decl.name = strdup(CRT_TEXT);

                consume(); // TOKEN_IDENT
                consume(); // TOKEN_LBRACE

                int capacity = 8;
                int count = 0;
                node->as.struct_decl.memb_decl = (AST_node **) calloc(capacity, sizeof(AST_node*));
                while (CRT_TYPE != TOKEN_RBRACE) {
                    if (count == capacity) {
                        capacity += 2;
                        REALLOC(node->as.struct_decl.memb_decl, capacity * sizeof(AST_node*));
                    }

                    Type *t = parse_type();
                    node->as.struct_decl.memb_decl[count++] = __parse_decl_internal(t, true);
                    expect_and_consume(TOKEN_SEMICOLON);
                }

                if (count < capacity) {
                    REALLOC(node->as.struct_decl.memb_decl, count * sizeof(AST_node*));
                }

                consume(); // TOKEN_RBRACE
                node->as.struct_decl.count = count;

                /*
                    A lazy workaround for the following issue that arises:
                    struct definitions require a semicolon at the end of their definition.

                    However, at the end of this function, there is the rule: if the previous token
                    is a right brace, meaning a block was just parsed, don't expect a semicolon. But,
                    at the end of the struct definition there is both a right brace and a semicolon so the function,
                    would not consume it. Also, just consuming the token here isn't enough, if break was used instead of
                    return, at the end of the function, it would check the previous token and it would be a 
                    semicolon instead of a right brace so it would expext another semicolon.
                */
                expect_and_consume(TOKEN_SEMICOLON);
                return node;
            } else {
                /*
                    Since variable declarations with a struct type (that may or may not has been declared)
                    i.e. struct s *ptr, where s is undefined, is valid C code.

                    For this reason, a type table is not created during parsing to store all defined structs.
                    Instead, whenever a variable is declared with a struct type, its type in the AST, will contain
                    the referenced struct name, but memb_types = memb_names = NULL and count = 0.
                */
                node = __parse_decl_internal(parse_type(), false);
                break;
            }
        }

        case TOKEN_ELSE:
        case TOKEN_ADD:
        case TOKEN_SUB:
        case TOKEN_LESS:
        case TOKEN_GREATER:
        case TOKEN_EQUAL:
        case TOKEN_LE:
        case TOKEN_GE:
        case TOKEN_NE:
        case TOKEN_LSH:
        case TOKEN_RSH:
        case TOKEN_AND:
        case TOKEN_XOR:
        case TOKEN_OR:
        case TOKEN_LOGICAL_AND:
        case TOKEN_LOGICAL_OR:
        case TOKEN_ASSIGN:
        case TOKEN_LPAR:
        case TOKEN_RPAR:
        case TOKEN_LBRACE:
        case TOKEN_RBRACE:
        case TOKEN_LBRACKET:
        case TOKEN_RBRACKET:
        case TOKEN_SEMICOLON:
        case TOKEN_COMMA: {
            printf("Syntax error: Unexpected token ");
            print_token(CRT_TYPE);
            break;
        }
    }

    // a block was just parsed, therefore a semicolon is not required
    if ((*(arr - 1)).type != TOKEN_RBRACE && expect_semicolon) {
        expect_and_consume(TOKEN_SEMICOLON);
    }

    return node;
}

AST_node *parse_block() {
    expect_and_consume(TOKEN_LBRACE);

    AST_node *node = malloc(sizeof(AST_node));
    node->type = AST_BLOCK;
    node->as.block.statements = (AST_node **) calloc(8, sizeof(AST_node*));
    node->as.block.count = 0;

    int capacity = 8;

    while (CRT_TYPE != TOKEN_RBRACE) {
        if (node->as.block.count == capacity) {
            capacity += 8;
            REALLOC(node->as.block.statements, capacity * sizeof(AST_node*));
        }

        if (CRT_TYPE == TOKEN_EOF) {
            fprintf(stderr, "Syntax error: Expected closing brace\n");
            return NULL;
        }

        node->as.block.statements[node->as.block.count++] = parse_statement(true);
    }

    if (node->as.block.count < capacity) {
        REALLOC(node->as.block.statements, node->as.block.count * sizeof(AST_node*));
    }

    consume(); // TOKEN_RBRACE
    return node;
}

AST_node *parse_program() {
    AST_node *node = malloc(sizeof(AST_node));
    int capacity = 8;

    node->type = AST_PROGRAM;
    node->as.program.count = 0;
    node->as.program.declarations = (AST_node **) calloc(capacity, sizeof(AST_node*));

    while (CRT_TYPE != TOKEN_EOF) {
        if (node->as.program.count == capacity) {
            capacity += 8;
            REALLOC(node->as.program.declarations, capacity * sizeof(AST_node*));
        }

        AST_node *statement = parse_statement(true);
        if (statement->type == AST_VAR_DECL || statement->type == AST_FUNCTION_DECL || statement->type == AST_STRUCT_DECL) {
            node->as.program.declarations[node->as.program.count++] = statement;
            continue;
        }

        // statement is not a declaration
        fprintf(stderr, "Syntax error: Only declarations are allowed in the global scope\n");
        return NULL;
    }

    if (node->as.program.count < capacity) {
        REALLOC(
            node->as.program.declarations, 
            node->as.program.count * sizeof(AST_node*)
        );
    }

    return node;
}

void __print_tabs(int depth) {
    for (int i = 0; i < depth; i++) {
        printf("    ");
    }

    return;
}

void print_token(enum tokentype token) {
    switch (token) {
        PRINT_CASE(TOKEN_VOID);
        PRINT_CASE(TOKEN_INT);
        PRINT_CASE(TOKEN_CHAR);
        PRINT_CASE(TOKEN_IDENT);
        PRINT_CASE(TOKEN_NUM);
        PRINT_CASE(TOKEN_IF);
        PRINT_CASE(TOKEN_ELSE);
        PRINT_CASE(TOKEN_WHILE);
        PRINT_CASE(TOKEN_FOR);
        PRINT_CASE(TOKEN_RETURN);
        PRINT_CASE(TOKEN_ADD);
        PRINT_CASE(TOKEN_SUB);
        PRINT_CASE(TOKEN_STAR);
        PRINT_CASE(TOKEN_LESS);
        PRINT_CASE(TOKEN_GREATER);
        PRINT_CASE(TOKEN_EQUAL);
        PRINT_CASE(TOKEN_LE);
        PRINT_CASE(TOKEN_GE);
        PRINT_CASE(TOKEN_NE);
        PRINT_CASE(TOKEN_NOT);
        PRINT_CASE(TOKEN_LSH);
        PRINT_CASE(TOKEN_RSH);
        PRINT_CASE(TOKEN_AND);
        PRINT_CASE(TOKEN_XOR);
        PRINT_CASE(TOKEN_OR);
        PRINT_CASE(TOKEN_LOGICAL_NOT);
        PRINT_CASE(TOKEN_LOGICAL_AND);
        PRINT_CASE(TOKEN_LOGICAL_OR);
        PRINT_CASE(TOKEN_INCREMENT);
        PRINT_CASE(TOKEN_DECREMENT);
        PRINT_CASE(TOKEN_ASSIGN);
        PRINT_CASE(TOKEN_LPAR);
        PRINT_CASE(TOKEN_RPAR);
        PRINT_CASE(TOKEN_LBRACE);
        PRINT_CASE(TOKEN_RBRACE);
        PRINT_CASE(TOKEN_LBRACKET);
        PRINT_CASE(TOKEN_RBRACKET);
        PRINT_CASE(TOKEN_SEMICOLON);
        PRINT_CASE(TOKEN_COMMA);
        PRINT_CASE(TOKEN_SIZEOF);
        PRINT_CASE(TOKEN_BREAK);
        PRINT_CASE(TOKEN_CONTINUE);
        PRINT_CASE(TOKEN_STRUCT);
        PRINT_CASE(TOKEN_MEMB_ACCESS);
        PRINT_CASE(TOKEN_PTR_MEMB_ACCESS);
        PRINT_CASE(TOKEN_EOF);
    }
}

void print_binary_operator(enum binop_operator op) {
    switch (op) {
        PRINT_CASE(OP_ADD);
        PRINT_CASE(OP_SUB);
        PRINT_CASE(OP_STAR);
        PRINT_CASE(OP_LESS);
        PRINT_CASE(OP_GREATER);
        PRINT_CASE(OP_EQUAL);
        PRINT_CASE(OP_LE);
        PRINT_CASE(OP_GE);
        PRINT_CASE(OP_NE);
        PRINT_CASE(OP_LSH);
        PRINT_CASE(OP_RSH);
        PRINT_CASE(OP_AND)
        PRINT_CASE(OP_XOR);
        PRINT_CASE(OP_OR);
        PRINT_CASE(OP_LOGICAL_AND);
        PRINT_CASE(OP_LOGICAL_OR);
        PRINT_CASE(OP_ASSIGN);
    }
}

void print_unary_operator(enum unary_operator op) {
    switch (op) {
        PRINT_CASE(OP_INCREMENT);
        PRINT_CASE(OP_DECREMENT);
        PRINT_CASE(OP_NOT);
        PRINT_CASE(OP_LOGICAL_NOT);
        PRINT_CASE(OP_DEREFERENCE);
        PRINT_CASE(OP_POINTER);
        PRINT_CASE(OP_SIZEOF);
    }
}

void print_data_type(enum data_type type) {
    switch (type) {
        PRINT_CASE(TYPE_VOID);
        PRINT_CASE(TYPE_INT);
        PRINT_CASE(TYPE_CHAR);
        PRINT_CASE(TYPE_POINTER);
        PRINT_CASE(TYPE_FUNCTION);
        PRINT_CASE(TYPE_ARRAY);
        PRINT_CASE(TYPE_STRUCT);
    }
}

void print_type(Type *t, int d) {
    __print_tabs(d);
    printf("type: ");
    print_data_type(t->type);

    switch (t->type) {
        case TYPE_VOID:
        case TYPE_INT:
        case TYPE_CHAR: {
            break;
        }

        case TYPE_POINTER: {
            __print_tabs(d);
            printf("POINTEE\n");
            
            print_type(t->pointee, d+1);
            break;
        }

        case TYPE_FUNCTION: {
            __print_tabs(d);
            printf("RETURN_TYPE\n");
            print_type(t->function.return_type, d+1);

            if (t->function.count > 0) {
                __print_tabs(d);
                printf("ARG_TYPES(count=%d)\n", t->function.count);
                for (int i = 0; i < t->function.count; i++) {
                    print_type(t->function.params[i], d+1);
                }
            }

            break;
        }

        case TYPE_ARRAY: {
            __print_tabs(d);
            printf("member count: %d\n", t->array.count);

            __print_tabs(d);
            printf("MEMBER_TYPE\n");
            print_type(t->array.memb_type, d+1);

            break;
        }

        case TYPE_STRUCT: {
            __print_tabs(d);
            printf("struct name: %s\n", t->structure.name);

            __print_tabs(d);
            printf("STRUCT_MEMBERS(count=%d)\n", t->structure.count);
            for (int i = 0; i < t->structure.count; i++) {
                __print_tabs(d);
                printf("MEMBER %d\n", i);

                __print_tabs(d+1);
                printf("name: %s\n", t->structure.memb_names[i]);
                print_type(t->structure.memb_types[i], d+1);
            }

            break;
        }
    }

    return;
}

uint32_t depth = 0;
void print_child(AST_node *node) {
    if (node == NULL) {
        __print_tabs(depth);
        printf("not set (null)\n");
        return;
    }

    switch (node->type) {
        case AST_INT_LITERAL: {
            __print_tabs(depth);
            printf("INT_LITERAL\n");
            __print_tabs(depth+1);
            printf("value: %d\n", node->as.int_literal.value);

            break;
        }

        case AST_VAR_REF: {
            __print_tabs(depth);
            printf("VAR_REF\n");
            __print_tabs(depth+1);
            printf("name: %s\n", node->as.var_ref.name);

            break;
        }

        case AST_VAR_DECL: {
            __print_tabs(depth);
            printf("VAR_DECL\n");

            __print_tabs(++depth);
            printf("name: %s\n", node->as.var_decl.name);

            __print_tabs(depth++);
            printf("TYPE (VAR_DECL)\n");
            print_type(node->as.var_decl.type, depth);

            __print_tabs(depth-1);
            printf("INIT (VAR_DECL)\n");

            print_child(node->as.var_decl.init);
            depth -= 2;
            break;
        }

        case AST_BINARY_OP: {
            __print_tabs(depth);
            printf("BINARY_OP\n");
            __print_tabs(++depth);
            printf("operator: ");
            print_binary_operator(node->as.binary_op.op);
            
            __print_tabs(depth++);
            printf("LEFT (BINARY_OP)\n");
            print_child(node->as.binary_op.left);

            __print_tabs(depth-1);
            printf("RIGHT (BINARY_OP)\n");
            print_child(node->as.binary_op.right);

            depth -= 2;
            break;
        }

        case AST_UNARY_OP: {
            __print_tabs(depth);
            printf("UNARY_OP\n");
            __print_tabs(++depth);
            printf("operator: ");
            print_unary_operator(node->as.unary_op.op);

            __print_tabs(depth);
            printf("prefix: %s\n", node->as.unary_op.prefix ? "true" : "false");

            __print_tabs(depth++);
            printf("OPERAND (UNARY_OP)\n");
            print_child(node->as.unary_op.operand);

            depth -= 2;
            break;
        }

        case AST_IF: {
            __print_tabs(depth);
            printf("IF_STATEMENT\n");
            
            __print_tabs(++depth);
            printf("CONDITION (IF_STATEMENT)\n");
            depth++;
            print_child(node->as.if_statement.condition);

            __print_tabs(depth-1);
            printf("BODY (IF_STATEMENT)\n");
            print_child(node->as.if_statement.body);

            __print_tabs(depth-1);
            printf("ELSE_BRANCH (IF_STATEMENT)\n");
            print_child(node->as.if_statement.else_branch);

            depth -= 2;
            break;
        }

        case AST_WHILE: {
            __print_tabs(depth);
            printf("WHILE_STATEMENT\n");
            
            __print_tabs(++depth);
            printf("CONDITION (WHILE_STATEMENT)\n");
            depth++;
            print_child(node->as.while_statement.condition);

            __print_tabs(depth-1);
            printf("BODY (WHILE_STATEMENT)\n");
            print_child(node->as.while_statement.body);

            depth -= 2;
            break;
        }

        case AST_FOR: {
            __print_tabs(depth);
            printf("FOR_STATEMENT\n");

            __print_tabs(++depth);
            printf("INIT (FOR_STATEMENT)\n");
            depth++;
            print_child(node->as.for_statement.init);

            __print_tabs(depth-1);
            printf("CONDITION (FOR_STATEMENT)\n");
            print_child(node->as.for_statement.condition);

            __print_tabs(depth-1);
            printf("UPDATION (FOR_STATEMENT)\n");
            print_child(node->as.for_statement.updation);

            __print_tabs(depth-1);
            printf("BODY (FOR_STATEMENT)\n");
            print_child(node->as.for_statement.body);

            depth -= 2;
            break;
        }

        case AST_RETURN: {
            __print_tabs(depth);
            printf("RETURN\n");

            __print_tabs(++depth);
            printf("PARAMETER (RETURN)\n");
            depth++;

            print_child(node->as.return_statement.expr);
            depth -= 2;
            break;
        }

        case AST_FUNCTION_DECL: {
            __print_tabs(depth);
            printf("FUNCTION_DECL\n");

            __print_tabs(++depth);
            printf("name: %s\n", node->as.function_decl.name);

            __print_tabs(depth);
            printf("TYPE (FUNCTION_DECL)\n");

            depth++;
            print_type(node->as.function_decl.type, depth);

            __print_tabs(depth-1);

            if (node->as.function_decl.count > 0) {
                printf("ARGS (FUNCTION_DECL, count=%d)\n", node->as.function_decl.count);
                for (int i = 0; i < node->as.function_decl.count; i++) {
                    print_child(node->as.function_decl.args[i]);
                }
            } else {
                printf("ARGS (FUNCTION_DECL): none\n");
            }

            __print_tabs(depth-1);
            printf("BODY (FUNCTION_DECL)\n");
            print_child(node->as.function_decl.body);

            depth -= 2;
            break;
        }

        case AST_FUNCTION_CALL: {
            __print_tabs(depth);
            printf("FUNCTION_CALL\n");

            __print_tabs(++depth);
            printf("CALLEE (FUNCTION_CALL)\n");
            depth++;
            print_child(node->as.function_call.callee);

            __print_tabs(depth-1);
            if (node->as.function_call.count > 0) {
                printf("ARGS (FUNCTION_CALL, count=%d)\n", node->as.function_call.count);
                for (int i = 0; i < node->as.function_call.count; i++) {
                    print_child(node->as.function_call.args[i]);
                }
            } else {
                printf("ARGS (FUNCTION_CALL): none\n");
            }

            depth -= 2;
            break;
        }

        case AST_BREAK: {
            __print_tabs(depth);
            printf("BREAK\n");
            break;
        }

        case AST_CONTINUE: {
            __print_tabs(depth);
            printf("CONTINUE\n");
            break;
        }

        case AST_STRUCT_DECL: {
            __print_tabs(depth);
            printf("STRUCT_DECL (count=%d)\n", node->as.struct_decl.count);

            __print_tabs(++depth);
            printf("struct name: %s\n", node->as.struct_decl.name);

            __print_tabs(depth++);
            printf("STRUCT MEMBERS (STRUCT_DECL)\n");
            for (int i = 0; i < node->as.struct_decl.count; i++){
                print_child(node->as.struct_decl.memb_decl[i]);
            }

            depth -= 2;
            break;
        }

        case AST_STRUCT_ACCESS: {
            __print_tabs(depth);
            printf("STRUCT_ACCESS\n");

            __print_tabs(++depth);
            printf("struct member: %s\n", node->as.struct_access.member);

            __print_tabs(depth);
            printf("pointer: %d\n", node->as.struct_access.pointer);

            __print_tabs(depth++);
            printf("SRC (STRUCT_ACCESS)\n");
            print_child(node->as.struct_access.src);

            depth -= 2;
            break;
        }

        case AST_ARRAY_ACCESS: {
            __print_tabs(depth);
            printf("ARRAY_ACCESS\n");

            __print_tabs(++depth);
            printf("ARRAY\n");
            depth++;
            print_child(node->as.array_access.array);
            
            __print_tabs(depth-1);
            printf("INDEX (ARRAY_ACCESS)\n");
            print_child(node->as.array_access.index);

            depth -= 2;
            break;
        }

        case AST_BLOCK: {
            __print_tabs(depth);
            printf("BLOCK (count=%d)\n", node->as.block.count);

            depth++;
            for (int i = 0; i < node->as.block.count; i++) {
                print_child(node->as.block.statements[i]);
            }

            depth--;
            break;
        }

        case AST_PROGRAM: {
            fprintf(stderr, "Unexpected program node\n");
            break;
        }
    }

    return;
}

void print_AST(AST_node *program) {
    printf("PROGRAM(count=%d)\n", program->as.program.count);

    for (int i = 0; i < program->as.program.count; i++) {
        depth = 1;
        print_child(program->as.program.declarations[i]);
    }
}

void free_type(Type *t) {
    if (t == NULL) return;

    switch (t->type) {
        case TYPE_VOID:
        case TYPE_INT:
        case TYPE_CHAR: {
            break;
        }

        case TYPE_POINTER: {
            free_type(t->pointee);
            break;
        }

        case TYPE_FUNCTION: {
            for (int i = 0; i < t->function.count; i++) {
                free_type(t->function.params[i]);
            }

            free_type(t->function.return_type);
            break;
        }

        case TYPE_ARRAY: {
            free_type(t->array.memb_type);
            break;
        }

        case TYPE_STRUCT: {
            // will member names be allocated by strdup() ?
            for (int i = 0; i < t->structure.count; i++) {
                free_type(t->structure.memb_types[i]);
                free(t->structure.memb_names[i]); // ?
            }

            free(t->structure.memb_types);
            free(t->structure.memb_names);
            break;
        }
    }

    free(t);
    return;
}

// frees nodes and types
void free_child(AST_node *node) {
    if (node == NULL) return;

    switch (node->type) {
        case AST_INT_LITERAL:
        case AST_BREAK:
        case AST_CONTINUE: {
            break;
        }

        case AST_VAR_REF: {
            free(node->as.var_ref.name); // allocated by strdup()
            break;
        }

        case AST_VAR_DECL: {
            free_type(node->as.var_decl.type);
            free_child(node->as.var_decl.init);
            free(node->as.var_decl.name);
            break;
        }

        case AST_BINARY_OP: {
            free_child(node->as.binary_op.left);
            free_child(node->as.binary_op.right);
            break;
        }

        case AST_UNARY_OP: {
            free_child(node->as.unary_op.operand);
            break;
        }

        case AST_IF: {
            free_child(node->as.if_statement.condition);
            free_child(node->as.if_statement.body);
            free_child(node->as.if_statement.else_branch);
            break;
        }

        case AST_WHILE: {
            free_child(node->as.while_statement.condition);
            free_child(node->as.while_statement.body);
            break;
        }

        case AST_FOR: {
            free_child(node->as.for_statement.init);
            free_child(node->as.for_statement.condition);
            free_child(node->as.for_statement.updation);
            free_child(node->as.for_statement.body);
            break;
        }

        case AST_RETURN: {
            free_child(node->as.return_statement.expr);
            break;
        }

        case AST_FUNCTION_DECL: {
            free_type(node->as.function_decl.type);
            free_child(node->as.function_decl.body);
            free(node->as.function_decl.name);

            for (int i = 0; i < node->as.function_decl.count; i++) {
                free_child(node->as.function_decl.args[i]);
            }

            free(node->as.function_decl.args);
            break;
        }

        case AST_FUNCTION_CALL: {
            for (int i = 0; i < node->as.function_call.count; i++) {
                free_child(node->as.function_call.args[i]);
            }

            free_child(node->as.function_call.callee);
            free(node->as.function_call.args);
            break;
        }

        case AST_BLOCK: {
            for (int i = 0; i < node->as.block.count; i++) {
                free_child(node->as.block.statements[i]);
            }

            free(node->as.block.statements);
            break;
        }

        case AST_PROGRAM: {
            fprintf(stderr, "Error: Unexpected program node");
            break;
        }
    }

    free(node);
    return;
}

void free_AST(AST_node *program) {
    for (int i = 0; i < program->as.program.count; i++) {
        free_child(program->as.program.declarations[i]);
    }

    free(program->as.program.declarations);
    free(program);
    return;
}

int main(void) {
    arr = (token_t *) calloc(128, sizeof(token_t));
    void *orig_arr_ptr = arr;
    FILE *f = fopen("program.c", "r");
    
    fseek(f, 0, SEEK_END);
    uint32_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    fread(program, 1, size, f);

    printf("%s\n", program);

    uint32_t tokens = tokenize(program, arr);
    printf("%d tokens parsed\n", tokens);

    for (int i = 0; i < tokens; i++) {
        print_token(arr[i].type);
    }

    AST_node *program = parse_program();

    print_AST(program);

    free_AST(program);
    free(orig_arr_ptr);
    return 0;
}
