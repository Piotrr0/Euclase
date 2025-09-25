#include "lexer.h"
#include "parser.h"

int main() {
    const char* code = 
        "namespace main {"
        "   int main() {"
        "       return 0;"
        "   }"
        "}";

    init_lexer(code);
    advance();

    ASTNode* root = parse_program();
    print_ast(root, 0);

    return 0;
}