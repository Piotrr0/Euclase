#ifndef CODEGEN_DECL_VISITOR_H
#define CODEGEN_DECL_VISITOR_H

#include "codegen_visitor.h"

void visit_var_decl_decl(CodegenVisitor* visitor, ASTNode* node);
void visit_function_decl(CodegenVisitor* visitor, ASTNode* node);
void visit_struct_decl_decl(CodegenVisitor* visitor, ASTNode* node);
void visit_program_decl(CodegenVisitor* visitor, ASTNode* node);

void visit_global_var_decl(CodegenVisitor* visitor, ASTNode* node);

void set_module_identifier(CodegenVisitor* visitor, const char* name);
void collect_function_param_types(CodegenVisitor* visitor, FunctionNode func_node, LLVMTypeRef* param_types);
void setup_function_params(CodegenVisitor* visitor, LLVMValueRef function, FunctionNode func_node);
void generate_function_body(CodegenVisitor* visitor, BlockNode body_node);

#endif