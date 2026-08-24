#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ast.h>
#include <analysis.h>
#include <tokenizer.h>


#define STRING_CASE(c) \
    case c: { \
        return #c; \
    }


void __print_tabs(int depth) {
    for (int i = 0; i < depth; i++) {
        printf("    ");
    }

    return;
}

const char *token_to_string(enum tokentype token) {
    switch (token) {
        STRING_CASE(TOKEN_VOID);
        STRING_CASE(TOKEN_INT);
        STRING_CASE(TOKEN_CHAR);
        STRING_CASE(TOKEN_IDENT);
        STRING_CASE(TOKEN_NUM);
        STRING_CASE(TOKEN_IF);
        STRING_CASE(TOKEN_ELSE);
        STRING_CASE(TOKEN_WHILE);
        STRING_CASE(TOKEN_FOR);
        STRING_CASE(TOKEN_RETURN);
        STRING_CASE(TOKEN_ADD);
        STRING_CASE(TOKEN_SUB);
        STRING_CASE(TOKEN_STAR);
        STRING_CASE(TOKEN_LESS);
        STRING_CASE(TOKEN_GREATER);
        STRING_CASE(TOKEN_EQUAL);
        STRING_CASE(TOKEN_LE);
        STRING_CASE(TOKEN_GE);
        STRING_CASE(TOKEN_NE);
        STRING_CASE(TOKEN_NOT);
        STRING_CASE(TOKEN_LSH);
        STRING_CASE(TOKEN_RSH);
        STRING_CASE(TOKEN_AND);
        STRING_CASE(TOKEN_XOR);
        STRING_CASE(TOKEN_OR);
        STRING_CASE(TOKEN_LOGICAL_NOT);
        STRING_CASE(TOKEN_LOGICAL_AND);
        STRING_CASE(TOKEN_LOGICAL_OR);
        STRING_CASE(TOKEN_INCREMENT);
        STRING_CASE(TOKEN_DECREMENT);
        STRING_CASE(TOKEN_ASSIGN);
        STRING_CASE(TOKEN_LPAR);
        STRING_CASE(TOKEN_RPAR);
        STRING_CASE(TOKEN_LBRACE);
        STRING_CASE(TOKEN_RBRACE);
        STRING_CASE(TOKEN_LBRACKET);
        STRING_CASE(TOKEN_RBRACKET);
        STRING_CASE(TOKEN_SEMICOLON);
        STRING_CASE(TOKEN_COMMA);
        STRING_CASE(TOKEN_SIZEOF);
        STRING_CASE(TOKEN_BREAK);
        STRING_CASE(TOKEN_CONTINUE);
        STRING_CASE(TOKEN_STRUCT);
        STRING_CASE(TOKEN_MEMB_ACCESS);
        STRING_CASE(TOKEN_PTR_MEMB_ACCESS);
        STRING_CASE(TOKEN_EOF);
    }
}

const char *binary_op_to_string(enum binop_operator op) {
    switch (op) {
        STRING_CASE(OP_ADD);
        STRING_CASE(OP_SUB);
        STRING_CASE(OP_STAR);
        STRING_CASE(OP_LESS);
        STRING_CASE(OP_GREATER);
        STRING_CASE(OP_EQUAL);
        STRING_CASE(OP_LE);
        STRING_CASE(OP_GE);
        STRING_CASE(OP_NE);
        STRING_CASE(OP_LSH);
        STRING_CASE(OP_RSH);
        STRING_CASE(OP_AND)
        STRING_CASE(OP_XOR);
        STRING_CASE(OP_OR);
        STRING_CASE(OP_LOGICAL_AND);
        STRING_CASE(OP_LOGICAL_OR);
        STRING_CASE(OP_ASSIGN);
    }
}

const char *unary_op_to_string(enum unary_operator op) {
    switch (op) {
        STRING_CASE(OP_INCREMENT);
        STRING_CASE(OP_DECREMENT);
        STRING_CASE(OP_NOT);
        STRING_CASE(OP_LOGICAL_NOT);
        STRING_CASE(OP_DEREFERENCE);
        STRING_CASE(OP_ADDRESS_OF);
        STRING_CASE(OP_SIZEOF);
    }
}

const char *data_type_to_string(enum data_type type) {
    switch (type) {
        STRING_CASE(TYPE_VOID);
        STRING_CASE(TYPE_INT);
        STRING_CASE(TYPE_CHAR);
        STRING_CASE(TYPE_POINTER);
        STRING_CASE(TYPE_FUNCTION);
        STRING_CASE(TYPE_ARRAY);
        STRING_CASE(TYPE_STRUCT);
    }
}

const char *symbol_type_to_string(enum symbol_type type) {
    switch (type) {
        STRING_CASE(SYMBOL_VAR);
        STRING_CASE(SYMBOL_FUNC);
        STRING_CASE(SYMBOL_STRUCT_TAG);
    }
}

