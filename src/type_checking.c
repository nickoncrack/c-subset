#include <ast.h>
#include <typing.h>
#include <common.h>


bool typecmp(Type *a, Type *b) {
    if (a == b) return true;
    if (a->type != b->type) return false;

    switch (a->type) {
        case TYPE_VOID:
        case TYPE_INT:
        case TYPE_CHAR: {
            return true;
        }

        case TYPE_POINTER: {
            return typecmp(a->pointee, b->pointee);
        }

        case TYPE_FUNCTION: {
            if (a->function.count != b->function.count) return false;
            if (!typecmp(a->function.return_type, b->function.return_type)) return false;

            bool flag = 1;
            for (int i = 0; i < a->function.count; i++) {
                flag &= typecmp(a->function.params[i]->type, b->function.params[i]->type);
                if (!flag) return false;
            }

            return true;
        }

        case TYPE_ARRAY: {
            if (a->array.count != b->array.count) return false;
            return typecmp(a->array.memb_type, b->array.memb_type);
        }

        case TYPE_STRUCT: {
            if (a->structure.count != b->structure.count) return false;

            bool flag = 1;
            for (int i = 0; i < a->structure.count; i++) {
                flag &= typecmp(a->structure.members[i]->type, b->structure.members[i]->type);
                if (!flag) return false;
            }

            return true;
        }
    }
}

bool is_integer_type(Type *t) {
    return t && (t->type == TYPE_INT || t->type == TYPE_CHAR);
}

bool is_pointer_type(Type *t) {
    return t && (t->type == TYPE_POINTER || t->type == TYPE_FUNCTION);
}

bool integer_compatible(Type *a, Type *b) {
    return is_integer_type(a) && is_integer_type(b);
}

bool pointer_compatible(Type *a, Type *b) {
    if (is_pointer_type(a) && is_pointer_type(b)) {
        if (a->pointee->type == TYPE_VOID || b->pointee->type == TYPE_VOID) {
            return true;
        }
    }

    return false;
}

bool is_assignable(Type *dst, Type *src) {
    if (typecmp(dst, src)) return true;
    if (integer_compatible(dst, src)) return true;
    if (pointer_compatible(dst, src)) return true;

    return false;
}


#define POINTER_OR_INT(a, b) \
    ((is_integer_type(a) && b->type == TYPE_POINTER) || \
    (is_integer_type(b) && a->type == TYPE_POINTER))

bool is_castable(Type *cast_type, Type *operand) {
    // if operand is assignable to cast_type, cast-ability is implied
    if (is_assignable(cast_type, operand)) return true;

    // pointer to/from integer casts are allowed
    if (POINTER_OR_INT(cast_type, operand)) return true;

    // pointer to pointer casts are allowed (not assignable)
    return cast_type->type == TYPE_POINTER && operand->type == TYPE_POINTER;
}

bool is_addable(Type *a, Type *b) {
    if (integer_compatible(a, b)) return true;
    return POINTER_OR_INT(a, b);
}