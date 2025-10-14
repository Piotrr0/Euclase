#ifndef TESTS_H
#define TESTS_H

#define TESTS_BUFFER 20

typedef struct {
    const char* name;
    const char* source;
    int expected;
} TestCase;

extern TestCase tests[TESTS_BUFFER];

void init_tests();
void run_tests();
int run_test(const char* test);
int run_llvm_and_get_exit_code(const char* filename);


#endif