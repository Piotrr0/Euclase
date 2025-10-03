#include "tests.h"
#include "parser.h"
#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>

const char* test_variables = 
    "namespace main {"
    "    int a = 1;"
    "    float b = 2.5f;"
    "    double c = 0.0d;"
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

const char* test_arithmetic = 
    "namespace main {"
    "   int add(int x, int y) {"
    "       return x + y + 5;"
    "   }"
    ""
    "   int main() {"
    "       int sum = add(3,10);"
    "       return sum;"
    "   }"
    "}";

const char* test_equality = 
    "namespace main {"
    "   int main() {"
    "       int a = 10;"
    "       int b = 15;"
    "       int c = 20;"
    "       int r1 = (a == b);"
    "       int r2 = (a != c);"
    "       int r3 = (a == c);"
    ""
    "       float x = 3.14f;"
    "       float y = 3.14f;"
    "       float z = 2.71f;"
    "       int r4 = (x == y);"
    "       int r5 = (x != z);"
    "       int r6 = (x != y);"
    ""
    "       double p = 1.23d;"
    "       double q = 1.23d;"
    "       double r = 4.56d;"
    "       int r7 = (p == q);"
    "       int r8 = (p != r);"
    "       int r9 = (p == r);"
    ""
    "       return r1 + r2 + r3 + r4 + r5 + r6 + r7 + r8 + r9;"
    "   }"
    "}";


TestCase tests[TESTS_BUFFER];

void init_tests() {
    tests[0] = (TestCase){"variables", test_variables, 10};
    tests[1] = (TestCase){"pointers", test_pointers, 15};
    tests[2] = (TestCase){"pointer_function_param", test_pointer_function_param, 10};
    tests[3] = (TestCase){"casting", test_casting, 3};
    tests[4] = (TestCase){"casting_pointer", test_casting_pointer, 5};
    tests[5] = (TestCase){"nested_functions", test_nested_functions, 4};
    tests[6] = (TestCase){"arithmetic", test_arithmetic, 18};
    tests[7] = (TestCase){"equality", test_equality, 5};
}

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
    init_tests();

    int results[TESTS_BUFFER];
    for(int i = 0; i < TESTS_BUFFER; i++) {
        if(tests[i].name == NULL)
            continue;

        results[i] = run_test(tests[i].source);
    }

    for(int i = 0; i < TESTS_BUFFER; i++) {
        if(tests[i].name == NULL)
            continue;

        int result = results[i];
        if(result == tests[i].expected)
            printf("Test: %d (%s), Passed with result: %d\n", i, tests[i].name, result);
        else
            printf("Test: %d (%s), Failed with result: %d (expected %d)\n", i, tests[i].name, result, tests[i].expected);
    }
}