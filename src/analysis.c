#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ast.h>
#include <typing.h>
#include <common.h>
#include <analysis.h>


#define PUSH_SCOPE(s) tab->crt_scope = s
#define POP_SCOPE(s)  tab->crt_scope = s->parent


extern Type *__deep_copy_type(Type *t);
extern Declarator *__deep_copy_declarator(Declarator *decl);
extern enum symbol_type __decl_type_to_symbol_type(Declarator *decl);


Symbol *__sym_chk_scope(char *name, Scope *scope) {
    if (scope == NULL) return NULL;

    Symbol *sym = scope->head;
    while (sym != NULL) {
        if (!strcmp(name, sym->decl->ident)) {
            return sym;
        }

        sym = sym->next;
    }

    return NULL;
}

Symbol *symbol_table_lookup(char *name, SymbolTable *tab) {
    Symbol *sym = __sym_chk_scope(name, tab->global_scope);
    if (sym != NULL) return sym;

    Scope *s = tab->crt_scope;
    while (s->parent != NULL && s->depth != 0) {
        sym = __sym_chk_scope(name, s);
        if (sym != NULL) return sym;

        s = s->parent;
    }

    return NULL;
}

extern void print_type(Type *t, int d);
Symbol *symbol_table_insert(Declarator *decl, Scope *scope) {
    if (decl == NULL) return NULL;

    Symbol *sym = malloc(sizeof(Symbol));
    sym->type = __decl_type_to_symbol_type(decl);
    sym->decl = __deep_copy_declarator(decl);
    sym->scope_depth = scope->depth;
    sym->next = NULL;

    if (scope->head != NULL) {
        Symbol *s = scope->head;
        while (s->next != NULL) {
            s = s->next;
        }

        s->next = sym;
    } else {
        scope->head = sym;
    }

    return sym;
}

Symbol *struct_tag_lookup(char *name, SymbolTable *tab) {
    if (name == NULL) return NULL;
    return __sym_chk_scope(name, tab->tag_scope);
}


// TODO: add eval_const_expr() which evaluates a constant expression recursively
bool is_constant_expr(AST_node *node) {
    if (node == NULL) return true;

    switch (node->type) {
        case AST_INT_LITERAL:
        case AST_SIZEOF: {
            return true;
        }

        // variable references and function calls arent constants
        case AST_VAR_REF:
        case AST_FUNCTION_CALL: {
            return false;
        }

        case AST_BINARY_OP: {
            return is_constant_expr(node->as.binary_op.left) &&
                   is_constant_expr(node->as.binary_op.right);
        }

        case AST_UNARY_OP: {
            return is_constant_expr(node->as.unary_op.operand);
        }

        default: {
            return false;
        }
    }
}

/*
    Given an arbitrary type, this function will check if it eventually points
    to a struct type
*/
Type *check_struct_type(Type *t) {
    Type *u = t;
    while (1) {
        if (!u) return NULL;

        switch (u->type) {
            case TYPE_VOID:
            case TYPE_INT:
            case TYPE_CHAR: {
                return NULL;
            }

            case TYPE_POINTER: {
                u = u->pointee;
                continue;
            }

            case TYPE_FUNCTION: {
                u = u->function.return_type;
                continue;
            }

            case TYPE_ARRAY: {
                u = u->array.memb_type;
                continue;
            }

            case TYPE_STRUCT: {
                return u;
            }
        }
    }
}

extern void print_type(Type *t, int d);
void analyze_var_decl(AST_node *decl, SymbolTable *tab) {
    if (tab->crt_scope->depth == 0 && decl->as.var_decl.init != NULL) {
        if (!is_constant_expr(decl->as.var_decl.init)) {
            printf("Semantic error: Cannot initialize global variable with non-constant expression\n");
            return;
        }
    }

    Symbol *s = symbol_table_lookup(decl->as.var_decl.decl->ident, tab);
    if (s != NULL) {
        printf("Semantic error: Redefinition of symbol %s\n", decl->as.var_decl.decl->ident);
        return;
    }

    Type *t = check_struct_type(decl->as.var_decl.decl->type);

    if (t) {
        Symbol *struct_sym = struct_tag_lookup(t->structure.name, tab);
        if (!struct_sym) {
            printf("Semantic error: Use of undeclared identifier %s\n", t->structure.name);
            return;
        }

        *t = *__deep_copy_type(struct_sym->decl->type);
    }    

    decl->as.var_decl.sym = symbol_table_insert(decl->as.var_decl.decl, tab->crt_scope);
}

