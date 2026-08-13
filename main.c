#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ast.h>
#include <tokenizer.h>


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
        if (alpha || digit ) break; \
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

        TOKENIZER_2CHAR_CASE('|', OR, '|', LOGICAL_OR);
        TOKENIZER_2CHAR_CASE('&', AND, '&', LOGICAL_AND);
        TOKENIZER_2CHAR_CASE('+', ADD, '+', INCREMENT);
        TOKENIZER_2CHAR_CASE('-', SUB, '-', DECREMENT);
        TOKENIZER_2CHAR_CASE('=', ASSIGN, '=', EQUAL);

        TOKENIZER_2CHAR_CASE('>', GREATER, '>', LSH);
        TOKENIZER_2CHAR_CASE('<', LESS, '<', RSH);

        TOKENIZER_CHAR_CASE(',', COMMA);
        TOKENIZER_CHAR_CASE('*', STAR);

        TOKENIZER_CHAR_CASE('!', LOGICAL_NOT);
        TOKENIZER_CHAR_CASE('~', NOT);
        TOKENIZER_CHAR_CASE('^', XOR);
        TOKENIZER_CHAR_CASE('(', LPAR);
        TOKENIZER_CHAR_CASE(')', RPAR);
        TOKENIZER_CHAR_CASE('{', LBRACE);
        TOKENIZER_CHAR_CASE('}', RBRACE);
        TOKENIZER_CHAR_CASE(';', SEMICOLON);

        TOKENIZER_CHAR_CASE(0, EOF);
    }

    if (!strcmp(buff, "int")) dst->type = TOKEN_INT;
    SET_BUFF_TYPE("if", IF);
    SET_BUFF_TYPE("while", WHILE);
    SET_BUFF_TYPE("return", RETURN);
    SET_BUFF_TYPE("void", VOID);
    SET_BUFF_TYPE("char", CHAR);
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

void print_token_array(token_t *arr, int tokens) {
    for (int i = 0; i < tokens; i++) {
        switch (arr[i].type) {
            case TOKEN_VOID: {
                printf("TOKEN_VOID ");
                break;
            }

            case TOKEN_INT: {
                printf("TOKEN_INT ");
                break;
            }

            case TOKEN_IDENT: {
                printf("TOKEN_IDENT ");
                break;
            }

            case TOKEN_NUM: {
                printf("TOKEN_NUM ");
                break;
            }

            case TOKEN_ADD: {
                printf("TOKEN_ADD ");
                break;
            }

            case TOKEN_ASSIGN: {
                printf("TOKEN_ASSIGN ");
                break;
            }

            case TOKEN_SEMICOLON: {
                printf("TOKEN_SEMICOLON ");
                break;
            }

            case TOKEN_IF: {
                printf("TOKEN_IF ");
                break;
            }

            case TOKEN_WHILE: {
                printf("TOKEN_WHILE ");
                break;
            }
            case TOKEN_RETURN: {
                printf("TOKEN_RETURN ");
                break;
            }
            case TOKEN_SUB: {
                printf("TOKEN_SUB ");
                break;
            }
            case TOKEN_LPAR: {
                printf("TOKEN_LPAR ");
                break;
            }
            case TOKEN_RPAR: {
                printf("TOKEN_RPAR ");
                break;
            }
            case TOKEN_LBRACE: {
                printf("TOKEN_LBRACE ");
                break;
            }
            case TOKEN_RBRACE: {
                printf("TOKEN_RBRACE ");
                break;
            }

            case TOKEN_EQUAL: {
                printf("TOKEN_EQUAL ");
                break;
            }

            case TOKEN_INCREMENT: {
                printf("TOKEN_INCREMENT ");
                break;
            }

            case TOKEN_LSH: {
                printf("TOKEN_LSH ");
                break;
            }

            case TOKEN_OR: {
                printf("TOKEN_OR ");
                break;
            }

            case TOKEN_AND: {
                printf("TOKEN_AND ");
                break;
            }

            case TOKEN_COMMA: {
                printf("TOKEN_COMMA ");
                break;
            }

            case TOKEN_EOF: {
                printf("TOKEN_EOF\n");
                break;
            }
        }
    }
}

