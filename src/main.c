#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include <string.h>

int main() {
    const char* code = 
        "namespace main {"
        "    int namespace_var = 1;"
        "    float get_pi() {"
        "       float pi = 3.14f;"
        "       pi = 2.71f;"
        "       return pi;"
        "    }          "
        "               "
        "    int main() {"
        "       namespace_var = 3;"
        "       return 0;"
        "   }"
        "}";

    init_lexer(&lexer, code);

    ASTNode* root = parse_program();
    if(root == NULL)
        return 1;

    print_ast(root, 0);
    generate_llvm_ir(root, "main", "output.ll");
    return 0;
}