void analyze_var_ref(AST_node *ref, SymbolTable *tab) {
    Symbol *s = symbol_table_lookup(ref->as.var_ref.name, tab);
    if (s == NULL) {
        printf("Semantic error: Use of undeclared identifier %s\n", ref->as.var_ref.name);
        return;
    }

    ref->as.var_ref.sym = s;
}

extern uint32_t sizeof_type(Type *t);
void analyze_struct_decl(AST_node *decl, SymbolTable *tab) {
    Symbol *s = struct_tag_lookup(decl->as.struct_decl.decl->ident, tab);
    if (s != NULL) {
        printf("Semantic error: Redefinition of struct %s\n", decl->as.struct_decl.decl->ident);
        return;
    }

    decl->as.struct_decl.sym = symbol_table_insert(decl->as.struct_decl.decl, tab->tag_scope);

    uint32_t offset = 0;
    for (int i = 0; i < decl->as.struct_decl.decl->type->structure.count; i++) {
        Declarator *memb_decl = decl->as.struct_decl.decl->type->structure.members[i];

        memb_decl->offset = offset;
        offset += sizeof_type(memb_decl->type);
    }
}

void analyze_func_decl(AST_node *decl, SymbolTable *tab) {
    Symbol *s = symbol_table_lookup(decl->as.function_decl.decl->ident, tab);
    if (s != NULL) {
        printf("Semantic error: Redefinition of symbol %s\n", s->decl->ident);
        return;
    }

    decl->as.function_decl.sym = symbol_table_insert(decl->as.function_decl.decl, tab->crt_scope);
}


bool is_lvalue(AST_node *node) {
    enum AST_node_type t = node->type;

    return  (t == AST_VAR_REF) || (t == AST_STRUCT_ACCESS) || (t == AST_ARRAY_ACCESS) || \
            ((t == AST_UNARY_OP) && (node->as.unary_op.op == OP_DEREFERENCE));
}


Type **types_to_free = NULL;
int types_capacity = 2;
int types_index = 0;

/*
    If the current scope is the global scope, this contains the offset in the
    data section
*/
int crt_scope_stack_offset = 0;
int crt_global_data_offset = 0;