char program[4096];
token_t *arr;

#define CRT_TYPE            get_current_token().type
#define CRT_VAL             get_current_token().val
#define OFFSET_CRT_TYPE(n)  (*(arr + n)).type

token_t get_current_token() {
    return *arr;
}

void consume() {
    // printf("DEBUG: Consuming %d\n", CRT_TYPE);
    arr++;
    return;
}

void expect(enum tokentype token) {
    if ((*arr).type != token) {
        fprintf(stderr, "Syntax error: Expected token %d, got %d.\n\tPrevious token: %d\n\tNext token: %d\n", (int) token, (int) CRT_TYPE, (int) (*(arr - 1)).type, (*(arr + 1)).type);
    }

    return;
}

void expect_and_consume(enum tokentype token) {
    expect(token);
    consume();
}

Type *parse_type() {
    if ((int) CRT_TYPE >= 3) {
        fprintf(stderr, "Syntax error: Expected type declaration, got %d", CRT_TYPE);
        return NULL;
    }

    Type *type = malloc(sizeof(Type));
    Type *root_type = type;
    enum data_type final = (enum data_type) CRT_TYPE;

    consume();
    while (CRT_TYPE == TOKEN_STAR) {
        type->type = TYPE_POINTER;
        type->pointee = malloc(sizeof(Type));

        type = type->pointee;
        consume();
    }

    type->type = final;
    return root_type;
}


uint32_t created_nodes = 0;

AST_node *parse_factor();
AST_node *parse_term();
AST_node *parse_bitwise_not();
AST_node *parse_bitwise_shifts();
AST_node *parse_bitwise_and();
AST_node *parse_bitwise_xor();


AST_node *parse_logical_expression() {
    AST_node *left = parse_bitwise_operations();

    while (CRT_TYPE == TOKEN_LOGICAL_OR || CRT_TYPE == TOKEN_LOGICAL_AND) {
        enum binop_operator op = (enum binop_operator) CRT_TYPE;
        consume();

        AST_node *right = parse_bitwise_operations();
        AST_node *bin_node = malloc(sizeof(AST_node));
        bin_node->type = AST_BINARY_OP;
        bin_node->as.binary_op.op = op;
        bin_node->as.binary_op.left = left;
        bin_node->as.binary_op.right = right;

        created_nodes++;
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

        created_nodes++;
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

        created_nodes++;
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

        created_nodes++;
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

        created_nodes++;
        left = bin_node;
    }

    return left;
}

// this function is ass. fix it
AST_node *parse_bitwise_not() {
    expect_and_consume(TOKEN_NOT);

    AST_node *bin_node = malloc(sizeof(AST_node));
    bin_node->type = AST_BINARY_OP;
    bin_node->as.binary_op.op = OP_NOT;
    bin_node->as.binary_op.left = NULL;
    bin_node->as.binary_op.right = parse_comparison();

    return bin_node;
}


/*
    expressions are parsed recursively, each precedence layer has its own function,
    so when parsing an expression, the order of operations is automatically applied
*/

AST_node *parse_comparison() {
    AST_node *left = parse_expression();

    if (CRT_TYPE == TOKEN_LESS || CRT_TYPE == TOKEN_GREATER || CRT_TYPE == TOKEN_EQUAL) {
        enum binop_operator op = (enum binop_operator) (CRT_TYPE - TOKEN_ADD);
        consume();

        AST_node *right = parse_expression();

        AST_node *bin_node = malloc(sizeof(AST_node));
        bin_node->type = AST_BINARY_OP;
        bin_node->as.binary_op.op = op;
        bin_node->as.binary_op.left = left;
        bin_node->as.binary_op.right = right;

        created_nodes++;
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

        created_nodes++;
        left = bin_node;
    }

    return left;
}

