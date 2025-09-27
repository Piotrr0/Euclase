#include "codegen.h"
#include "parser.h"
#include <llvm-c/Types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CodegenContext ctx;

void init_codegen(CodegenContext* ctx, const char* module_name) {
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();
    
    ctx->context = LLVMContextCreate();
    ctx->module = LLVMModuleCreateWithNameInContext(module_name, ctx->context);
    ctx->builder = LLVMCreateBuilderInContext(ctx->context);
    ctx->var_count = 0;
}

void cleanup_codegen(CodegenContext* ctx) {
    if (ctx->builder) {
        LLVMDisposeBuilder(ctx->builder);
        ctx->builder = NULL;
    }
    if (ctx->module) {
        LLVMDisposeModule(ctx->module);
        ctx->module = NULL;
    }
    if (ctx->context) {
        LLVMContextDispose(ctx->context);
        ctx->context = NULL;
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

int add_variable(CodegenContext* ctx, const char* name, LLVMValueRef alloc)
{
    if(ctx->var_count > MAX_VARIABLES )
        return 0;

    ctx->variables[ctx->var_count].name = strdup(name);
    ctx->variables[ctx->var_count].alloc = alloc;

    ctx->var_count++;
    return 1;
}

LLVMValueRef get_variable(CodegenContext* ctx, const char* name)
{
    for(int i = 0; i<ctx->var_count; i++)
    {
        if (strcmp(ctx->variables[i].name, name) == 0) {
            return ctx->variables[i].alloc;
        }
    }
    return NULL;
}

LLVMValueRef codegen_expression(ASTNode *node) {
    if (node->type != AST_EXPRESSION) {
        printf("Codegen: Expected expression node\n");
        return NULL;
    }

    if (node->name) {
        LLVMValueRef var_alloca = get_variable(&ctx, node->name);
        if (!var_alloca) {
            printf("Codegen: Undefined variable '%s'\n", node->name);
            return NULL;
        }
        return LLVMBuildLoad2(ctx.builder, LLVMGetAllocatedType(var_alloca), var_alloca, "loadtmp");
    }

    switch (node->value_type) {
        case VAL_INT:
            return LLVMConstInt(LLVMInt32TypeInContext(ctx.context), node->value.int_val, 0);
        case VAL_FLOAT:
            return LLVMConstReal(LLVMFloatTypeInContext(ctx.context), node->value.float_val);
        case VAL_DOUBLE:
            return LLVMConstReal(LLVMDoubleTypeInContext(ctx.context), node->value.double_val);
        case VAL_NONE:
            break;
    }

    return NULL;
}

void codegen_statement(ASTNode *node) {
    switch (node->type)
    {
        case AST_RETURN:
            codegen_return(node);
            break;
        case AST_VAR_DECL:
            codegen_variable_declaration(node);
            break;
        case AST_ASSIGN:
            codegen_assign(node);
            break;;
        default:
            return;
    }
}

void codegen_return(ASTNode* node) {
    if(node->type != AST_RETURN)
        return;

    if(node->child_count > 0)
    {
        LLVMValueRef ret = codegen_expression(node->children[0]);
        if(ret)
            LLVMBuildRet(ctx.builder, ret);
        else
            LLVMBuildRetVoid(ctx.builder);
    } 
}

void codegen_variable_declaration(ASTNode* node) {
    if(node->type != AST_VAR_DECL)
        return ;

    LLVMTypeRef var_type = token_type_to_llvm_type(&ctx, node->decl_type);
    LLVMValueRef alloca = LLVMBuildAlloca(ctx.builder, var_type, node->name);

    add_variable(&ctx, node->name, alloca);

    if(node->child_count > 0)
    {
        LLVMValueRef init_val = codegen_expression(node->children[0]);
        if (init_val == NULL) 
        {
            return;
        }

        LLVMBuildStore(ctx.builder, init_val, alloca);
    }
}

void codegen_assign(ASTNode* node)
{
    if(node->type != AST_ASSIGN)
        return ;

    LLVMValueRef v = get_variable(&ctx, node->name);
    if(v == NULL)
        return;

    if(node->child_count > 0)
    {
        LLVMValueRef new_v = codegen_expression(node->children[0]);
        if(new_v)
        {
            LLVMBuildStore(ctx.builder, new_v, v);
        }
    }
}

void codegen_block(ASTNode *node) {
    if(node->type != AST_BLOCK)
    {
        printf("Codegen: Expected block node\n");
        return;
    }

    for(int i = 0; i<node->child_count; i++)
    {
        codegen_statement(node->children[i]);
    }
}

void codegen_function(ASTNode *node) {
    if(node->type != AST_FUNCTION)
    {
        printf("Codegen: Expected function node\n");
        return;
    }
    
    LLVMTypeRef ret_type = token_type_to_llvm_type(&ctx, node->decl_type);    
    LLVMTypeRef param_types[] = { };

    LLVMTypeRef func_type = LLVMFunctionType(ret_type, param_types, 0, 0);
    LLVMValueRef function = LLVMAddFunction(ctx.module, node->name, func_type);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(ctx.context, function, "entry");
    LLVMPositionBuilderAtEnd(ctx.builder, entry);

    if (node->child_count > 0) {
        for (int i = 0; i < node->child_count; i++) {
            codegen_block(node->children[i]);
        }
    }

    if (LLVMVerifyFunction(function, LLVMPrintMessageAction)) {
        printf("Codegen: Function verification failed for %s\n", node->name);
    }
}

void codegen_program(ASTNode* node)
{
    if(node->type != AST_PROGRAM)
    {
        printf("Codegen: Expected program node\n");
        return;
    }

    if (node->name) {
        char* module_id = malloc(strlen("namespace.") + strlen(node->name) + 1);
        sprintf(module_id, "namespace.%s", node->name);
        LLVMSetModuleIdentifier(ctx.module, module_id, strlen(module_id));
        free(module_id);
    }

    for (int i = 0; i < node->child_count; i++) {
        codegen_function(node->children[i]);
    }

    char* error = NULL;
    if (LLVMVerifyModule(ctx.module, LLVMPrintMessageAction, &error)) {
        printf("Module verification failed: %s\n", error);
        LLVMDisposeMessage(error);
    }
}

void generate_llvm_ir(ASTNode* ast, const char* module_name, const char* output_filename) {
    init_codegen(&ctx, module_name);
    codegen_program(ast);
    
    if (output_filename && strcmp(output_filename, "-") != 0) {
        char* error = NULL;
        if (LLVMPrintModuleToFile(ctx.module, output_filename, &error)) {
            printf("Error writing to file: %s\n", error);
            LLVMDisposeMessage(error);
        } 
    }
    cleanup_codegen(&ctx);
}