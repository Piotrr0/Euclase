#include "codegen_visitor.h"
#include "lexer.h"
#include "parser.h"
#include "tests.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int has_ecl_extension(const char* filename) {
    if (filename == NULL)
        return 0;
    
    size_t len = strlen(filename);
    size_t len_ext = strlen(".ecl");
    if (len < len_ext + 1) 
        return 0;
    
    return strcmp(filename + len - len_ext, ".ecl") == 0;
}

void print_usage() {
    fprintf(stderr, "Usage: <source_file.ecl>\n");
}

int check_argument_count(int argc, int desired_count) {
    if (argc != desired_count) {
        fprintf(stderr, "Error: Invalid number of arguments.\n");
        print_usage();
        return 0;
    }
    return 1;
}

char* get_module_name(const char* filename)
{
    if (filename == NULL)
        return NULL;
    
    const char* basename = filename;
    const char* last_backslash = strrchr(filename, '\\');
    
    if (last_backslash != NULL)
        basename = last_backslash + 1;
    
    size_t name_len;
    const char* last_dot = strrchr(basename, '.');    
    if (last_dot != NULL)
        name_len = last_dot - basename;
    else
        name_len = strlen(basename);
    
    char* module_name = malloc(name_len + 1);
    if (module_name == NULL)
        return NULL;
    
    strncpy(module_name, basename, name_len);
    module_name[name_len] = '\0';
    
    return module_name;
}

char* get_output_filename(const char* module_name)
{
    if (module_name == NULL)
        return NULL;
    
    size_t len = strlen(module_name);
    char* output = malloc(len + strlen(".ll") + 1);
    if (output == NULL)
        return NULL;
    
    sprintf(output, "%s.ll", module_name);
    return output;
}

char* get_source_from_file(int argc, char** argv) {
    if(!check_argument_count(argc, 2))
        return NULL;

    const char* filename = argv[1];    
    if (!has_ecl_extension(filename)) {
        print_usage();
        return NULL;
    }
    
    FILE* fptr = fopen(filename, "r");
    if (fptr == NULL) {
        return NULL;
    }
    
    if (fseek(fptr, 0, SEEK_END) != 0) {
        fclose(fptr);
        return NULL;
    }
    
    long fsize = ftell(fptr);
    if (fsize <= 0) {
        fclose(fptr);
        return NULL;
    }
    
    fseek(fptr, 0, SEEK_SET);
    
    char* code = malloc(fsize + 1);
    if (code == NULL) {
        fclose(fptr);
        return NULL;
    }
    
    size_t bytes_read = fread(code, 1, fsize, fptr);
    fclose(fptr);
    
    if (bytes_read != fsize) {
        free(code);
        return NULL;
    }
    
    code[fsize] = '\0';
    return code;
}

void compile_source_file(int argc, char** argv)
{
    char* code = get_source_from_file(argc, argv);
    if (code == NULL)
        return;
    
    char* module_name = get_module_name(argv[1]);
    if (module_name == NULL) {
        free(code);
        return;
    }
    
    char* output_filename = get_output_filename(module_name);
    if (output_filename == NULL) {
        free(module_name);
        free(code);
        return;
    }
    
    Lexer lexer;
    Tokens* tokens = tokenize(&lexer, code, 1);
    cleanup_lexer(&lexer);
    init_parser(tokens);
    ASTNode* program = parse_program();

    generate_llvm_ir_visitor(program, module_name, output_filename);
    
    free(output_filename);
    free(module_name);
    free(code);
}

int main(int argc, char** argv) {
    compile_source_file(argc, argv);

#ifdef __unix__
    run_tests();
#endif
    return 0;
}