// returns the type of the analyzed expression
// TODO: add lvalue checking
Type *analyze_node(AST_node *node, SymbolTable *tab) {
    if (node == NULL) return NULL;
    if (types_to_free == NULL) {
        types_to_free = (Type **) calloc(types_capacity, sizeof(Type*));
    } else if (types_index+1 == types_capacity) {
        types_capacity++;
        REALLOC(types_to_free, types_capacity * sizeof(Type*));
    }

    switch (node->type) {
        case AST_INT_LITERAL:
        case AST_SIZEOF: {
            Type *t = calloc(1, sizeof(Type));
            t->type = TYPE_INT;
            types_to_free[types_index++] = t;

            node->expr_type = t;
            return t;
        }

        case AST_VAR_REF: {
            analyze_var_ref(node, tab);

            node->expr_type = node->as.var_ref.sym->decl->type;
            return node->expr_type;
        }

        case AST_VAR_DECL: {
            analyze_var_decl(node, tab);

            Type *t;
            if (node->as.var_decl.init != NULL) {
                t = analyze_node(node->as.var_decl.init, tab);
                
                if (!is_assignable(node->as.var_decl.decl->type, t)) {
                    printf("Semantic error: Incompatible assignment types\n");
                    return NULL;
                }
            }

            uint32_t size = sizeof_type(node->as.var_decl.decl->type);

            if (tab->crt_scope->depth == 0) { // global scope
                node->as.var_decl.sym->stack_offset = crt_global_data_offset;
                crt_global_data_offset += size;
            } else {
                node->as.var_decl.sym->stack_offset = crt_scope_stack_offset;
                crt_scope_stack_offset -= size;
            }

            node->expr_type = node->as.var_decl.decl->type;
            return node->expr_type;
        }

        case AST_BINARY_OP: {
            Type *left = analyze_node(node->as.binary_op.left, tab);
            Type *right = analyze_node(node->as.binary_op.right, tab);

            if (node->as.binary_op.op == OP_ASSIGN && !is_lvalue(node->as.binary_op.left)) {
                printf("Semantic error: Incompatible operands to binary expression\n");
                return NULL;
            }

            if (integer_compatible(left, right)) return left;
            if (POINTER_OR_INT(left, right)) {
                if (node->as.binary_op.op == OP_ADD || node->as.binary_op.op == OP_SUB) {
                    if (is_addable(left, right)) {
                        node->expr_type = (left->type == TYPE_POINTER) ? left : right;
                        return node->expr_type;
                    }
                }
            }

            printf("Semantic error: Incompatible operands to binary expression\n");
            return NULL;
        }

        case AST_UNARY_OP: {
            Type *operand = analyze_node(node->as.unary_op.operand, tab);

            if (node->as.unary_op.op == OP_INCREMENT || node->as.unary_op.op == OP_DECREMENT) {
                if (is_integer_type(operand) || is_pointer_type(operand)) {
                    node->expr_type = operand;
                    return node->expr_type;
                }

                printf("Semantic error: Incompatible operand to increment/decrement\n");
                return NULL;
            } else if (node->as.unary_op.op == OP_DEREFERENCE) {
                if (is_pointer_type(operand)) {
                    if (operand->pointee->type != TYPE_VOID) {
                        node->expr_type = operand->pointee;
                        return node->expr_type;
                    }

                    printf("Semantic error: Cannot dereference type void*\n");
                    return NULL;
                }

                printf("Semantic error: Cannot dereference non-pointer type\n");
                return NULL;
            } else if (node->as.unary_op.op == OP_ADDRESS_OF) {
                if (node->as.unary_op.operand->type == AST_VAR_REF) {
                    Type *t = calloc(1, sizeof(Type));
                    t->type = TYPE_POINTER;
                    t->pointee = operand;
                    types_to_free[types_index++] = t;

                    node->expr_type = t;
                    return node->expr_type;
                }

                printf("Semantic error: Cannot take the address of this type\n");
                return NULL;
            } else {
                if (operand->type == TYPE_INT || operand->type == TYPE_CHAR) {
                    node->expr_type = operand;
                    return node->expr_type;
                }

                printf("Semantic error: Incompatible operand to unary expression\n");
                return NULL;
            }
        }

        case AST_IF: {
            /*
                In if and while statement conditions structures, arrays and functions
                that return void are not allowed
            */

            Type *t;
            for (int i = 0; i < node->as.if_statement.count; i++) {
                t = analyze_node(node->as.if_statement.conditions[i], tab);

                if (t == NULL) {
                    return NULL;
                } else if (t->type == TYPE_ARRAY || t->type == TYPE_VOID) {
                    printf("Statement requires expression of scalar type\n");
                    return NULL;
                }

                if (node->as.if_statement.blocks[i]->type == AST_BLOCK) {
                    Scope *s = calloc(1, sizeof(Scope));
                    s->type = SCOPE_CONDITION;
                    s->depth = tab->crt_scope->depth + 1;
                    s->parent = tab->crt_scope;
                    s->owner = tab->crt_scope->owner;

                    PUSH_SCOPE(s);

                    node->as.if_statement.blocks[i]->as.block.scope = s;
                    analyze_node(node->as.if_statement.blocks[i], tab);

                    POP_SCOPE(s);
                } else {
                    analyze_node(node->as.if_statement.blocks[i], tab);
                }
            }

            if (node->as.if_statement.else_branch != NULL && node->as.if_statement.else_branch->type == AST_BLOCK) {
                    Scope *s = calloc(1, sizeof(Scope));
                    s->type = SCOPE_CONDITION;
                    s->depth = tab->crt_scope->depth + 1;
                    s->parent = tab->crt_scope;
                    s->owner = tab->crt_scope->owner;

                    PUSH_SCOPE(s);

                    node->as.if_statement.else_branch->as.block.scope = s;
                    analyze_node(node->as.if_statement.else_branch, tab);

                    POP_SCOPE(s);
            } else {
                analyze_node(node->as.if_statement.else_branch, tab);
            }

            node->expr_type = t;
            return node->expr_type;
        }

        case AST_WHILE: {
            Type *t = analyze_node(node->as.while_statement.condition, tab);

            if (t == NULL) {
                return NULL;
            } else if (t->type == TYPE_ARRAY || t->type == TYPE_VOID) {
                printf("Statement requires expression of scalar type\n");
                return NULL;
            }

            Scope *s = calloc(1, sizeof(Scope));
            s->type = SCOPE_LOOP;
            s->depth = tab->crt_scope->depth + 1;
            s->parent = tab->crt_scope;

            PUSH_SCOPE(s);

            node->as.while_statement.body->as.block.scope = s;
            analyze_node(node->as.while_statement.body, tab);

            POP_SCOPE(s);

            node->expr_type = t;
            return node->expr_type;
        }

        case AST_FOR: {
            // analyze_node() handles type checking for declarations and reassignments
            analyze_node(node->as.for_statement.init, tab);
            Type *t = analyze_node(node->as.for_statement.condition, tab);
            analyze_node(node->as.for_statement.updation, tab);

            Scope *s = calloc(1, sizeof(Scope));
            s->type = SCOPE_LOOP;
            s->depth = tab->crt_scope->depth + 1;
            s->parent = tab->crt_scope;

            PUSH_SCOPE(s);

            node->as.for_statement.body->as.block.scope = s;
            analyze_node(node->as.for_statement.body, tab);

            POP_SCOPE(s);

            node->expr_type = t;
            return node->expr_type;
        }

        case AST_RETURN: {
            Type *return_type = analyze_node(node->as.return_statement.expr, tab);
            
            /*
                The parser does not allow return statements in the global scope,
                so it is guaranteed that it will eventually find a function scope
            */
            Scope *s = tab->crt_scope;
            while (s != NULL) {
                if (s->type == SCOPE_FUNCTION) {
                    break;
                }
                s = s->parent;
            }

            Type *function_return = s->owner->decl->type->function.return_type;

            if (!is_assignable(function_return, return_type)) {
                printf("Semantic error: Incompatible return types\n");
                return NULL;
            }

            node->expr_type = return_type;
            return node->expr_type;
        }

        case AST_FUNCTION_DECL: {
            analyze_func_decl(node, tab);

            if (node->as.function_decl.body != NULL) {
                Scope *scope = calloc(1, sizeof(Scope));
                scope->type = SCOPE_FUNCTION;
                scope->owner = node->as.function_decl.sym;

                scope->depth = tab->crt_scope->depth + 1;
                scope->parent = tab->crt_scope;

                node->as.function_decl.body->as.block.scope = scope;
                tab->crt_scope = scope;

                /*
                    Add function argumets to symbol table and calculate their stack offsets.
                    Starting from offset = 8 reserves space for the return address and the old frame pointer.
                */
                int offset = 2 * ADDRESS_WIDTH;
                for (int i = 0; i < node->as.function_decl.decl->type->function.count; i++) {
                    Symbol *s = symbol_table_insert(node->as.function_decl.decl->type->function.params[i], scope);
                    s->stack_offset = offset;

                    offset += sizeof_type(node->as.function_decl.decl->type->function.params[i]->type);
                }

                crt_scope_stack_offset = 0;
                analyze_node(node->as.function_decl.body, tab);
                node->as.function_decl.sym->frame_size = -crt_scope_stack_offset;

                tab->crt_scope = scope->parent;
            }

            node->expr_type = node->as.function_decl.decl->type;
            return node->expr_type;
        }

        case AST_FUNCTION_CALL: {
            Type *callee = analyze_node(node->as.function_call.callee, tab);

            // only function types and ponters to function types are callable
            if (callee->type != TYPE_FUNCTION && !(callee->type == TYPE_POINTER && callee->pointee->type == TYPE_FUNCTION)) {
                printf("Semantic error: Expression is not callable\n");
                return NULL;
            }

            if (callee->type == TYPE_POINTER) {
                callee = callee->pointee;
            }

            if (node->as.function_call.count != callee->function.count) {
                printf("Semantic error: Invalid number of arguments in function call\n");
                return NULL;
            }

            for (int i = 0; i < node->as.function_call.count; i++) {
                Type *t = analyze_node(node->as.function_call.args[i], tab);
                if (!is_assignable(t, callee->function.params[i]->type)) {
                    printf("Semantic error: Incompatible type conversion\n");
                    return NULL;
                }
            }

            node->expr_type = callee->function.return_type;
            return node->expr_type;
        }

        case AST_BREAK:
        case AST_CONTINUE: {
            Scope *s = tab->crt_scope;
            bool loop = false;

            while (s != NULL) {
                if (s->type == SCOPE_LOOP) {
                    loop = true;
                    break;
                }
                s = s->parent;
            }

            if (!loop) {
                printf("Semantic error: Invoked loop instruction in a non-loop scope\n");
            }

            return NULL;
        }

        case AST_STRUCT_DECL: {
            if (tab->crt_scope->depth != 0) {
                printf("Semantic error: Struct declarations on a non-global scope are not allowed\n");
                return NULL;
            }

            // nothing to type check; struct member declarations do not allow initializers
            analyze_struct_decl(node, tab);
            node->expr_type = node->as.struct_decl.decl->type;
            return node->expr_type;
        }

        case AST_STRUCT_ACCESS: {
            Type *t = analyze_node(node->as.struct_access.src, tab);
            if (t->type != TYPE_STRUCT) {
                if (t->type != TYPE_POINTER || t->pointee->type != TYPE_STRUCT) {
                    printf("Semantic error: Source operand of struct access is not a struct or a pointer to a struct\n");
                    return NULL;
                }
            }

            if ((t->type == TYPE_POINTER) != node->as.struct_access.pointer) {
                printf("Semantic error: Incompatible struct access operator\n");
                return NULL;
            }

            if (t->type == TYPE_POINTER) t = t->pointee;

            Symbol *sym = struct_tag_lookup(t->structure.name, tab);
            if (sym == NULL) {
                printf("Semantic error: Use of undeclared struct: %s\n", t->structure.name);
                return NULL;
            }

            bool found = false;
            int i = 0;
            for (i = 0; i < t->structure.count; i++) {
                if (!strcmp(t->structure.members[i]->ident, node->as.struct_access.member)) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                printf(
                    "Semantic error: %s is not a member of the struct %s\n",
                    node->as.struct_access.member, t->structure.name
                );

                return NULL;
            }

            node->expr_type = t->structure.members[i]->type;
            return node->expr_type;
        }

        case AST_ARRAY_ACCESS: {
            Type *array = analyze_node(node->as.array_access.array, tab);
            Type *index = analyze_node(node->as.array_access.index, tab);

            if (array->type != TYPE_ARRAY && array->type != TYPE_POINTER) {
                printf("Semantic error: Identifier is not an array\n");
                return NULL;
            }

            if (!is_integer_type(index)) {
                printf("Semantic error: Index identifier does not evaluate to an integer compatible type\n");
                return NULL;
            }
            
            node->expr_type = array->array.memb_type;
            return node->expr_type;
        }

        case AST_TYPE_CAST: {
            Type *operand = analyze_node(node->as.type_cast.operand, tab);

            if (!is_castable(node->as.type_cast.type, operand)) {
                printf("Semantic error: Cast to incompatible type\n");
                return NULL;
            }

            node->expr_type = node->as.type_cast.type;
            return node->expr_type;
        }

        case AST_BLOCK: {
            Type *t;
            for (int i = 0; i < node->as.block.count; i++) {
                t = analyze_node(node->as.block.statements[i], tab);
            }

            node->expr_type = t;
            return node->expr_type;
        }
        
        default: {
            return NULL;
        }
    }
}

void analyze_AST(AST_node *program, SymbolTable *tab) {
    tab->global_scope = calloc(1, sizeof(Scope));
    tab->tag_scope = calloc(1, sizeof(Scope));
    tab->crt_scope = tab->global_scope;

    tab->global_scope->type = SCOPE_GLOBAL;
    tab->tag_scope->type = SCOPE_TAG;

    for (int i = 0; i < program->as.program.count; i++) {
        analyze_node(program->as.program.declarations[i], tab);
    }

    return;
}


// TODO: Add function arguments to symbol table