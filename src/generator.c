#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <analysis.h>
#include <generator.h>
#include <instructions.h>


#define GENERATE_BIN_OP(instruction, operator, return_result) \
    case operator: {\
        fputs(instruction, stdout); \
        if (is_mem_op) putchar('d'); \
        putchar(' '); \
        __print_location(left); \
        fputs(", ", stdout); \
        __print_location(right); \
        putchar('\n'); \
        if (return_result) { \
            Register r = allocate_register(); \
            Location *loc = calloc(1, sizeof(Location)); \
            loc->type = LOC_REGISTER; \
            loc->as.reg = r; \
            return loc; \
        } \
        break; \
    }


extern uint32_t sizeof_type(Type *t);


bool allocated_registers[NREGISTERS] = { 0 };
Register allocate_register() {
    // use registers B-F for now, register A is reserved for return values
    for (int i = 2; i <= NREGISTERS-4; i++) {
        if (!allocated_registers[i]) {
            allocated_registers[i] = true;
            return (Register) i;
        }
    }

    return NONE;
}

inline void set_reg_used(Register r) {
    allocated_registers[r] = true;
}

inline void free_register(Register r) {
    allocated_registers[r] = false;
}


static void __reg_to_str(Register reg, char *dst) {
    if (REGISTER_A <= reg && reg <= REGISTER_F) {
        dst[0] = 'a' + reg - 1;
    } else if (REGISTER_D0 <= reg && reg <= REGISTER_D3) {
        dst[0] = 'd';
        dst[1] = '0' + reg - REGISTER_D0;
    } else if (reg == REGISTER_SP) {
        dst[0] = 's';
        dst[1] = 'p';
    } else if (reg == REGISTER_BP) {
        dst[0] = 'b';
        dst[1] = 'p';
    } else if (reg == REGISTER_R0 || reg == REGISTER_R1) {
        dst[0] = 'r';
        dst[1] = '0' + reg - REGISTER_R0;
    }

    return;
}

static void __loc_to_str(Location *loc, char *dst) {
    switch (loc->type) {
        case LOC_DATA: {
            if (loc->offset != 0) {
                if (loc->dereference) sprintf(dst, "[$.data+%d]", loc->offset);
                else sprintf(dst, "$.data+%d", loc->offset);
            } else {
                if (loc->dereference) sprintf(dst, "[$.data]");
                else sprintf(dst, "$.data");
            }

            break;
        }

        case LOC_CODE: {
            if (loc->offset != 0) {
                if (loc->dereference) sprintf(dst, "[$.code+%d]", loc->offset);
                else sprintf(dst, "$.code+%d", loc->offset);
            } else {
                if (loc->dereference) sprintf(dst, "[$.code]");
                else sprintf(dst, "$.code");
            }

            break;
        }

        case LOC_REGISTER: {
            char reg[3] = { 0 };
            __reg_to_str(loc->reg, reg);

            if (loc->offset != 0) {
                if (loc->dereference) {
                    sprintf(
                        dst, "[%s%c%d]",
                        reg, (loc->offset >= 0) ? '+' : '-', abs(loc->offset)
                    );
                } else {
                    sprintf(
                        dst, "%s%c%d",
                        reg, (loc->offset >= 0) ? '+' : '-', abs(loc->offset)
                    );
                }
            } else {
                if (loc->dereference) sprintf(dst, "[%s]", reg);
                else sprintf(dst, "%s", reg);
            }

            break;
        }

        case LOC_LITERAL: {
            sprintf(dst, "%d", loc->offset);
        }

        default: {
            break;
        }
    }
}

static Declarator *__get_struct_memb(Type *s, char *memb) {
    if (!s) return NULL;
    
    Type *t = s;
    if (t->type == TYPE_POINTER) t = t->pointee;

    for (int i = 0; i < t->structure.count; i++) {
        if (!strcmp(t->structure.members[i]->ident, memb)) {
            return t->structure.members[i];
        }
    }

    return NULL;
}


int code_offset = 0; // offset in .code
void emit_instruction(const char *fmt, ...) {
    va_list arg;
    va_start(arg, fmt);
    putc('\t', stdout);
    vfprintf(stdout, fmt, arg);
    va_end(arg);

    code_offset += 10;
}


