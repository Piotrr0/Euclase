#include "tests.h"
#include "lexer.h"
#include "parser.h"
#include "codegen_visitor.h"
#include <stdio.h>
#include <stdlib.h>

#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define RESET "\033[0m"

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

const char* test_conditions = 
    "namespace main {"
    "   int main() {"
    "       int a = 1;"
    "       int b = 2;"
    "       int c = 0;"
    ""
    "       if(a == b) {"
    "           c = 10;"
    "       }"
    "       else {"
    "           if(b == 2) {"
    "               c = 1;"
    "           }"
    "           else {"
    "               c = 2;"
    "           }"
    "       }"
    ""
    "       return c;"
    "   }"
    "}";

const char* test_less_greater =
    "namespace main {"
    "   int main() {"
    "       int c = 3;"
    "       if(3 > 2) {"
    "           return 2;"
    "       }"
    "       return c;"
    "   }"
    "}";

const char* test_negative_numbers = 
    "namespace main {"
    "   int main() {"
    "       int a = 100;"
    "       int b = -50;"
    "       return a - b;"
    "   }"
    "}";

const char* test_for_loops =
    "namespace main {"
    "   int main() {"
    "       int x = 0;"
    "       int s = 1;"
    "       int e = 3;"
    "       for(; s<e; s = s + 1) {"
    "           for(int j = 0; j<3; j = j + 1) {"
    "               x = x + 1;"
    "           }"
    "       }"
    "       return x;"
    "   }"
    "}";

const char* test_while_loops = 
    "namespace main {"
    "   int main() {"
    "       int a = 5;"
    "       int s = 0;"
    "       while (a > 1) {"
    "           a = a - 1;"
    "           s = s + 1;"
    "       }"
    "       return s;"
    "   }"
    "}";

const char* test_less_greater_equals =
    "namespace main {"
    "   int main() {"
    "       int c = 3;"
    "       if(3 >= 3) {"
    "           if(2 <= 2) {"
    "               return 3;"
    "           }"
    "           return 4;"
    "       }"
    "       return c;"
    "   }"
    "}";

const char* test_scopes = 
    "namespace main {"
    "   int main() {"
    "       int c = 0;"
    "       {"
    "           int a = 5;"
    "           int b = 3;"
    "           c = a + b;"
    "       }"
    "       return c;"
    "   }"
    "}";

const char* test_comments =
    "namespace main {\n"
    "   int main() {\n"
    "       int c = 2;\n"
    "       // c = 5;\n"
    "       /*\n"
    "       c = 7;\n"
    "       */\n"
    "       return c;\n"
    "   }\n"
    "}\n";

const char* test_compound_operators = 
    "namespace main {"
    "   int main() {"
    "       int a = 1;"
    "       int b = 2;"
    "       int c = 3;"
    "       int d = 4;"
    "       int e = 5;"
    "       a += b;"
    "       b -= c;"
    "       c *= d;"
    "       d /= e;"
    "       e %= a;"
    "       return a + b + c + d + e;"
    "   }"
    "}";

const char* test_struct = 
    "namespace main {"
    "   struct foo {"
    "       int a;"
    "       int b;"
    "   };"
    ""
    "   int main() {"
    "       return 40;"
    "   }"
    "}";

const char* test_string_literal = 
    "namespace main {"
    "   int main() {"
    "       char a = 'h';"
    "       char* b = \"hello world\";"

    "       return 5;"
    "   }"
    "}";

const char* test_inc_dec = 
    "namespace main {"
    "   int main() {"
    "       int a = 5;"
    "       ++a;"
    "       ++a;"
    "       --a;"
    "       a--;"
    "       a--;"
    "       a++;"
    "       return a;"
    "   }"
    "}";

const char* test_access_member = 
    "namespace main {"
    "   struct foo {"
    "       int a;"
    "       int b;"
    "   };"
    ""
    "   int main() {"
    "       foo my_foo;"
    "       my_foo.a = 10;"
    "       return my_foo.a;"
    "   }"
    "}";

const char* test_print =
    "namespace main {"
    "   int main() {"
    "       print(\"Hello World\");"
    "       return 1;"
    "   }"
    "}";

const char* test_array =
    "namespace main {"
    "   int main() {"
    "       int arr[5];"
    "       arr[1] = 2;"
    "       return arr[1];"
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
    tests[8] = (TestCase){"conditions", test_conditions, 1};
    tests[9] = (TestCase){"less_greater", test_less_greater, 2};
    tests[10] =(TestCase){"negative", test_negative_numbers, 150};
    tests[11] =(TestCase){"for_loop", test_for_loops, 6};
    tests[12] =(TestCase){"while_loop", test_while_loops, 4};
    tests[13] =(TestCase){"less_greater_equals", test_less_greater_equals, 3};
    tests[14] =(TestCase){"scopes", test_scopes, 8};
    tests[15] =(TestCase){"comments", test_comments, 2};
    tests[16] =(TestCase){"compound_operators", test_compound_operators, 16};
    tests[17] =(TestCase){"struct", test_struct, 40};
    tests[18] =(TestCase){"string_literal", test_string_literal, 5};
    tests[19] =(TestCase){"inc_dec", test_inc_dec, 5};
    tests[20] =(TestCase){"access_member", test_access_member, 10};
    // tests[21] =(TestCase){"print", test_print, 1};
    tests[22] =(TestCase){"array", test_array, 2};
}

int run_test(const char* test) 
{
    Lexer lexer;
    Tokens* tokens = tokenize(&lexer, test, 1);
    
    Parser parser;
    init_parser(&parser, tokens);

    ASTNode* root = parse_program(&parser);
    if(root == NULL) {
        printf("Test failed for: %s", test);
        return -1;
    }

    print_ast(root, 0);

    generate_llvm_ir_visitor(root, "main", "output.ll");
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
        if (result == tests[i].expected)
            printf("Test: %d (%s), %sPassed%s with result: %d\n", i, tests[i].name, KGRN, RESET, result);
        else
            printf("Test: %d (%s), %sFailed%s with result: %d (expected %d)\n", i, tests[i].name, KRED, RESET, result, tests[i].expected);
    }
}

int main()
{
#ifdef __unix__
    run_tests();
#endif
}