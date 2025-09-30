#include "tests.h"
#include "parser.h"
#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>

const char* test_variables = 
    "namespace main {"
    "    int a = 1;"
    "    float b = 2.5f;"
    "    double c = 0d;"
    ""
    "    int main() {"
    "        a = 10;"
    "        b = 3.14f;"
    "        c = 2.718d;"
    "        return a;"
    "    }"
    "}";

const char* test_pointers =
    "namespace main {"
    "    int a = 5;"
    "    int* p = &a;"
    "    int** pp = &p;"
    ""
    "    int main() {"
    "        *p = 10;"
    "        **pp = 15;"
    "        return a;"
    "    }"
    "}";

const char* test_pointer_function_param =
    "namespace main {"
    "    int modify(int* x) {"
    "        return 10;"
    "    }"
    "    int main() {"
    "        int a = 7;"
    "        return modify(&a);"
    "    }"
    "}";

const char* test_casting =
    "namespace main {"
    "    int main() {"
    "        float f = 3.99f;"
    "        int i = (int) f;"
    "        return i;"
    "    }"
    "}";

const char* test_casting_pointer =
    "namespace main {"
    "    int main() {"
    "        int a = 5;"
    "        void* vp = &a;"
    "        int* ip = (int*) vp;"
    "        return *ip;"
    "    }"
    "}";

const char* test_nested_functions =
    "namespace main {"
    "    int inner(int x) {"
    "        return x;"
    "    }"
    "    int outer(int y) {"
    "        return inner(y);"
    "    }"
    "    int main() {"
    "        return outer(4);"
    "    }"
    "}";

int expected[] = {10, 15, 10, 3, 5, 4};
const int tests_count = sizeof(expected) / sizeof(expected[0]);

int run_test(const char* test) 
{
    init_lexer(&lexer, test);

    ASTNode* root = parse_program();
    if(root == NULL) {
        printf("Test failed for: %s", test);
        return -1;
    }

    print_ast(root, 0);

    generate_llvm_ir(root, "main", "output.ll");
    free_ast(root);

    return run_llvm_and_get_exit_code("output.ll");
}

int run_llvm_and_get_exit_code(const char* filename) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "lli %s", filename);
    int ret = system(cmd);
    return WEXITSTATUS(ret);
}

void run_tests() {
    
    const char* tests[6] = {
        test_variables,
        test_pointers,
        test_pointer_function_param,
        test_casting,
        test_casting_pointer,
        test_nested_functions
    };

    int results[6] = {};

    for(int i = 0; i<tests_count; i++)
    {
        results[i] = run_test(tests[i]);
    }

    for(int i = 0; i<tests_count; i++)
    {
        int result = results[i];
        if(result == expected[i])
            printf("Test: %d, Passed with result: %d\n", i, result);
        else
            printf("Test: %d, Failed with result: %d\n", i, result);
    }
}