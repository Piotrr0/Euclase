#include "codegen_decl_visitor.h"
#include "codegen_visitor.h"
#include <llvm-c/Analysis.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void visit_var_decl_decl(CodegenVisitor* visitor, ASTNode* node)
{
    VarDeclNode var_decl = node->as.var_decl;
    TypeInfo var_decl_type = var_decl.type;

    LLVMTypeRef var_type = build_type_from_info(visitor->ctx, &var_decl_type);
    LLVMValueRef alloca = LLVMBuildAlloca(visitor->ctx->builder, var_type, var_decl.name);
    
    if (var_decl.initializer != NULL)
    {
        LLVMValueRef init_val = visit_expression(visitor, var_decl.initializer);
        if (init_val != NULL)
        {
            LLVMBuildStore(visitor->ctx->builder, init_val, alloca);
        }
    }
    
    add_variable_symbol(visitor->ctx->symbol_table, var_decl.name, var_decl.type, alloca, 0);
}

void visit_global_var_decl(CodegenVisitor* visitor, ASTNode* node)
{
    VarDeclNode var_decl = node->as.var_decl;
    TypeInfo var_decl_type = var_decl.type;

    LLVMTypeRef var_type = build_type_from_info(visitor->ctx, &var_decl_type);
    LLVMValueRef global_alloca = LLVMAddGlobal(visitor->ctx->module, var_type, var_decl.name);

    if (var_decl.initializer != NULL) {
        LLVMValueRef init_val = visit_expression(visitor, var_decl.initializer);
        if (init_val != NULL) {
            LLVMSetInitializer(global_alloca, init_val);
        }
    } 
    else {
        LLVMSetInitializer(global_alloca, LLVMConstNull(var_type));
    }

    add_variable_symbol(visitor->ctx->symbol_table, var_decl.name, var_decl.type, global_alloca, 1);
}

void visit_struct_decl_decl(CodegenVisitor* visitor, ASTNode* node) 
{
    StructDeclNode struct_decl = node->as.struct_decl;

    LLVMTypeRef structType = LLVMStructCreateNamed(LLVMGetGlobalContext(), struct_decl.type);
    LLVMTypeRef* field_types = malloc(sizeof(LLVMTypeRef) * struct_decl.member_count);

    char** member_names = malloc(sizeof(char*) * struct_decl.member_count);
    TypeInfo* member_types = malloc(sizeof(TypeInfo) * struct_decl.member_count);

    for (int i = 0; i < struct_decl.member_count; i++) {
        ASTNode* field = struct_decl.members[i];
        VarDeclNode field_decl = field->as.var_decl;
        LLVMTypeRef field_type = build_type_from_info(visitor->ctx, &field_decl.type);

        field_types[i] = field_type;
        member_names[i] = strdup(field_decl.name);
        member_types[i] = field_decl.type;
    }

    LLVMStructSetBody(structType, field_types, struct_decl.member_count, 0);
    free(field_types);

    add_struct_symbol(visitor->ctx->symbol_table, struct_decl.type, structType, struct_decl.member_count, member_names, member_types);
}

void collect_function_param_types(CodegenVisitor* visitor, FunctionNode func_node, LLVMTypeRef* param_types) {
    for (int i = 0; i < func_node.param_count; i++) {
        ASTNode* param = func_node.params[i];
        param_types[i] = build_type_from_info(visitor->ctx, &param->as.param.type);
    }
}

void setup_function_params(CodegenVisitor* visitor, LLVMValueRef function, FunctionNode func_node) {
    for (int i = 0; i < func_node.param_count; i++) {
        ASTNode* param = func_node.params[i];
        TypeInfo* param_info = &param->as.param.type;
        char* param_name = param->as.param.name;

        LLVMTypeRef param_type = build_type_from_info(visitor->ctx, param_info);
        LLVMValueRef alloca = LLVMBuildAlloca(visitor->ctx->builder, param_type, param_name);

        add_variable_symbol(visitor->ctx->symbol_table, param_name, *param_info, alloca, 0);

        LLVMValueRef param_val = LLVMGetParam(function, i);
        LLVMBuildStore(visitor->ctx->builder, param_val, alloca);
    }
}

void generate_function_body(CodegenVisitor* visitor, BlockNode body_node) {
    for (int i = 0; i < body_node.statement_count; i++) {
        visit_statement(visitor, body_node.statements[i]);
    }
}

void visit_function_decl(CodegenVisitor* visitor, ASTNode* node) {
    FunctionNode func_node = node->as.function;

    LLVMTypeRef* param_types = malloc(sizeof(LLVMTypeRef) * func_node.param_count);
    collect_function_param_types(visitor, func_node, param_types);

    LLVMTypeRef return_type = token_type_to_llvm_type(visitor->ctx, func_node.return_type.base_type);
    LLVMTypeRef func_type = LLVMFunctionType(return_type, param_types, func_node.param_count, 0);

    LLVMValueRef function = LLVMAddFunction(visitor->ctx->module, func_node.name, func_type);
    visitor->ctx->current_function = function;

    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(visitor->ctx->context, function, "entry");
    LLVMPositionBuilderAtEnd(visitor->ctx->builder, entry);

    push_scope(visitor->ctx->symbol_table);

    setup_function_params(visitor, function, func_node);
    generate_function_body(visitor, func_node.body->as.block);

    pop_scope(visitor->ctx->symbol_table);
}

void set_module_identifier(CodegenVisitor* visitor, const char* name)
{
    if (name == NULL)
        return;

    char* module_id = malloc(strlen("namespace.") + strlen(name) + 1);
    if (module_id == NULL)
        return;

    sprintf(module_id, "namespace.%s", name);
    LLVMSetModuleIdentifier(visitor->ctx->module, module_id, strlen(module_id));
    free(module_id);
}

void visit_program_decl(CodegenVisitor* visitor, ASTNode* node)
{
    ProgramNode program = node->as.program;

    set_module_identifier(visitor, program.name);

    for (int i = 0; i < program.struct_count; i++) {
        visit_struct_decl_decl(visitor, program.structs[i]);
    }

    for (int i = 0; i < program.global_count; i++) {
        visit_global_var_decl(visitor, program.globals[i]);
    }

    for (int i = 0; i < program.function_count; i++) {
        visit_function_decl(visitor, program.functions[i]);
    }
}