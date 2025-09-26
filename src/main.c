#include "lexer.h"
#include "parser.h"

int main() {
    const char* code = 
        "namespace main {"
        "   float getPi() {"
        "       float pi = 3.14f;"
        "       return 3.14f;"
        "   }"
        ""
        "   int main() {"
        "       return 0;"
        "   }"
        ""
        "   double getE() {"
        "       return 2.71d;"
        "   }"
        "}";

    init_lexer(code);

    ASTNode* root = parse_program();
    print_ast(root, 0);

    return 0;
}