Location register_contents[REGISTER_R1+1];
static void __load_loc_to_reg(Register dst, Location *loc, int extra_offset) {
    loc->offset += extra_offset;

    char op1[3] = { 0 };
    char op2[16] = { 0 };
    __reg_to_str(dst, op1);

    if (loc->type == LOC_LITERAL) {
        __loc_to_str(loc, op2);
        emit_instruction("mov %s, %s\n", op1, op2);
    } else {
        __loc_to_str(loc, op2);

        if (loc->dereference) emit_instruction("movd %s, %s\n", op1, op2);
        else emit_instruction("mov %s, %s\n", op1, op2);
    }

    loc->offset -= extra_offset;
    return;
}

static uint32_t __push_to_stack(Location *loc, Type *t) {
    if (!t || t->type == TYPE_VOID || !loc) return 0;

    char op[16] = { 0 };
    if (t->type == TYPE_STRUCT || t->type == TYPE_ARRAY) {
        uint32_t size = sizeof_type(t);
        Location temp = *loc;

        int offset = size - 4;
        for (; offset >= 0; offset -= 4) {
            temp.offset = offset;
            __loc_to_str(&temp, op);
            
            if (loc->dereference) emit_instruction("pushd %s\n", op);
            else emit_instruction("push %s\n", op);

            memset(op, 0, 16);
        }

        int rem = offset + 4;
        if (rem > 0) {
            __load_loc_to_reg(REGISTER_R0, loc, 0);
        }

        return size;
    }

    __loc_to_str(loc, op);

    if (loc->dereference) emit_instruction("pushd %s\n", op);
    else emit_instruction("push %s\n", op);

    return 4;
}

Location *generate_code(AST_node *node);

void free_location(Location *loc) {
    if (!loc) return;

    if (loc->type == LOC_REGISTER && loc->freeable_reg) {
        free_register(loc->reg);
    }

    free(loc);
    return;
}

static char *__cond_jmp_ins[] = {"jl", "jg", "je", "jle", "jge", "jne"};
static char *__cond_inv_jmp_ins[] = {"jge", "jle", "jne", "jg", "jl", "je"};

static void __gen_condition(AST_node *condition, char *true_label, char *false_label) {
    if (OP_LESS <= condition->as.binary_op.op && condition->as.binary_op.op <= OP_NE) {
        Location *left = generate_code(condition->as.binary_op.left);
        Location *right = generate_code(condition->as.binary_op.right);

        Location left_final = *left;
        Location right_final = *right;

        bool left_literal = true;
        if (left->type == LOC_DATA || left->reg == REGISTER_BP) {
            __load_loc_to_reg(REGISTER_R0, left, 0);

            left_literal = false;
            left_final.type = LOC_REGISTER;
            left_final.reg = REGISTER_R0;
            left_final.offset = 0;
            left_final.dereference = false;
        }

        if (right->type != LOC_LITERAL) {
            Register r = (left_literal) ? REGISTER_R0 : REGISTER_R1;
            __load_loc_to_reg(r, right, 0);

            right_final.type = LOC_REGISTER;
            right_final.reg = r;
            right_final.offset = 0;
            right_final.dereference = false;
        }

        free_location(left);
        free_location(right);

        enum binop_operator operator = condition->as.binary_op.op;

        // if the left operand is a literal, invert the operand order and comparison operator
        if (left_final.type == LOC_LITERAL) {
            Location temp = right_final;

            /*
                In the binop_operator enum, the "greater" operators are numerical successors
                of the "less" operators.

                i.e. OP_GREATER = OP_LESS + 1
            */
            if (operator == OP_LESS || operator == OP_LE) operator++;
            else if (operator == OP_GREATER || operator == OP_GE) operator--;

            right_final = left_final;
            left_final = temp;
        }

        char *instruction = __cond_jmp_ins[operator - OP_LESS];
        char *inv_instruction = __cond_inv_jmp_ins[operator - OP_LESS];

        char op1[16] = { 0 };
        char op2[16] = { 0 };
        __loc_to_str(&left_final, op1);
        __loc_to_str(&right_final, op2);

        emit_instruction("cmp %s, %s\n", op1, op2);

        if (true_label != NULL) {
            emit_instruction("%s %s\n", instruction, true_label);
            
            if (false_label != NULL) emit_instruction("jmp %s\n", false_label);
        } else {
            emit_instruction("%s %s\n", inv_instruction, false_label);
        }
    } else if (condition->as.binary_op.op == OP_LOGICAL_AND) {
        /*
            If a condition is of the form (c1 && c2), if c1 is false, jump to the false label instantly,
            without evaluating c2 (short circuit)
        */

        __gen_condition(condition->as.binary_op.left, NULL, false_label);
        __gen_condition(condition->as.binary_op.right, true_label, false_label);
    } else if (condition->as.binary_op.op == OP_LOGICAL_OR) {
        // basically the opposite of logical and (for the evaluation of the first expression)

        __gen_condition(condition->as.binary_op.left, true_label, NULL);
        __gen_condition(condition->as.binary_op.right, true_label, false_label);
    } else if (condition->type == AST_UNARY_OP && condition->as.unary_op.op == OP_LOGICAL_NOT) {
        __gen_condition(condition->as.unary_op.operand, false_label, true_label);
    } else {
        Location *operand = generate_code(condition);
        __load_loc_to_reg(REGISTER_R0, operand, 0);

        emit_instruction("cmp r0, 0\n");

        if (true_label != NULL) {
            emit_instruction("jnz %s\n", true_label);
            if (false_label != NULL) emit_instruction("jmp %s\n", false_label);
        } else {
            emit_instruction("jz %s\n", false_label);
        }

        free_location(operand);
        return;
    }

    return;
}


