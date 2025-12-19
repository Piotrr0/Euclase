#include "codegen_visitor.h"
#include "codegen_expr_visitor.h"
#include "codegen_stmt_visitor.h"
#include "codegen_decl_visitor.h"
#include "parser.h"
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void init_codegen_context(CodegenContext* ctx, const char* module_name)
{
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();

    ctx->context = LLVMContextCreate();
    ctx->module = LLVMModuleCreateWithNameInContext(module_name, ctx->context);
    ctx->builder = LLVMCreateBuilderInContext(ctx->context);
    ctx->symbol_table = init_symbol_table();
    ctx->current_function = NULL;
}

void cleanup_codegen_context(CodegenContext* ctx)
{
    if (ctx->builder != NULL) {
        LLVMDisposeBuilder(ctx->builder);
        ctx->builder = NULL;
    }

    if (ctx->module != NULL) {
        LLVMDisposeModule(ctx->module);
        ctx->module = NULL;
    }

    if (ctx->context != NULL) {
        LLVMContextDispose(ctx->context);
        ctx->context = NULL;
    }
}

CodegenVisitor* create_codegen_visitor(const char* module_name) {
    CodegenVisitor* visitor = (CodegenVisitor*)malloc(sizeof(CodegenVisitor));
    if (visitor == NULL) 
        return NULL;
    
    visitor->ctx = (CodegenContext*)malloc(sizeof(CodegenContext));
    if (visitor->ctx == NULL) {
        free(visitor);
        return NULL;
    }

    init_codegen_context(visitor->ctx, module_name);

    visitor->visit_int_literal = visit_int_literal_expr;
    visitor->visit_float_literal = visit_float_literal_expr;
    visitor->visit_double_literal = visit_double_literal_expr;
    visitor->visit_string_literal = visit_string_literal_expr;
    visitor->visit_char_literal = visit_char_literal_expr;
    visitor->visit_identifier = visit_identifier_expr;
    visitor->visit_binary_op = visit_binary_op_expr;
    visitor->visit_unary_op = visit_unary_op_expr;
    visitor->visit_func_call = visit_func_call_expr;
    visitor->visit_cast = visit_cast_expr;
    visitor->visit_member_access = visit_member_access_expr;
    visitor->visit_array_access = visit_array_access_expr;


    visitor->visit_return = visit_return_stmt;
    visitor->visit_if = visit_if_stmt;
    visitor->visit_while = visit_while_stmt;
    visitor->visit_for = visit_for_stmt;
    visitor->visit_block = visit_block_stmt;
    visitor->visit_assign = visit_assign_stmt;

    visitor->visit_var_decl = visit_var_decl_decl;
    visitor->visit_function = visit_function_decl;
    visitor->visit_struct_decl = visit_struct_decl_decl;
    visitor->visit_program = visit_program_decl;
    visitor->visit_print = visit_print_stmt;

    return visitor;
}

void destroy_codegen_visitor(CodegenVisitor* visitor) {
    if (visitor == NULL) 
        return;

    if (visitor->ctx != NULL) {
        cleanup_codegen_context(visitor->ctx);
        free(visitor->ctx);
    }

    free(visitor);
}

LLVMValueRef visit_expression(CodegenVisitor* visitor, ASTNode* node) {
    if (node == NULL || visitor == NULL) 
        return NULL;

    switch (node->type) {
        case AST_INT_LITERAL:       return visitor->visit_int_literal(visitor, node);
        case AST_FLOAT_LITERAL:     return visitor->visit_float_literal(visitor, node);
        case AST_DOUBLE_LITERAL:    return visitor->visit_double_literal(visitor, node);
        case AST_STRING_LITERAL:    return visitor->visit_string_literal(visitor, node);
        case AST_CHAR_LITERAL:      return visitor->visit_char_literal(visitor, node);
        case AST_IDENTIFIER:        return visitor->visit_identifier(visitor, node);
        case AST_FUNC_CALL:         return visitor->visit_func_call(visitor, node);
        case AST_CAST:              return visitor->visit_cast(visitor, node);
        case AST_UNARY_OP:          return visitor->visit_unary_op(visitor, node);
        case AST_BINARY_OP:         return visitor->visit_binary_op(visitor, node);
        case AST_MEMBER_ACCESS:     return visitor->visit_member_access(visitor, node);
        case AST_ARRAY_ACCESS:      return visitor->visit_array_access(visitor, node);
        default:                    printf("Unhandled expression type: %d\n", node->type); return NULL;
    }
}

void visit_statement(CodegenVisitor* visitor, ASTNode* node) {
    if (node== NULL || visitor == NULL)
        return;

    switch (node->type) {
        case AST_RETURN:    visitor->visit_return(visitor, node); break;
        case AST_VAR_DECL:  visitor->visit_var_decl(visitor, node); break;
        case AST_ASSIGN:    visitor->visit_assign(visitor, node); break;
        case AST_IF:        visitor->visit_if(visitor, node); break;
        case AST_FOR:       visitor->visit_for(visitor, node); break;
        case AST_WHILE:     visitor->visit_while(visitor, node); break;
        case AST_BLOCK:     visitor->visit_block(visitor, node); break;
        case AST_PRINT:     visitor->visit_print(visitor, node); break;
        default:            visit_expression(visitor, node); break;
    }
}

