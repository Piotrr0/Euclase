#ifndef CODEGEN_H
#define CODEGEN_H

#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Types.h>

#include "ast_layout.h"
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

LLVMTypeRef build_param_type(TypeInfo* type_info);

void init_codegen(CodegenContext* ctx, const char* module_name);
void cleanup_codegen(CodegenContext* ctx);
LLVMTypeRef token_type_to_llvm_type(CodegenContext* ctx, TokenType type);

void codegen_struct_declaration(StructDeclNode struct_decl_node);
LLVMValueRef codegen_member_access(ASTNode* node);

void codegen_if(IfNode if_node);
void codegen_then_block(ASTNode* node_block, LLVMBasicBlockRef mergeBB);
void codegen_else_block(ASTNode* node_else, LLVMBasicBlockRef mergeBB);

LLVMValueRef codegen_equality(ASTNode* node);
LLVMValueRef codegen_compare(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type, int is_not_equal);
LLVMValueRef codegen_less(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type);
LLVMValueRef codegen_greater(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type);
LLVMValueRef codegen_less_equal(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type);
LLVMValueRef codegen_greater_equal(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type);

LLVMValueRef codegen_constant(ASTNode* node);

LLVMValueRef codegen_string_literal(StringLiteralNode string_node);
LLVMValueRef codegen_char_literal(CharLiteralNode char_node);
LLVMValueRef codegen_float_literal(FloatLiteralNode float_node);
LLVMValueRef codegen_int_literal(IntLiteralNode int_node);
LLVMValueRef codegen_double_literal(DoubleLiteralNode double_node);

LLVMValueRef codegen_variable_load(const char* name);
LLVMValueRef codegen_cast(CastNode cast_node);
LLVMValueRef codegen_function_call(FuncCallNode function_call_node);

void codegen_while_loop(WhileNode while_node);
void codegen_for_loop(ForNode for_node);

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
void codegen_program(ProgramNode program_node);

void codegen_function(FunctionNode func_node);
void setup_function_params(LLVMValueRef function, FunctionNode func_node);
void collect_function_param_types(FunctionNode func_node, LLVMTypeRef* param_types);
void generate_function_body(BlockNode body_node);

LLVMValueRef codegen_expression(ASTNode* node);
void codegen_block(BlockNode block_node);
void codegen_statement(ASTNode* node);
void codegen_return(ReturnNode return_node);
void codegen_variable_declaration(VarDeclNode var_decl_node);
void codegen_global_variable_declaration(VarDeclNode var_decl_node);
void codegen_assign(AssignNode assign_node);

LLVMValueRef codegen_unary_op(UnaryOpNode node);
LLVMValueRef codegen_negation(UnaryOpNode node);
LLVMValueRef codegen_dereference(UnaryOpNode node);
LLVMValueRef codegen_address_of(UnaryOpNode node);
LLVMValueRef codegen_pre_inc(UnaryOpNode node);
LLVMValueRef codegen_pre_dec(UnaryOpNode node);
LLVMValueRef codegen_post_inc(UnaryOpNode node);
LLVMValueRef codegen_post_dec(UnaryOpNode node);


void generate_llvm_ir(ASTNode* ast, const char* module_name, const char* output_filename);

#endif