// multiplication
AST_node *parse_term() {
    AST_node *left = parse_factor();

    while (CRT_TYPE == TOKEN_STAR) {
        enum binop_operator op = (enum binop_operator) (CRT_TYPE - TOKEN_ADD);
        consume();

        AST_node *right = parse_factor();

        AST_node *bin_node = malloc(sizeof(AST_node));
        bin_node->type = AST_BINARY_OP;
        bin_node->as.binary_op.op = op;
        bin_node->as.binary_op.left = left;
        bin_node->as.binary_op.right = right;

        created_nodes++;
        left = bin_node;
    }

    return left;
}

AST_node *parse_factor() {
    if (CRT_TYPE == TOKEN_NUM) {
        AST_node *node = malloc(sizeof(AST_node));
        node->type = AST_INT_LITERAL;
        node->as.int_literal.value = CRT_VAL;

        created_nodes++;
        consume();
        return node;
    } else if (CRT_TYPE == TOKEN_IDENT) {
        // function call or variable reference
        AST_node *node = malloc(sizeof(AST_node));
        created_nodes++;

        if (OFFSET_CRT_TYPE(1) == TOKEN_LPAR) { // function call
            node->type = AST_FUNCTION_CALL;
            node->as.function_call.name = strdup(get_current_token().text);

            consume(); // TOKEN_IDENT
            consume(); // TOKEN_LPAR

            int argc = 0;
            int argv_max = 8;
            AST_node **argv = (AST_node **) calloc(argv_max, sizeof(AST_node*));
            while (CRT_TYPE != TOKEN_RPAR) {
                if (argc == argv_max) {
                    argv_max += 2;
                    argv = realloc(argv, argv_max * sizeof(AST_node*));
                }

                argv[argc++] = parse_logical_expression();

                if (CRT_TYPE != TOKEN_COMMA && CRT_TYPE != TOKEN_RPAR) {
                    fprintf(stderr, "Syntax error: Expected comma or closing parenthesis");
                    return NULL;
                }

                if (CRT_TYPE == TOKEN_COMMA) consume();
            }

            if (argc > 0 && argc < argv_max) {
                argv = realloc(argv, argc * sizeof(AST_node*));
            } else if (argc == 0) {
                free(argv);
                argv = NULL;
            }

            node->as.function_call.args = argv;
            node->as.function_call.count = argc;

            consume(); // TOKEN_RPAR
            return node;
        }

        node->type = AST_VAR_REF;
        node->as.var_ref.name = strdup(get_current_token().text);

        consume(); // TOKEN_IDENT
        return node;
    } else if (CRT_TYPE == TOKEN_LPAR) {
        consume();
        AST_node *node = parse_bitwise_operations();
        expect_and_consume(TOKEN_RPAR);

        return node;
    }

    perror("Syntax error\n");
    return NULL;
}

AST_node *parse_assignment() {
    if (CRT_TYPE == TOKEN_VOID || CRT_TYPE == TOKEN_INT || CRT_TYPE == TOKEN_CHAR) {
        AST_node *node = malloc(sizeof(AST_node));
        node->type = AST_VAR_DECL;
        node->as.var_decl.type = parse_type();

        expect(TOKEN_IDENT);
        node->as.var_decl.name = strdup(get_current_token().text);
        consume();

        if (CRT_TYPE == TOKEN_ASSIGN) {
            consume();
            node->as.var_decl.init = parse_logical_expression();
        } else {
            node->as.var_decl.init = NULL;
        }

        created_nodes++;
        return node;
    }

    perror("Syntax error\n");
    return NULL;
}

