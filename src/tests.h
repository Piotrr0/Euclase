#ifndef TESTS_H
#define TESTS_H

extern const char* test_variables;
extern const char* test_pointers;
extern const char* test_pointer_function_param;
extern const char* test_casting;
extern const char* test_casting_pointer;
extern const char* test_nested_functions;

extern int expected[];

void run_tests();
int run_test(const char* test);
int run_llvm_and_get_exit_code(const char* filename);


#endif