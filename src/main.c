#include <stdio.h>
#include <stdlib.h>

#include <ast.h>
#include <tokenizer.h>
#include <generator.h>


extern const char *token_to_string(enum tokentype token);
extern void print_AST(AST_node*);
extern void free_AST(AST_node*);

extern char *filename;
extern token_t *arr;


char program[4096];

extern void print_type(Type *t, int d);
extern void analyze_AST(AST_node *program, SymbolTable *tab);
extern Location *generate_code(AST_node *node);

int main(int argc, char *argv[]) {
    if (argc == 1) {
        printf("Error: no input files\n");
        return -1;
    }

    filename = argv[1];

    arr = (token_t *) calloc(512, sizeof(token_t));
    void *orig_arr_ptr = arr;
    FILE *f = fopen(filename, "r");
    
    fseek(f, 0, SEEK_END);
    uint32_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    fread(program, 1, size, f);

    uint32_t tokens = tokenize(program, arr);
    printf("%d tokens parsed\n", tokens);
    printf("\n=========================\n");

    // for (int i = 0; i < tokens; i++) {
    //     printf("%s ", token_to_string(arr[i].type));
    // }

    // printf("\n=========================\n");
    
    AST_node *program = parse_program();

    SymbolTable table;
    analyze_AST(program, &table);

    // print_AST(program);

    // walk 1: global declarations
    puts("section .data");
    for (int i = 0; i < program->as.program.count; i++) {
        AST_node *decl = program->as.program.declarations[i];

        if (decl->type == AST_VAR_DECL) generate_code(decl);
    }

    // walk 2: everything else
    puts("\nsection .code");
    for (int i = 0; i < program->as.program.count; i++) {
        AST_node *decl = program->as.program.declarations[i];

        if (decl->type != AST_VAR_DECL) generate_code(decl);
    }

    // print_AST(program);

    free_AST(program);
    free(orig_arr_ptr);
    return 0;
}