void visit_declaration(CodegenVisitor* visitor, ASTNode* node) {
    if (node == NULL || visitor == NULL) return;

    switch (node->type) {
        case AST_FUNCTION: visitor->visit_function(visitor, node); break;
        case AST_VAR_DECL: visitor->visit_var_decl(visitor, node); break;
        case AST_STRUCT_DECL: visitor->visit_struct_decl(visitor, node); break;
        case AST_PROGRAM: visitor->visit_program(visitor, node); break;
        default: printf("Unhandled declaration type: %d\n", node->type); break;
    }
}

LLVMTypeRef token_type_to_llvm_type(CodegenContext* ctx, TokenType type) {
    switch(type) {
        case TOK_INT:     return LLVMInt32TypeInContext(ctx->context);
        case TOK_UINT:    return LLVMInt32TypeInContext(ctx->context);
        case TOK_FLOAT:   return LLVMFloatTypeInContext(ctx->context);
        case TOK_UFLOAT:  return LLVMFloatTypeInContext(ctx->context);
        case TOK_DOUBLE:  return LLVMDoubleTypeInContext(ctx->context);
        case TOK_UDOUBLE: return LLVMDoubleTypeInContext(ctx->context);
        case TOK_CHAR:    return LLVMInt8TypeInContext(ctx->context);
        case TOK_UCHAR:   return LLVMInt8TypeInContext(ctx->context);
        case TOK_VOID:    return LLVMVoidTypeInContext(ctx->context);
        default:          return LLVMInt32TypeInContext(ctx->context);
    }
}

LLVMTypeRef build_type_from_info(CodegenContext* ctx, TypeInfo* type_info)
{
    LLVMTypeRef base_type;
    if (type_info->base_type == TOK_IDENTIFIER)
    {
        SymbolEntry* struct_entry = lookup_symbol(ctx->symbol_table, type_info->type);
        if (struct_entry == NULL || struct_entry->symbol_data.kind != SYMBOL_STRUCT) {
            return NULL;
        }
        base_type = struct_entry->symbol_data.as.struct_def.struct_type;
    } 
    else {
        base_type = token_type_to_llvm_type(ctx, type_info->base_type);
    }

    for (int i = 0; i < type_info->pointer_level; i++) {
        base_type = LLVMPointerType(base_type, 0);
    }

    if (type_info->is_array) {
        for (int i = type_info->array_dim_count - 1; i >= 0; i--) {
            base_type = LLVMArrayType(base_type, type_info->array_sizes[i]);
        }
    }

    return base_type;
}

LLVMTypeRef get_element_type_from_info(CodegenVisitor* visitor, TypeInfo type_info) {
    TypeInfo elem_info = type_info;

    if (elem_info.is_array)
    {
        if (elem_info.array_dim_count > 1) {
            for (int i = 0; i < elem_info.array_dim_count - 1; i++) {
                elem_info.array_sizes[i] = elem_info.array_sizes[i + 1];
            }
            elem_info.array_dim_count--;
        }
        else {
            elem_info.is_array = 0;
            elem_info.array_dim_count = 0;
        }
    } 
    else if (elem_info.pointer_level > 0) {
        elem_info.pointer_level--;
    }
    return build_type_from_info(visitor->ctx, &elem_info);
}

void generate_llvm_ir_visitor(ASTNode* ast, const char* module_name, const char* output_filename)
{
    if (ast == NULL)
        return;

    CodegenVisitor* visitor = create_codegen_visitor(module_name);
    if (visitor == NULL) {
        printf("Failed to create codegen visitor\n");
        return;
    }

    visit_declaration(visitor, ast);

    char* error = NULL;
    if (LLVMVerifyModule(visitor->ctx->module, LLVMPrintMessageAction, &error)) {
        printf("Module verification failed: %s\n", error);
        LLVMDisposeMessage(error);
    }

    if (output_filename != NULL) {
        if (LLVMPrintModuleToFile(visitor->ctx->module, output_filename, &error)) {
            printf("Error writing to file: %s\n", error);
            LLVMDisposeMessage(error);
        }
    }

    destroy_codegen_visitor(visitor);
}

LLVMValueRef get_printf_func(CodegenContext* ctx) {
    LLVMValueRef printf_func = LLVMGetNamedFunction(ctx->module, "printf");

    if (printf_func != NULL)
        return printf_func;

    LLVMTypeRef args[] = { LLVMPointerType(LLVMInt8TypeInContext(ctx->context), 0) };    
    LLVMTypeRef func_type = LLVMFunctionType(LLVMInt32TypeInContext(ctx->context), args, 1, 1);
    
    printf_func = LLVMAddFunction(ctx->module, "printf", func_type);
    return printf_func;
}