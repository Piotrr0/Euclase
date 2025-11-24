#ifndef CODEGEN_VISITOR_H
#define CODEGEN_VISITOR_H

#include "ast_layout.h"
#include "lookup_table.h"
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>

typedef struct CodegenVisitor CodegenVisitor;
typedef struct CodegenContext CodegenContext;

typedef struct CodegenContext {
    LLVMContextRef context;
    LLVMModuleRef module;
    LLVMBuilderRef builder;

    SymbolTable* symbol_table;
    LLVMValueRef current_function;
} CodegenContext;

typedef LLVMValueRef (*ExprVisitorFn)(CodegenVisitor*, ASTNode*);
typedef void (*StmtVisitorFn)(CodegenVisitor*, ASTNode*);
typedef void (*DeclVisitorFn)(CodegenVisitor*, ASTNode*);

typedef struct CodegenVisitor {
    CodegenContext* ctx;

    ExprVisitorFn visit_int_literal;
    ExprVisitorFn visit_float_literal;
    ExprVisitorFn visit_double_literal;
    ExprVisitorFn visit_string_literal;
    ExprVisitorFn visit_char_literal;
    ExprVisitorFn visit_identifier;
    ExprVisitorFn visit_binary_op;
    ExprVisitorFn visit_unary_op;
    ExprVisitorFn visit_func_call;
    ExprVisitorFn visit_cast;
    ExprVisitorFn visit_member_access;

    StmtVisitorFn visit_return;
    StmtVisitorFn visit_if;
    StmtVisitorFn visit_while;
    StmtVisitorFn visit_for;
    StmtVisitorFn visit_block;
    StmtVisitorFn visit_assign;

    DeclVisitorFn visit_var_decl;
    DeclVisitorFn visit_function;
    DeclVisitorFn visit_struct_decl;
    DeclVisitorFn visit_program;
} CodegenVisitor;

void init_codegen_context(CodegenContext* ctx, const char* module_name);
void cleanup_codegen_context(CodegenContext* ctx);

CodegenVisitor* create_codegen_visitor(const char* module_name);
void destroy_codegen_visitor(CodegenVisitor* visitor);

LLVMValueRef visit_expression(CodegenVisitor* visitor, ASTNode* node);
void visit_statement(CodegenVisitor* visitor, ASTNode* node);
void visit_declaration(CodegenVisitor* visitor, ASTNode* node);

LLVMTypeRef token_type_to_llvm_type(CodegenContext* ctx, TokenType type);
LLVMTypeRef build_type_from_info(CodegenContext* ctx, TypeInfo* type_info);

void generate_llvm_ir_visitor(ASTNode* ast, const char* module_name, const char* output_filename);
LLVMValueRef generate_cast_instruction(CodegenVisitor* visitor, LLVMValueRef value, LLVMTypeRef from_type, LLVMTypeRef to_type, const char* name);

#endif