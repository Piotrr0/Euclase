#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "tests.h"
#include <stdio.h>
#include <stdlib.h>


void compile_source_file(int argc, char** argsv)
{
    if(argc != 2)
        return;
    
    FILE* fptr;
    fptr = fopen(argsv[1], "r");
    fseek(fptr, 0, SEEK_END);
    int fsize = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);


    char* code = malloc(fsize + 1);
    fread(code, fsize, 1, fptr);
    fclose(fptr);
    code[fsize] = '\0';

    Lexer lexer;
    Tokens* tokens = tokenize(&lexer, code, 1);

    init_parser(tokens);
    ASTNode* program = parse_program();
    generate_llvm_ir(program, "program", "program.ll");
    free(code);
}

int main(int argc, char** argsv) {

    compile_source_file(argc, argsv);

#ifdef __unix__
    run_tests();
#endif
    return 0;
}