int __eval_const_expr(AST_node *node);

extern void print_type(Type *t, int d);

static char *loop_init_label = NULL;
static char *loop_exit_label = NULL;

// returns the register in which the result is stored
Location *generate_code(AST_node *node) {
    switch (node->type) {
        case AST_INT_LITERAL: {
            Location *loc = calloc(1, sizeof(Location));

            loc->type = LOC_LITERAL;
            loc->dereference = false;
            loc->offset = node->as.int_literal.value;
            
            return loc;
        }

        case AST_VAR_REF: {
            Location *loc = calloc(1, sizeof(Location));
            *loc = *(node->as.var_ref.sym->loc);

            return loc;
        }

        case AST_VAR_DECL: {
            // global variables are placed in .data
            if (node->as.var_decl.sym->scope_depth == 0) {
                if (node->as.var_decl.init == NULL) {
                    printf("times %d db 0", sizeof_type(node->as.var_decl.sym->decl->type));
                } else {
                    // TODO: arrays and structs
                    // int val = __eval_const_expr(node->as.var_decl.init);
                    int val = 0;
                    if (sizeof_type(node->as.var_decl.decl->type) == 4) {
                        printf("dd %d", val);
                    } else {
                        // if the type size is not 4 (int or pointer) it would be 1 (char)
                        printf("db %d", (char) val);
                    }
                }

                #ifdef __GENERATOR_ANNOTATE_SYMBOL_NAMES
                printf(
                    " ; var %s @ off %d",
                    node->as.var_decl.decl->ident,
                    node->as.var_decl.sym->loc->offset
                );
                #endif

                printf("\n");

                Location *loc = calloc(1, sizeof(Location));
                *loc = *(node->as.var_decl.sym->loc);

                return loc;
            }

            Location *loc = calloc(1, sizeof(Location));
            *loc = *(node->as.var_decl.sym->loc);

            if (node->as.var_decl.init != NULL) {
                Location *l = generate_code(node->as.var_decl.init);
                
                char op1[16] = { 0 };
                char op2[16] = { 0 };
                __loc_to_str(loc, op1);
                __loc_to_str(l, op2);

                emit_instruction("movd %s, %s\n", op1, op2);
                free_location(l);
            }

            return loc;
        }

        case AST_BINARY_OP: {
            switch (node->as.binary_op.op) {
                case OP_ADD:
                case OP_SUB:
                case OP_STAR: {
                    Location *left = generate_code(node->as.binary_op.left);
                    Location *right = generate_code(node->as.binary_op.right);

                    char left_s[16] = { 0 };
                    char right_s[16] = { 0 };
                    __loc_to_str(left, left_s);
                    __loc_to_str(right, right_s);

                    char res_s[16] = { 0 };
                    char opcode[3][4] = {"add\0", "sub\0", "mul\0"};

                    Register reg = allocate_register();
                    Location *loc = calloc(1, sizeof(Location));
                    loc->type = LOC_REGISTER;
                    loc->reg = reg;
                    loc->freeable_reg = true;
                    loc->dereference = false;

                    __loc_to_str(loc, res_s);
                    
                    if (left->dereference) emit_instruction("movd %s, %s\n", res_s, left_s);
                    else emit_instruction("mov %s, %s\n", res_s, left_s);

                    // perform the operation
                    if (right->dereference) {
                        emit_instruction("%sd %s, %s\n", opcode[node->as.binary_op.op], res_s, right_s);
                    } else {
                        emit_instruction("%s %s, %s\n", opcode[node->as.binary_op.op], res_s, right_s);
                    }

                    free_location(left);
                    free_location(right);

                    return loc;
                }

                case OP_LSH:
                case OP_RSH:
                case OP_AND:
                case OP_XOR:
                case OP_OR: {
                    // these instructions do not allow memory dereferences in their operands
                    
                    Location *left = generate_code(node->as.binary_op.left);
                    Location *right = generate_code(node->as.binary_op.right);

                    char left_s[16] = { 0 };
                    char right_s[16] = { 0 };
                    __loc_to_str(left, left_s);
                    __loc_to_str(right, right_s);

                    char op1[16] = { 0 };
                    char op2[16] = { 0 };
                    char opcode[5][4] = {"lsh\0", "rsh\0", "and\0", "xor\0", "or\0"};
                    
                    Location *res = calloc(1, sizeof(Location));
                    if (!left->dereference && left->type == LOC_REGISTER) {
                        *res = *left;
                    } else {
                        // move the left operand into a result register
                        Register reg = allocate_register();
                        res->type = LOC_REGISTER;
                        res->reg = reg;
                        res->freeable_reg = true;
                        res->dereference = false;

                        __load_loc_to_reg(reg, left, 0);
                    }

                    Location *right_temp = calloc(1, sizeof(Location));
                    if (!right->dereference) {
                        *right_temp = *right;
                    } else {
                        Register reg = allocate_register();
                        right_temp->type = LOC_REGISTER;
                        right_temp->reg = reg;
                        right_temp->freeable_reg = true;
                        right_temp->dereference = false;

                        __load_loc_to_reg(reg, right, 0);
                    }

                    __loc_to_str(res, op1);
                    __loc_to_str(right_temp, op2);

                    emit_instruction(
                        "%s %s, %s\n",
                        opcode[node->as.binary_op.op - OP_LSH],
                        op1, op2
                    );

                    free_location(right);
                    free_location(right_temp);

                    /*
                        If the left operand has the appropriate location (register, no dereference),
                        it is very likely that it is a register allocated by allocate_register().
                        free_location() frees an allocated register as well, but the register cannot be 
                        freed now since it stores the result of the operation, so we can only free its location
                        pointer.
                    */
                    if (left->dereference || left->type != LOC_REGISTER) {
                        free_location(left);
                    } else {
                        free(left);
                    }

                    return res;
                }

                case OP_LESS:
                case OP_GREATER:
                case OP_EQUAL:
                case OP_LE:
                case OP_GE:
                case OP_NE:
                case OP_LOGICAL_AND:
                case OP_LOGICAL_OR: {
                    /*
                        The left and right nodes of these binary operators are evaluated by __gen_condition().
                        If they were evaluated at the root of the AST_BINARY_OP case, the resulting assembly code
                        will contain a significant amount of dead code.
                    */

                    Location *res = calloc(1, sizeof(Location));
                    res->type = LOC_REGISTER;
                    res->reg = allocate_register();
                    res->freeable_reg = true;
                    res->dereference = false;

                    char true_label[19] = { 0 };
                    char false_label[20] = { 0 };
                    char exit_label[19] = { 0 };
                    
                    char res_s[3] = { 0 };
                    __reg_to_str(res->reg, res_s);

                    sprintf(true_label, "cond_true_%08x", code_offset);
                    sprintf(false_label, "cond_false_%08x", code_offset);
                    sprintf(exit_label, "cond_exit_%08x", code_offset);

                    __gen_condition(node, true_label, false_label);

                    printf("\n%s:\n", true_label);
                    emit_instruction("mov %s, 1\n", res_s);
                    emit_instruction("jmp %s\n", exit_label);

                    printf("\n%s:\n", false_label);
                    emit_instruction("xor %s, %s\n", res_s, res_s);
                    
                    printf("\n%s:\n", exit_label);

                    return res;
                }

                case OP_ASSIGN: {
                    Location *left = generate_code(node->as.binary_op.left);
                    Location *right = generate_code(node->as.binary_op.right);

                    char left_s[16] = { 0 };
                    char right_s[16] = { 0 };
                    __loc_to_str(left, left_s);
                    __loc_to_str(right, right_s);

                    if (right->dereference || left->dereference) {
                        emit_instruction("movd %s, %s\n", left_s, right_s);
                    } else {
                        emit_instruction("mov %s, %s\n", left_s, right_s);
                    }

                    if (right->type == LOC_REGISTER) {
                        free_register(right->reg);
                    }

                    free_location(left);
                    free_location(right);

                    return NULL;
                }

                default: {
                    return NULL;
                }
            }
        }

        case AST_UNARY_OP: {
            Location *operand = generate_code(node->as.unary_op.operand);

            switch (node->as.unary_op.op) {
                case OP_INCREMENT:
                case OP_DECREMENT: {
                    char *instruction[3] = {"add", "sub"};
                    char op[16] = { 0 };
                    __loc_to_str(operand, op);

                    if (operand->dereference) {
                        emit_instruction(
                            "%sd %s, 1\n",
                            instruction[node->as.unary_op.op - OP_INCREMENT], op
                        );
                    } else {
                        emit_instruction(
                            "%s %s, 1\n",
                            instruction[node->as.unary_op.op - OP_INCREMENT], op
                        );
                    }

                    return operand;
                }

                case OP_DEREFERENCE: {
                    if (!operand->dereference) {
                        operand->dereference = true;
                        return operand;
                    }

                    // if the operand is already a dereference, we need its value stored in an accumulator
                    char op[16] = { 0 };
                    char ac[16] = { 0 };
                    __loc_to_str(operand, op);
                    
                    Location *accumulator = malloc(sizeof(Location));
                    accumulator->type = LOC_REGISTER;
                    accumulator->reg = allocate_register();
                    accumulator->dereference = false;
                    __loc_to_str(accumulator, ac);

                    emit_instruction("movd %s, %s\n", ac, op);
                    accumulator->dereference = true;

                    free_location(operand);
                    return accumulator;
                }

                case OP_ADDRESS_OF: {
                    Location *operand = generate_code(node->as.unary_op.operand);

                    if (operand->dereference) {
                        operand->dereference = false;
                        return operand;
                    }

                    free_location(operand);
                    return NULL;
                }

                default: {
                    return NULL;
                }
            }
        }

        case AST_IF: {
            char true_label[19] = { 0 };
            char false_label[20] = { 0 };
            char exit_label[19] = { 0 };

            for (int i = 0; i < node->as.if_statement.count; i++) {
                sprintf(true_label, "cond_true_%08x", code_offset);
                sprintf(false_label, "cond_false_%08x", code_offset);
                sprintf(exit_label, "cond_exit_%08x", code_offset);

                /*
                    If this is not the last condition or there is an else branch, emit at the end of the label
                    a jump to an exit label
                */
                if (i+1 != node->as.if_statement.count || node->as.if_statement.else_branch != NULL) {
                    __gen_condition(node->as.if_statement.conditions[i], true_label, false_label);
                } else {
                    __gen_condition(node->as.if_statement.conditions[i], true_label, exit_label);
                }

                printf("\n%s:\n", true_label);
                Location *l = generate_code(node->as.if_statement.blocks[i]);

                if (i+1 != node->as.if_statement.count || node->as.if_statement.else_branch != NULL) {
                    emit_instruction("jmp %s\n", exit_label);
                    printf("\n%s:\n", false_label);
                }

                free_location(l);
            }

            if (node->as.if_statement.else_branch) {
                Location *l = generate_code(node->as.if_statement.else_branch);
                free_location(l);
            }

            printf("\n%s:\n", exit_label);

            return NULL;
        }

        case AST_WHILE: {
            char label[14] = { 0 };
            char exit_label[19] = { 0 };

            sprintf(label, "loop_%08x", code_offset);
            sprintf(exit_label, "loop_%08x_exit", code_offset);

            loop_init_label = strdup(label);
            loop_exit_label = strdup(exit_label);

            printf("\n%s:\n", label);
            __gen_condition(node->as.while_statement.condition, NULL, exit_label);
            Location *l = generate_code(node->as.while_statement.body);
            
            emit_instruction("jmp %s\n", label);

            free_location(l);
            printf("\n%s:\n", exit_label);

            free(loop_init_label);
            free(loop_exit_label);
            loop_init_label = NULL;
            loop_exit_label = NULL;
            return NULL;
        }

        case AST_FOR: {
            Location *temp;

            if (node->as.for_statement.init->type != AST_VAR_DECL) {
                temp = generate_code(node->as.for_statement.init);
            } else {
                /*
                    If the loop initializer contains a variable declaration, evaluate the initializer of the
                    declaration and save it directly to register C. Loading it into the stack is unnecessary,
                    since for the entire lifetime of the loop scope, the loop variable is stored into register C.

                    TODO: Add a compiler error for attempting to access the address of a loop variable.
                    (that cannot be done since it is stored in a register)
                */
                temp = generate_code(node->as.for_statement.init->as.var_decl.init);
            }

            Location counter;
            counter.type = LOC_REGISTER;
            counter.reg = REGISTER_C;
            counter.offset = 0;
            counter.dereference = false;

            emit_instruction("push c\n");
            set_reg_used(REGISTER_C);

            __load_loc_to_reg(REGISTER_C, temp, 0);
            free_location(temp);

            Location *orig = NULL;

            if (node->as.for_statement.init->type == AST_VAR_DECL) {
                free(node->as.for_statement.init->as.var_decl.sym->loc);
                node->as.for_statement.init->as.var_decl.sym->loc = &counter;
            } else {
                orig = node->as.for_statement.init->as.binary_op.left->as.var_ref.sym->loc;
                node->as.for_statement.init->as.binary_op.left->as.var_ref.sym->loc = &counter;
            }

            char label[14] = { 0 };
            char exit_label[19] = { 0 };
            
            sprintf(label, "loop_%08x", code_offset);
            sprintf(exit_label, "loop_%08x_exit", code_offset);
            printf("\n%s:\n", label);

            loop_init_label = strdup(label);
            loop_exit_label = strdup(exit_label);

            __gen_condition(node->as.for_statement.condition, NULL, exit_label);
            
            Location *l = generate_code(node->as.for_statement.body);
            free_location(l);

            l = generate_code(node->as.for_statement.updation);
            free_location(l);

            emit_instruction("jmp %s\n", label);
            printf("\n%s:\n", exit_label);
            
            emit_instruction("pop c\n");
            free_register(REGISTER_C);

            free(loop_init_label);
            free(loop_exit_label);
            loop_init_label = NULL;
            loop_exit_label = NULL;

            if (orig != NULL) node->as.for_statement.init->as.binary_op.left->as.var_ref.sym->loc = orig;
            return NULL;
        }

        case AST_RETURN: {
            Location *return_value = generate_code(node->as.return_statement.expr);

            char operand[16] = { 0 };
            __loc_to_str(return_value, operand);

            if (return_value->dereference) {
                emit_instruction("movd a, %s\n", operand);
            } else {
                if (!(return_value->type == LOC_REGISTER && return_value->reg == REGISTER_A)) {
                    emit_instruction("mov a, %s\n", operand);
                }
            }

            // restore stack pointer in order for ret to find its return frame
            emit_instruction("mov sp, bp\n");
            emit_instruction("pop bp\n");
            emit_instruction("ret\n");

            // if (return_value->type == LOC_REGISTER) free_register(return_value->reg);
            free_location(return_value);

            break;
        }

        case AST_FUNCTION_DECL: {
            node->as.function_decl.sym->loc->type = LOC_CODE;
            node->as.function_decl.sym->loc->offset = code_offset;
            node->as.function_decl.sym->loc->dereference = false;

            printf("\n%s:\n", node->as.function_decl.decl->ident);
            emit_instruction("push bp\n");
            emit_instruction("mov bp, sp\n");

            Location *l = generate_code(node->as.function_decl.body);
            free_location(l);
            break;
        }

        case AST_FUNCTION_CALL: {
            Location *callee = generate_code(node->as.function_call.callee);

            // push arguments to the stack in right to left order (cdecl)
            uint32_t arg_frame_size = 0;
            for (int i = node->as.function_call.count-1; i >= 0; i--) {
                Location *arg_loc = generate_code(node->as.function_call.args[i]);
                Type *arg_type = node->as.function_call.args[i]->expr_type;

                arg_frame_size += __push_to_stack(arg_loc, arg_type);
                free_location(arg_loc);
            }

            /*
                If the callee is a reference to a function symbol, just call the label
                with the name of that reference. If not, use the callee's evaluated location.
            */
            if (
                node->as.function_call.callee->type == AST_VAR_REF && \
                node->as.function_call.callee->as.var_ref.sym->type == SYMBOL_FUNC
            ) {
                emit_instruction("call %s\n", node->as.function_call.callee->as.var_ref.name);
            } else {
                // char callee_str[16] = { 0 };
                // __loc_to_str(callee, callee_str);
                __load_loc_to_reg(REGISTER_R0, callee, 0);
                emit_instruction("call r0\n");
            }

            // after function returns restore stack frame
            if (arg_frame_size > 0) {
                emit_instruction("add sp, %d\n", arg_frame_size);
            }

            free_location(callee);

            // return value is stored in register A
            Location *ret = calloc(1, sizeof(Location));
            ret->type = LOC_REGISTER;
            ret->reg = REGISTER_A;

            return ret;
        }

        case AST_BREAK: {
            if (loop_exit_label != NULL) {
                emit_instruction("jmp %s\n", loop_exit_label);
            } 

            return NULL;
        }

        case AST_CONTINUE: {
            if (loop_init_label != NULL) {
                emit_instruction("jmp %s\n", loop_init_label);
            } 

            return NULL;
        }

        case AST_STRUCT_ACCESS: {
            Location *src = generate_code(node->as.struct_access.src);
            Location *temp = calloc(1, sizeof(Location));

            Declarator *memb_type = __get_struct_memb(
                node->as.struct_access.src->expr_type,
                node->as.struct_access.member
            );

            // node->as.struct_access.src has to evaluate to a struct or a pointer to a struct
            if (node->as.struct_access.pointer) {
                // dereference pointer to struct (not struct member !!)

                // TODO: Add register caching
                __load_loc_to_reg(REGISTER_R0, src, 0);
                temp->type = LOC_REGISTER;
                temp->reg = REGISTER_R0;

                // dereference struct member
                temp->offset = memb_type->offset;
            } else {
                temp->type = src->type;
                temp->reg = src->reg;
                temp->offset = src->offset - memb_type->offset;
            }

            temp->dereference = true;
            free_location(src);
            return temp;
        }

        case AST_ARRAY_ACCESS: {
            Location *array = generate_code(node->as.array_access.array);
            Location *index = generate_code(node->as.array_access.index);

            Location temp = *array;
            temp.dereference = false;
            
            __load_loc_to_reg(REGISTER_R0, &temp, 0);

            if (!(index->type == LOC_LITERAL && index->offset == 0)) {
                uint32_t memb_size = sizeof_type(node->as.array_access.array->expr_type->array.memb_type);

                if (index->type == LOC_LITERAL) {
                    emit_instruction("add r0, %u\n", index->offset * memb_size);
                } else {
                    Register r = allocate_register();

                    char reg[3] = { 0 };
                    char offset[16] = { 0 };
                    __reg_to_str(r, reg);
                    __loc_to_str(index, offset);

                    if (index->dereference) emit_instruction("movd %s, %s\n", reg, offset);
                    else emit_instruction("mov %s, %s\n", reg, offset);

                    if (memb_size != 1) {
                        emit_instruction("mul %s, %u\n", reg, memb_size);
                    }

                    emit_instruction("add r0, %s\n", reg);
                    free_register(r);
                }
            }

            Location *loc = calloc(1, sizeof(Location));
            loc->type = LOC_REGISTER;
            loc->reg = REGISTER_R0;
            loc->dereference = true;

            free_location(array);
            free_location(index);

            return loc;
        }

        case AST_TYPE_CAST: {
            /*
                TODO: emit code for float <-> int and char <-> int using
                ftoi, itof and movsx (sign extend)
            */

            return generate_code(node->as.type_cast.operand);
        }

        case AST_SIZEOF: {
            Location *res = calloc(1, sizeof(Location));
            res->type = LOC_LITERAL;
            res->offset = sizeof_type(node->as.sizeof_.operand_type);

            return res;
        }

        case AST_BLOCK: {
            for (int i = 0; i < node->as.block.count; i++) {
                Location *l = generate_code(node->as.block.statements[i]);
                free_location(l);
            }
        }

        default: {
            return NULL;
        }
    }
    
    return NULL;
}