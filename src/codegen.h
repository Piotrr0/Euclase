#ifndef CODEGEN_H
#define CODEGEN_H

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Types.h>

#include "lookup_table.h"
#include "parser.h"

typedef struct CodegenContext
{
    LLVMContextRef context;
    LLVMModuleRef module;
    LLVMBuilderRef builder;

} CodegenContext;

extern CodegenContext ctx;
extern LLVMValueRef current_function;
extern SymbolTable* st;

void init_codegen(CodegenContext* ctx, const char* module_name);
void cleanup_codegen(CodegenContext* ctx);
LLVMTypeRef token_type_to_llvm_type(CodegenContext* ctx, TokenType type);

void codegen_struct_declaration(ASTNode* node);

void codegen_condition(ASTNode* node);
void codegen_then_block(ASTNode* node_block, LLVMBasicBlockRef mergeBB);
void codegen_else_block(ASTNode* node_else, LLVMBasicBlockRef mergeBB);

LLVMValueRef codegen_equality(ASTNode* node);
LLVMValueRef codegen_compare(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type, int is_not_equal);
LLVMValueRef codegen_less(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type);
LLVMValueRef codegen_greater(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type);
LLVMValueRef codegen_less_equal(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type);
LLVMValueRef codegen_greater_equal(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type);

LLVMValueRef codegen_constant(ASTNode* node);
LLVMValueRef codegen_string_literal(ASTNode* node);
LLVMValueRef codegen_char_literal(ASTNode* node);
LLVMValueRef codegen_variable_load(const char* name);
LLVMValueRef codegen_cast(ASTNode* node);
LLVMValueRef codegen_function_call(ASTNode* node);

void codegen_while_loop(ASTNode* node);
void codegen_for_loop(ASTNode* node);

int does_type_kind_match(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind* out_kind);
LLVMValueRef codegen_arithmetic_op(ASTNode* node);
LLVMValueRef codegen_addition(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type);
LLVMValueRef codegen_subtraction(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type);
LLVMValueRef codegen_multiplication(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type);
LLVMValueRef codegen_division(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type);
LLVMValueRef codegen_modulo(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type);

int are_types_compatible(LLVMTypeRef form_type, LLVMTypeRef to_type);
LLVMValueRef generate_cast_instruction(LLVMValueRef value, LLVMTypeRef from_type, LLVMTypeRef to_type, const char* name);

void codegen_module_id(const char* name);
void codegen_program(ASTNode* node);
void codegen_function(ASTNode* node);
void codegen_block(ASTNode* node);
void codegen_statement(ASTNode* node);
void codegen_return(ASTNode* node);
void codegen_variable_declaration(ASTNode* node);
void codegen_global_variable_declaration(ASTNode* node);
void codegen_assign(ASTNode* node);

LLVMValueRef codegen_unary_minus(ASTNode* node);
LLVMValueRef codegen_expression(ASTNode* node);
LLVMValueRef codegen_dereference(ASTNode* node);
LLVMValueRef codegen_address_of(ASTNode* node);

void generate_llvm_ir(ASTNode* ast, const char* module_name, const char* output_filename);

#endif