enum symbol_type __decl_type_to_symbol_type(Declarator *decl) {
    switch (decl->type->type) {
        case TYPE_VOID:
        case TYPE_INT:
        case TYPE_CHAR:
        case TYPE_POINTER:
        case TYPE_ARRAY: {
            return SYMBOL_VAR;
        }

        case TYPE_FUNCTION: {
            return SYMBOL_FUNC;
        }

        case TYPE_STRUCT: {
            // check if its struct declaration or struct reference
            if (decl->type->structure.count == 0) return SYMBOL_VAR;
            return SYMBOL_STRUCT_TAG;
        }
    }
}

void print_type(Type *t, int d) {
    __print_tabs(d);
    printf("type: %s\n", data_type_to_string(t->type));

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
                    __print_tabs(d);
                    printf("ARG %d\n", i);

                    __print_tabs(d+1);
                    printf("ident: %s\n", t->function.params[i]->ident);
                    print_type(t->function.params[i]->type, d+1);
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
                __print_tabs(d+1);
                printf("MEMBER %d\n", i);

                __print_tabs(d+1);
                printf("name: %s\n", t->structure.members[i]->ident);
                print_type(t->structure.members[i]->type, d+2);
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
            printf("name: %s\n", node->as.var_decl.decl->ident);

            __print_tabs(depth++);
            printf("TYPE (VAR_DECL)\n");
            print_type(node->as.var_decl.decl->type, depth);

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
            printf("operator: %s\n", binary_op_to_string(node->as.binary_op.op));
            
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
            printf("operator: %s\n", unary_op_to_string(node->as.unary_op.op));

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

            depth += 2;
            for (int i = 0; i < node->as.if_statement.count; i++) {
                __print_tabs(depth-1);
                printf("CONDITION %d (IF_STATEMENT)\n", i);
                print_child(node->as.if_statement.conditions[i]);

                __print_tabs(depth-1);
                printf("BLOCK %d (IF_STATEMENT)\n", i);
                print_child(node->as.if_statement.blocks[i]);
            }

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
            printf("name: %s\n", node->as.function_decl.decl->ident);

            __print_tabs(depth);
            printf("TYPE (FUNCTION_DECL)\n");

            depth++;
            print_type(node->as.function_decl.decl->type, depth);

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
            Type *t = node->as.struct_decl.decl->type;

            __print_tabs(depth);
            printf(
                "STRUCT_DECL (count=%d)\n",
                t->structure.count
            );

            __print_tabs(++depth);
            printf("struct name: %s\n", node->as.struct_decl.decl->ident);

            __print_tabs(depth++);
            printf("STRUCT MEMBERS (STRUCT_DECL)\n");
            for (int i = 0; i < t->structure.count; i++){
                __print_tabs(depth-1);
                printf("MEMBER %d (STRUCT_DECL)\n", i);
                __print_tabs(depth);
                printf("name: %s\n", t->structure.members[i]->ident);

                print_type(t->structure.members[i]->type, depth);
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

        case AST_TYPE_CAST: {
            __print_tabs(depth);
            printf("TYPE_CAST\n");

            __print_tabs(++depth);
            printf("TYPE (TYPE_CAST)\n");
            print_type(node->as.type_cast.type, depth+1);

            __print_tabs(depth++);
            printf("OPERAND (TYPE_CAST)\n");
            print_child(node->as.type_cast.operand);

            depth -= 2;
            break;
        }

        case AST_SIZEOF: {
            __print_tabs(depth);
            printf("SIZEOF\n");

            __print_tabs(++depth);
            if (node->as.sizeof_.is_type) {
                printf("OPERAND TYPE (SIZEOF)\n");
                print_type(node->as.sizeof_.operand_type, depth+1);
            } else {
                printf("OPERAND (SIZEOF)\n");
                depth++;
                print_child(node->as.sizeof_.operand_ast);
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
            ret->function.params = calloc(t->function.count, sizeof(Declarator*));

            for (int i = 0; i < t->function.count; i++) {
                ret->function.params[i]->type = __deep_copy_type(t->function.params[i]->type);
                ret->function.params[i]->ident = strdup(t->function.params[i]->ident);
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
            ret->structure.members = calloc(t->structure.count, sizeof(Declarator*)); 

            for (int i = 0; i < t->structure.count; i++) {
                ret->structure.members[i]->type = __deep_copy_type(t->structure.members[i]->type);
                ret->structure.members[i]->ident = strdup(t->structure.members[i]->ident);
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
            ret->as.var_decl.decl = malloc(sizeof(Declarator));

            ret->as.var_decl.decl->type = __deep_copy_type(node->as.var_decl.decl->type);
            ret->as.var_decl.decl->ident = strdup(node->as.var_decl.decl->ident);
            ret->as.var_decl.init = __deep_copy_node(node->as.var_decl.init);
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

        case AST_IF: {
            ret->as.if_statement.count = node->as.if_statement.count;
            ret->as.if_statement.blocks = calloc(
                node->as.if_statement.count, sizeof(AST_node*)
            );
            ret->as.if_statement.conditions = calloc(
                node->as.if_statement.count, sizeof(AST_node*)
            );

            for (int i = 0; i < node->as.if_statement.count; i++) {
                ret->as.if_statement.blocks[i] = __deep_copy_node(node->as.if_statement.blocks[i]);
                ret->as.if_statement.conditions[i] = __deep_copy_node(node->as.if_statement.conditions[i]);
            }

            ret->as.if_statement.else_branch = __deep_copy_node(node->as.if_statement.else_branch);
            break;
        }

        case AST_WHILE: {
            ret->as.while_statement.body = __deep_copy_node(node->as.while_statement.body);
            ret->as.while_statement.condition = __deep_copy_node(node->as.while_statement.condition);
            break;
        }

        case AST_FOR: {
            ret->as.for_statement.init = __deep_copy_node(node->as.for_statement.init);
            ret->as.for_statement.condition = __deep_copy_node(node->as.for_statement.condition);
            ret->as.for_statement.updation = __deep_copy_node(node->as.for_statement.updation);
            ret->as.for_statement.body = __deep_copy_node(node->as.for_statement.body);
            break;
        }

        case AST_RETURN: {
            ret->as.return_statement.expr = __deep_copy_node(node->as.return_statement.expr);
            break;
        }

        case AST_FUNCTION_DECL: {
            ret->as.function_decl.decl = malloc(sizeof(Declarator));
            ret->as.function_decl.decl->type = __deep_copy_type(node->as.function_decl.decl->type);
            ret->as.function_decl.decl->ident = strdup(node->as.function_decl.decl->ident);
            ret->as.function_decl.body = __deep_copy_node(node->as.function_decl.body);
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

        case AST_STRUCT_DECL: {
            ret->as.struct_decl.decl->type = __deep_copy_type(node->as.struct_decl.decl->type);
            ret->as.struct_decl.decl->ident = strdup(node->as.struct_decl.decl->ident);
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

        case AST_TYPE_CAST: {
            ret->as.type_cast.type = __deep_copy_type(node->as.type_cast.type);
            ret->as.type_cast.operand = __deep_copy_node(node->as.type_cast.operand);
            break;
        }

        case AST_SIZEOF: {
            ret->as.sizeof_.is_type = node->as.sizeof_.is_type;
            if (node->as.sizeof_.is_type) {
                ret->as.sizeof_.operand_type = __deep_copy_type(node->as.sizeof_.operand_type);
            } else {
                ret->as.sizeof_.operand_ast = __deep_copy_node(node->as.sizeof_.operand_ast);
            }
            break;
        }

        case AST_BREAK:
        case AST_CONTINUE: {
            break;
        }

        default: {
            printf("Error: Unexpected argument in __deep_copy_node()\n");
        }
    }

    return ret;
}

Declarator *__deep_copy_declarator(Declarator *decl) {
    if (decl == NULL) return NULL;

    Declarator *declarator = malloc(sizeof(Declarator));
    declarator->type = __deep_copy_type(decl->type);
    declarator->ident = strdup(decl->ident);

    return declarator;
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
                free_type(t->function.params[i]->type);
                if (t->function.params[i]->ident != NULL) {
                    free(t->function.params[i]->ident);
                }

                free(t->function.params[i]);
            }

            free(t->function.params);
            free_type(t->function.return_type);
            break;
        }

        case TYPE_ARRAY: {
            free_type(t->array.memb_type);
            break;
        }

        case TYPE_STRUCT: {
            for (int i = 0; i < t->structure.count; i++) {
                free_type(t->structure.members[i]->type);
                free(t->structure.members[i]->ident);
                free(t->structure.members[i]);
            }

            free(t->structure.members);
            free(t->structure.members);
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
            free_type(node->as.var_decl.decl->type);
            free_child(node->as.var_decl.init);
            free(node->as.var_decl.decl->ident);
            free(node->as.var_decl.decl);
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
            for (int i = 0; i < node->as.if_statement.count; i++) {
                free_child(node->as.if_statement.blocks[i]);
                free_child(node->as.if_statement.conditions[i]);
            }

            free(node->as.if_statement.blocks);
            free(node->as.if_statement.conditions);
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
            free_type(node->as.function_decl.decl->type);
            free_child(node->as.function_decl.body);
            free(node->as.function_decl.decl->ident);
            free(node->as.var_decl.decl);
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

        case AST_STRUCT_DECL: {
            free_type(node->as.struct_decl.decl->type);
            free(node->as.struct_decl.decl->ident);
            break;
        }

        case AST_STRUCT_ACCESS: {
            free(node->as.struct_access.member);
            free_child(node->as.struct_access.src);
            break;
        }

        case AST_ARRAY_ACCESS: {
            free_child(node->as.array_access.array);
            free_child(node->as.array_access.index);
            break;
        }

        case AST_TYPE_CAST: {
            free_type(node->as.type_cast.type);
            free_child(node->as.type_cast.operand);
            break;
        }

        case AST_SIZEOF: {
            if (node->as.sizeof_.is_type) {
                free_type(node->as.sizeof_.operand_type);
            } else {
                free_child(node->as.sizeof_.operand_ast);
            }
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
