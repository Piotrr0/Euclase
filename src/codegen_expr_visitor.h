#ifndef CODEGEN_EXPR_VISITOR_H
#define CODEGEN_EXPR_VISITOR_H

#include "codegen_visitor.h"

LLVMValueRef visit_int_literal_expr(CodegenVisitor* visitor, ASTNode* node);
LLVMValueRef visit_float_literal_expr(CodegenVisitor* visitor, ASTNode* node);
LLVMValueRef visit_double_literal_expr(CodegenVisitor* visitor, ASTNode* node);
LLVMValueRef visit_string_literal_expr(CodegenVisitor* visitor, ASTNode* node);
LLVMValueRef visit_char_literal_expr(CodegenVisitor* visitor, ASTNode* node);

LLVMValueRef visit_identifier_expr(CodegenVisitor* visitor, ASTNode* node);
LLVMValueRef visit_func_call_expr(CodegenVisitor* visitor, ASTNode* node);
LLVMValueRef visit_cast_expr(CodegenVisitor* visitor, ASTNode* node);
LLVMValueRef visit_binary_op_expr(CodegenVisitor* visitor, ASTNode* node);
LLVMValueRef visit_unary_op_expr(CodegenVisitor* visitor, ASTNode* node);
LLVMValueRef visit_member_access_expr(CodegenVisitor* visitor, ASTNode* node);

int are_types_compatible(LLVMTypeRef form_type, LLVMTypeRef to_type);
LLVMValueRef generate_cast_instruction(CodegenVisitor* visitor, LLVMValueRef value, LLVMTypeRef from_type, LLVMTypeRef to_type, const char* name);

#endif