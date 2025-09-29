#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include <string.h>

int main() {
    const char* code_variables = 
        "namespace main {"
        "    int a = 1;"
        "    float b = 2.5f;"
        "    double c;"
        ""
        "    float get_pi() {"
        "       float pi = 3.14f;"
        "       pi = 2.71f;"
        "       return pi;"
        "    }"
        ""
        "    int assign(int to, int from) {"
        "       return x = y ;"
        "    }"

        "    int sum() {"
        "       int x = 10;"
        "       int y = 20;"
        "       return 30;"
        "    }"
        ""
        "    int main() {"
        "       a = 42;"
        "       b = 5.5f;"
        "       return 0;"
        "    }"
        "}";


    const char* code_pointers = 
        "namespace main {"
        "    int x = 10;"
        "    int y = 20;"
        "    int* ptr_x = &x;"
        "    int** ptr_ptr = &ptr_x;"
        "    int main() {"
        "                      "
        "        return *ptr_x;"
        "    }"
        "}";

    const char* code_casting = 
        "namespace main {"
        "   int main() {"
        "       float fvar = 3.14f;"
        "       int ivar = (int) fvar;"
        ""
        "       int a = 42;"
        "       float b = (float) a;"
        ""
        "       int* p = &ivar;"
        "       int* vp = (int*) p;"
        "       return ivar;"
        "    }"
        "}";

    init_lexer(&lexer, code_variables);

    ASTNode* root = parse_program();
    if(root == NULL)
        return 1;

    print_ast(root, 0);
    //generate_llvm_ir(root, "main", "output.ll");
    return 0;
}