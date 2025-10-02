#ifndef TESTS_H
#define TESTS_H

#define TESTS_BUFFER 10

typedef struct {
    const char* name;
    const char* source;
    int expected;
} TestCase;

extern const char* test_variables;
extern const char* test_pointers;
extern const char* test_pointer_function_param;
extern const char* test_casting;
extern const char* test_casting_pointer;
extern const char* test_nested_functions;
extern const char* test_arithmetic;

extern TestCase* tests[TESTS_BUFFER];

void init_tests();
void run_tests();
int run_test(const char* test);
int run_llvm_and_get_exit_code(const char* filename);


#endif