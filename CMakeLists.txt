cmake_minimum_required(VERSION 3.16)
project(Euclase C)

find_program(LLVM_CONFIG llvm-config)

if(NOT LLVM_CONFIG)
    message(FATAL_ERROR "llvm-config not found. Make sure LLVM is installed and in PATH.")
endif()

execute_process(
        COMMAND ${LLVM_CONFIG} --cflags
        OUTPUT_VARIABLE LLVM_CFLAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
        COMMAND ${LLVM_CONFIG} --libs core
        OUTPUT_VARIABLE LLVM_LIBS
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
        COMMAND ${LLVM_CONFIG} --system-libs
        OUTPUT_VARIABLE LLVM_SYSTEM_LIBS
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

file(GLOB SRC "src/*.c")
list(REMOVE_ITEM SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c")
list(REMOVE_ITEM SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/tests.c")

add_executable(${PROJECT_NAME} src/main.c ${SRC})
add_executable(EuclaseTests src/tests.c ${SRC})


target_compile_options(${PROJECT_NAME} PRIVATE ${LLVM_CFLAGS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LLVM_LIBS} ${LLVM_SYSTEM_LIBS})

target_compile_options(EuclaseTests PRIVATE ${LLVM_CFLAGS})
target_link_libraries(EuclaseTests PRIVATE ${LLVM_LIBS} ${LLVM_SYSTEM_LIBS})