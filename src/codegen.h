#ifndef CODEGEN_H
#define CODEGEN_H

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>

#include "parser.h"

#define MAX_VARIABLES 256

typedef struct Variable {
    const char* name;
    LLVMValueRef alloc;
} Variable;

typedef struct CodegenContext
{
    LLVMContextRef context;
    LLVMModuleRef module;
    LLVMBuilderRef builder;

    Variable variables[MAX_VARIABLES];
    int var_count;

} CodegenContext;

extern CodegenContext ctx;

void init_codegen(CodegenContext* ctx, const char* module_name);
void cleanup_codegen(CodegenContext* ctx);
LLVMTypeRef token_type_to_llvm_type(CodegenContext* ctx, TokenType type);

int add_variable(CodegenContext* ctx, const char* name, LLVMValueRef alloc);
LLVMValueRef get_variable(CodegenContext* ctx, const char* name);

void codegen_program(ASTNode* node);
void codegen_function(ASTNode* node);
void codegen_block(ASTNode* node);
void codegen_statement(ASTNode* node);
void codegen_return(ASTNode* node);
void codegen_variable_declaration(ASTNode* node);
void codegen_assign(ASTNode* node);

LLVMValueRef codegen_expression(ASTNode* node);


void generate_llvm_ir(ASTNode* ast, const char* module_name, const char* output_filename);

#endif