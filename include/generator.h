#pragma once

#include <ast.h>

#define NREGISTERS 10
#define __GENERATOR_ANNOTATE_SYMBOL_NAMES


typedef enum {
    NONE,
    REGISTER_A,
    REGISTER_B,
    REGISTER_C,
    REGISTER_D,
    REGISTER_E,
    REGISTER_F,
    REGISTER_D0,
    REGISTER_D1,
    REGISTER_D2,
    REGISTER_D3,
    REGISTER_SP,
    REGISTER_BP,
    REGISTER_R0,
    REGISTER_R1
} Register;

enum location_type {
    LOC_DATA,
    LOC_REGISTER,
    LOC_LITERAL
};

typedef struct {
    enum location_type type;

    /*
        This field indicates that the location falls into 
        any of the following cases:
        1. Access variable value in stack
        2. Access variable value in .data
        3. Dereference operator is used

        If the address of operator (&) is used on cases 1 and 2,
        dereference will be set to false.

        so in short this field indicates whether the 
        location represented by this struct needs to be
        dereferenced in the emitted code.

        (long ass comment because 2 days after adding this
        I was very confused because of it)
    */
    bool dereference;

    Register reg;
    int offset;
} Location;


Register allocate_register();
void free_register(Register r);