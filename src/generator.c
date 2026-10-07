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
    // use registers A-F for now
    for (int i = 1; i <= NREGISTERS-4; i++) {
        if (!allocated_registers[i]) {
            allocated_registers[i] = true;
            return (Register) i;
        }
    }

    return NONE;
}

inline void free_register(Register r) {
    allocated_registers[r] = false;
}


void __reg_to_str(Register reg, char *dst) {
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

void __loc_to_str(Location *loc, char *dst) {
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

int code_offset = 0; // offset in .code
void emit_instruction(const char *fmt, ...) {
    va_list arg;
    va_start(arg, fmt);
    putc('\t', stdout);
    vfprintf(stdout, fmt, arg);
    va_end(arg);

    code_offset += 10;
}


static void __load_loc_to_reg(Register dst, Location *loc, int extra_offset) {
    int offset = loc->offset + extra_offset;
    bool dereference = loc->dereference;

    Location temp = *loc;
    temp.offset = 0;
    temp.dereference = false;

    char op1[3] = { 0 };
    char op2[16] = { 0 };
    __reg_to_str(dst, op1);

    if (loc->type == LOC_LITERAL) {
        __loc_to_str(loc, op2);
        emit_instruction("mov %s, %s\n", op1, op2);
        
        return;
    } else if (loc->type == LOC_DATA) {
        // $.data is a constant and if it is offset, the result is calculated during compile-time
        __loc_to_str(loc, op2);
        if (loc->dereference) emit_instruction("movd %s, %s\n", op1, op2);
        else emit_instruction("mov %s, %s\n", op1, op2);

        return;
    }

    __loc_to_str(&temp, op2);

    /*
        The assembler converts in-operand operations (i.e. mov a, b+1) into multiple instructions
        that move the initial value into an accumulator, perform the calculation,
        and finally move the result to the destination register.

        The easy solution to this is checking if the offset is nonzero, emitting the instruction with the in-operand
        operation, incrementing the code offset by 30 (3 instructions instead of 1) and letting the assembler handle
        the rest. However, this obviously creates a heavy dependance on the assembler's behavior. That is not optimal,
        therefore this process is done manually here.
    */

    // LOC_DATA and LOC_REGISTER follow the exact same logic
    if (offset != 0) {
        Register temp_scratch_reg = dst;
        char reg_string[3] = { 0 };

        /*
            In the case of a dereference, the contents of the target would need to
            be dereferenced before being moved to the destination register.
        */
        if (loc->dereference) temp_scratch_reg = allocate_register();
        __reg_to_str(temp_scratch_reg, reg_string);

        // move value-to-be-offset into the accumulator & perform offset calculation
        emit_instruction("mov %s, %s\n", reg_string, op2);
        emit_instruction(
            "%s %s, %d\n",
            (offset > 0) ? "add" : "sub", reg_string, abs(offset)
        );

        if (loc->dereference) {
            emit_instruction("movd %s, [%s]\n", op1, reg_string);
            free_register(temp_scratch_reg);
        } // if dereference is unset, the destination already contains the expected result
    } else {
            if (loc->dereference) {
                emit_instruction("movd %s, [%s]\n", op1, op2);
            } else {
                if (loc->reg != dst) {
                    emit_instruction("mov %s, %s\n", op1, op2);
                } // else: mov a, a
            }
        }

    return;
}

uint32_t __push_to_stack(Location *loc, Type *t) {
    if (!t || t->type == TYPE_VOID || !loc) return 0;

    if (t->type == TYPE_STRUCT || t->type == TYPE_ARRAY) {
        uint32_t size = sizeof_type(t);

        int offset = size - 4;
        for (; offset >= 0; offset -= 4) {
            __load_loc_to_reg(REGISTER_R0, loc, offset);
            emit_instruction("push r0\n");
            
        }

        int rem = offset + 4;
        if (rem > 0) {
            __load_loc_to_reg(REGISTER_R0, loc, 0);
        }

        return size;
    }

    if (loc->dereference) {
        __load_loc_to_reg(REGISTER_R0, loc, 0);
        emit_instruction("push r0\n");
    } else {
        char operand[16] = { 0 };
        __loc_to_str(loc, operand);
        
        emit_instruction("push %s\n", operand);
    }

    return 4;
}

int __eval_const_expr(AST_node *node);

// returns the register in which the result is stored
Location *generate_code(AST_node *node) {
    Location *loc = malloc(sizeof(Location));

    switch (node->type) {
        case AST_INT_LITERAL: {
            loc->type = LOC_LITERAL;
            loc->dereference = false;
            loc->offset = node->as.int_literal.value;
            
            return loc;
        }

        case AST_VAR_REF: {
            if (node->as.var_ref.sym->scope_depth == 0) { // could be a function
                loc->type = LOC_DATA;
                loc->dereference = true;
                loc->offset = node->as.var_ref.sym->stack_offset;
            } else {
                loc->type = LOC_REGISTER;
                loc->reg = REGISTER_BP;
                loc->dereference = true;
                loc->offset = node->as.var_ref.sym->stack_offset;
            }

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
                printf(" ; var %s @ off %d", node->as.var_decl.decl->ident, node->as.var_decl.sym->stack_offset);
                #endif

                printf("\n");
            }

            break;
        }

        case AST_BINARY_OP: {
            Location *left = generate_code(node->as.binary_op.left);
            Location *right = generate_code(node->as.binary_op.right);

            char left_s[16] = { 0 };
            char right_s[1] = { 0 };
            __loc_to_str(left, left_s);
            __loc_to_str(right, right_s);

            switch (node->as.binary_op.op) {
                case OP_ADD:
                case OP_SUB:
                case OP_STAR: {
                    Location *l;
                    char res_s[16] = { 0 };
                    char opcode[3][3] = {"add", "sub", "mul"};

                    // setup accumulator
                    if (left->type != LOC_REGISTER) {
                        Register reg = allocate_register();
                        l = malloc(sizeof(Location));
                        l->type = LOC_REGISTER;
                        l->reg = reg;
                        l->dereference = false;

                        __loc_to_str(l, res_s);

                        // load left operand into accumulator
                        if (left->dereference) emit_instruction("movd %s, %s\n", res_s, left_s);
                        else emit_instruction("mov %s, %s\n", res_s, left_s);

                        free(left);
                    } else {
                        l = left;
                        __loc_to_str(l, res_s);
                    }

                    // perform the operation
                    if (right->dereference) {
                        emit_instruction("%sd %s, %s\n", opcode[node->as.binary_op.op], res_s, right_s);
                    } else {
                        emit_instruction("%s %s, %s\n", opcode[node->as.binary_op.op], res_s, right_s);
                    }

                    if (right->type == LOC_REGISTER) {
                        free_register(right->reg);
                    }
                    free(right);

                    return l;
                }

                case OP_ASSIGN: {
                    if (right->dereference || left->dereference) {
                        emit_instruction("movd %s, %s\n", left_s, right_s);
                    } else {
                        emit_instruction("mov %s, %s\n", left_s, right_s);
                    }

                    if (right->type == LOC_REGISTER) {
                        free_register(right->reg);
                    }

                    free(left);
                    free(right);

                    return NULL;
                }

                default: {
                    return NULL;
                }
            }
        }

        case AST_UNARY_OP: {
            switch (node->as.unary_op.op) {
                case OP_DEREFERENCE: {
                    Location *operand = generate_code(node->as.unary_op.operand);

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

                    free(operand);
                    return accumulator;
                }

                case OP_ADDRESS_OF: {
                    Location *operand = generate_code(node->as.unary_op.operand);

                    if (operand->dereference) {
                        operand->dereference = false;
                        return operand;
                    }

                    return NULL;
                }

                default: {
                    return NULL;
                }
            }
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
            free(return_value);

            break;
        }

        case AST_FUNCTION_DECL: {
            printf("\n%s:\n", node->as.function_decl.decl->ident);
            emit_instruction("push bp\n");
            emit_instruction("mov bp, sp\n");

            generate_code(node->as.function_decl.body);
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
                free(arg_loc);
            }

            if (
                node->as.function_call.callee->type == AST_VAR_REF && \
                node->as.function_call.callee->as.var_ref.sym->type == SYMBOL_FUNC
            ) {
                emit_instruction("call %s\n", node->as.function_call.callee->as.var_ref.name);
            } else {
                char callee_str[16] = { 0 };
                __loc_to_str(callee, callee_str);
                emit_instruction("call %s\n", callee_str);
            }

            // after function returns
            if (arg_frame_size > 0) {
                emit_instruction("add sp, %d\n", arg_frame_size);
            }

            free(callee);

            // return value is stored in register A
            Location *ret = calloc(1, sizeof(Location));
            ret->type = LOC_REGISTER;
            ret->reg = REGISTER_A;

            return ret;
        }

        case AST_STRUCT_ACCESS: {
            Location *src = generate_code(node->as.struct_access.src);
        }

        case AST_BLOCK: {
            for (int i = 0; i < node->as.block.count; i++) {
                generate_code(node->as.block.statements[i]);
            }
        }

        default: {
            return NULL;
        }
    }
    
    return NULL;
}