AST_node *parse_function_decl() {
    AST_node *node = malloc(sizeof(AST_node));
    node->type = AST_FUNCTION_DECL;
    node->as.function_decl.type = parse_type();
    
    expect(TOKEN_IDENT);
    node->as.function_decl.name = strdup(get_current_token().text);
    consume();

    created_nodes++;
    expect_and_consume(TOKEN_LPAR);
    
    int argc = 0;
    int argv_max = 8;
    AST_node **argv = (AST_node **) calloc(argv_max, sizeof(AST_node*));
    while (CRT_TYPE != TOKEN_RPAR) {
        if (CRT_TYPE == TOKEN_VOID || CRT_TYPE == TOKEN_INT || CRT_TYPE == TOKEN_CHAR) {
            if (argc == argv_max) {
                argv_max += 2;
                argv = (AST_node **) realloc(argv, argv_max * sizeof(AST_node*));
            }

            argv[argc] = (AST_node *) malloc(sizeof(AST_node));

            argv[argc]->type = AST_VAR_DECL;
            argv[argc]->as.var_decl.type = parse_type();
            argv[argc]->as.var_decl.init = NULL;

            expect(TOKEN_IDENT);
            argv[argc]->as.var_decl.name = strdup(get_current_token().text);
            consume(); // TOKEN_IDENT

            argc++;
            created_nodes++;
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
        argv = (AST_node **) realloc(argv, argc * sizeof(AST_node*));
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

AST_node *parse_function_call() {
    AST_node *node = (AST_node *) malloc(sizeof(AST_node));
    node->type = AST_FUNCTION_CALL;

    created_nodes++;
    expect(TOKEN_IDENT);
    node->as.function_call.name = strdup(get_current_token().text);

    consume(); // TOKEN_IDENT
    expect_and_consume(TOKEN_LPAR);

    int argc = 0;
    int argv_max = 8;
    AST_node **argv = (AST_node **) calloc(8, sizeof(AST_node*));
    while (CRT_TYPE != TOKEN_RPAR) {
        if (argc == argv_max) {
            argv_max += 2;
            argv = (AST_node **) realloc(argv, argv_max * sizeof(AST_node*));
        }

        argv[argc++] = parse_logical_expression();

        if (CRT_TYPE != TOKEN_RPAR && CRT_TYPE != TOKEN_COMMA) {
            fprintf(stderr, "Syntax error: Expected comma or closing parenthesis\n");
            return NULL;
        }

        if (CRT_TYPE == TOKEN_COMMA) consume();
    }

    if (argc > 0 && argc < argv_max) {
        argv = (AST_node **) realloc(argv, argc * sizeof(AST_node*));
    } else if (argc == 0) {
        free(argv);
        argv = NULL;
    }

    consume(); // TOKEN_RPAR

    node->as.function_call.args = argv;
    node->as.function_call.count = argc;

    return node;
}


AST_node *parse_if_statement() {
    consume();
    expect_and_consume(TOKEN_LPAR);

    AST_node *node = malloc(sizeof(AST_node));
    node->type = AST_IF;
    node->as.if_statement.condition = parse_comparison();
    expect_and_consume(TOKEN_RPAR);

    created_nodes++;
    if (CRT_TYPE != TOKEN_LBRACE) {
        // single line body
        // not implemented
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
    node->as.return_statement.expr = parse_bitwise_operations();

    created_nodes++;
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

    created_nodes++;
    return node;
}

AST_node *parse_statement() {
    AST_node *node = NULL;

    switch (CRT_TYPE) {

        // variable assignment or function declaration
        case TOKEN_INT:
        case TOKEN_CHAR:
        case TOKEN_VOID: {
            /*
                check "crt_token + 2": if its a variable decl. it would be TOKEN_ASSIGN or TOKEN_SEMICOLON,
                if its a function declaration it would be TOKEN_LPAR

                token layout:
                TOKEN_(type) TOKEN_IDENT TOKEN_ASSIGN/TOKEN_LPAR
            */

            if (OFFSET_CRT_TYPE(2) == TOKEN_ASSIGN || OFFSET_CRT_TYPE(2) == TOKEN_SEMICOLON) {
                node = parse_assignment();
                break;
            } else if (OFFSET_CRT_TYPE(2) == TOKEN_LPAR) {
                node = parse_function_decl();
                break;
            }
        }

        // TODO: add pointer case in above cases

        // either variable increment, inline operation or function call
        case TOKEN_IDENT: {
            // inline operations are not yet implemented
            /*
                check "crt_token + 1". if its an increment/decrement, it would be TOKEN_INCREMENT or TOKEN_DECREMENT
                if its a function call it would be TOKEN_LPAR
            */

            if (OFFSET_CRT_TYPE(1) == TOKEN_LPAR) {
                node = parse_function_call();
                break;
            } else if (OFFSET_CRT_TYPE(1) == TOKEN_INCREMENT || OFFSET_CRT_TYPE(1) == TOKEN_DECREMENT) {
                /* this block transforms x++ (or x--) to x = x +/- 1*/
                AST_node *var_ref_left = malloc(sizeof(AST_node));
                var_ref_left->type = AST_VAR_REF;
                var_ref_left->as.var_ref.name = strdup(get_current_token().text);

                /*
                    Creating a shallow copy of the var_ref node is required, as both sides of the
                    binary operation (=), would eventually point to the same address, thus freeing one 
                    would later cause a segmentation fault while trying to free the other
                    (i.e. node->as.binary_op.left = right->as.binary_op.left = var_ref)
                */
                AST_node *var_ref_right = malloc(sizeof(AST_node));
                var_ref_right->type = AST_VAR_REF;

                /*
                    For the same exact reason memcpy() is not used in this case.
                    If memcpy() was used, both node names would point to the same address:
                    var_ref_right->as.var_ref.name = var_ref_left->as.var_ref.name

                    Therefore, a segmentation fault would be caused while freeing the names
                */
                var_ref_right->as.var_ref.name = strdup(get_current_token().text);

                consume(); // TOKEN_IDENT

                node = malloc(sizeof(AST_node));
                node->type = AST_BINARY_OP;
                node->as.binary_op.op = OP_ASSIGN;
                node->as.binary_op.left = var_ref_left;

                AST_node *right = malloc(sizeof(AST_node));
                right->type = AST_BINARY_OP;
                if (CRT_TYPE == TOKEN_INCREMENT) right->as.binary_op.op = OP_ADD;
                else right->as.binary_op.op = OP_SUB;
                right->as.binary_op.left = var_ref_right;

                AST_node *int_literal = malloc(sizeof(AST_node));
                int_literal->type = AST_INT_LITERAL;
                int_literal->as.int_literal.value = 1;

                right->as.binary_op.right = int_literal;
                node->as.binary_op.right = right;

                created_nodes += 4;
                consume(); // TOKEN_INCREMENT

                break;
            }
        }

        case TOKEN_NUM: {
            fprintf(stderr, "Syntax error: Unexpected integer literal (prev=%d, next=%d)\n", OFFSET_CRT_TYPE(-1), OFFSET_CRT_TYPE(1));
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

        case TOKEN_RETURN: {
            node = parse_return();
            break;
        }

        case TOKEN_ADD:
        case TOKEN_SUB:
        case TOKEN_STAR: // this might be a dereference, implement later !!!
        case TOKEN_LESS:
        case TOKEN_GREATER:
        case TOKEN_EQUAL:
        case TOKEN_NOT:
        case TOKEN_LSH:
        case TOKEN_RSH:
        case TOKEN_AND:
        case TOKEN_XOR:
        case TOKEN_OR:
        case TOKEN_LOGICAL_NOT:
        case TOKEN_LOGICAL_AND:
        case TOKEN_LOGICAL_OR:
        case TOKEN_INCREMENT: // might be ++x
        case TOKEN_DECREMENT: // same thing
        case TOKEN_ASSIGN:
        case TOKEN_LPAR:
        case TOKEN_RPAR:
        case TOKEN_LBRACE:
        case TOKEN_RBRACE:
        case TOKEN_SEMICOLON:
        case TOKEN_COMMA: {
            fprintf(stderr, "Syntax error: Unexpected token %d\n", CRT_TYPE);
            break;
        }
    }

    // a block was just parsed, therefore a semicolon is not required
    if ((*(arr - 1)).type != TOKEN_RBRACE) {
        expect_and_consume(TOKEN_SEMICOLON);
    }
    return node;
}

AST_node *parse_block() {
    expect_and_consume(TOKEN_LBRACE);

    created_nodes++;
    AST_node *node = malloc(sizeof(AST_node));
    node->type = AST_BLOCK;
    node->as.block.statements = (AST_node **) calloc(8, sizeof(AST_node*));
    node->as.block.count = 0;

    int capacity = 8;

    while (CRT_TYPE != TOKEN_RBRACE) {
        if (node->as.block.count == capacity) {
            capacity += 8;
            node->as.block.statements = (AST_node **) realloc(node->as.block.statements, capacity * sizeof(AST_node*));
        }

        if (CRT_TYPE == TOKEN_EOF) {
            fprintf(stderr, "Syntax error: Expected closing brace\n");
            return NULL;
        }

        node->as.block.statements[node->as.block.count++] = parse_statement();
    }

    if (node->as.block.count < capacity) {
        node->as.block.statements = (AST_node **) realloc(node->as.block.statements, node->as.block.count * sizeof(AST_node*));
    }

    consume(); // TOKEN_RBRACE
    return node;
}

AST_node *parse_program() {
    AST_node *node = malloc(sizeof(AST_node));
    created_nodes++;
    int capacity = 8;

    node->type = AST_PROGRAM;
    node->as.program.count = 0;
    node->as.program.declarations = (AST_node **) calloc(capacity, sizeof(AST_node*));

    while (CRT_TYPE != TOKEN_EOF) {
        if (node->as.program.count == capacity) {
            capacity += 8;
            node->as.program.declarations = (AST_node **) realloc(node->as.program.declarations, capacity * sizeof(AST_node*));
        }

        AST_node *statement = parse_statement();
        if (statement->type == AST_VAR_DECL || statement->type == AST_FUNCTION_DECL) {
            node->as.program.declarations[node->as.program.count++] = statement;
            continue;
        }

        // statement is not a declaration
        fprintf(stderr, "Syntax error: Only declarations are allowed in the global scope\n");
        return NULL;
    }

    if (node->as.program.count < capacity) {
        node->as.program.declarations = (AST_node **) realloc(
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

#define PRINT_CASE(c) \
    case c: { \
        printf(#c "\n"); \
        break; \
    }



void print_binary_operator(enum binop_operator op) {
    switch (op) {
        PRINT_CASE(OP_ADD);
        PRINT_CASE(OP_SUB);
        PRINT_CASE(OP_STAR);
        PRINT_CASE(OP_LESS);
        PRINT_CASE(OP_GREATER);
        PRINT_CASE(OP_EQUAL);
        PRINT_CASE(OP_NOT);
        PRINT_CASE(OP_LSH);
        PRINT_CASE(OP_RSH);
        PRINT_CASE(OP_AND)
        PRINT_CASE(OP_XOR);
        PRINT_CASE(OP_OR);
        PRINT_CASE(OP_LOGICAL_NOT)
        PRINT_CASE(OP_LOGICAL_AND);
        PRINT_CASE(OP_LOGICAL_OR);
        PRINT_CASE(OP_INCREMENT);
        PRINT_CASE(OP_DECREMENT);
        PRINT_CASE(OP_ASSIGN);
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
            printf("name: %s\n", node->as.function_call.name);

            depth++;
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
        case AST_INT_LITERAL: {
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

            free(node->as.function_call.args);
            free(node->as.function_call.name);
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

    // print_token_array(arr, tokens);

    AST_node *program = parse_program();
    printf("%d AST nodes created\n", created_nodes);

    print_AST(program);

    free_AST(program);
    free(orig_arr_ptr);
    return 0;
}
