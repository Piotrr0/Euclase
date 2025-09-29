#include "codegen.h"
#include "parser.h"
#include <llvm-c/Core.h>
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

int add_variable(CodegenContext* ctx, const char* name, LLVMValueRef alloc, int is_global, TokenType base_type, int pointer_level)
{
    if(ctx->var_count > MAX_VARIABLES )
        return 0;

    const int var_count = ctx->var_count;

    ctx->variables[var_count].name = strdup(name);
    ctx->variables[var_count].alloc = alloc;
    ctx->variables[var_count].is_global = is_global;
    ctx->variables[var_count].base_type = base_type;
    ctx->variables[var_count].pointer_level = pointer_level;

    ctx->var_count++;
    return 1;
}

Variable* get_variable(CodegenContext* ctx, const char* name)
{
    for(int i = 0; i<ctx->var_count; i++)
    {
        if (strcmp(ctx->variables[i].name, name) == 0) {
            return &ctx->variables[i];
        }
    }
    return NULL;
}


LLVMValueRef codegen_variable_load(const char* name) {
    if (name == NULL)
        return NULL; 
    
    Variable* var = get_variable(&ctx, name);
    if (var == NULL || var->alloc == NULL) {
        printf("Codegen: Undefined variable '%s'\n", name);
        return NULL;
    }

    if (var->is_global) 
        return LLVMBuildLoad2(ctx.builder, LLVMGlobalGetValueType(var->alloc), var->alloc, "global_load");
    else
        return LLVMBuildLoad2(ctx.builder, LLVMGetAllocatedType(var->alloc), var->alloc, "local_load");
}

LLVMValueRef codegen_constant(ASTNode* node)
{
    switch (node->value.type) {
        case VAL_INT:       return LLVMConstInt(LLVMInt32TypeInContext(ctx.context), node->value.int_val, 0); break;
        case VAL_FLOAT:     return LLVMConstReal(LLVMFloatTypeInContext(ctx.context), node->value.float_val); break;
        case VAL_DOUBLE:    return LLVMConstReal(LLVMDoubleTypeInContext(ctx.context), node->value.double_val); break;
        case VAL_NONE:      break;
    }
    return NULL;
}

LLVMValueRef codegen_cast(ASTNode* node) 
{
    if(node->type != AST_CAST || node->child_count == 0)
        return NULL;

    LLVMValueRef value = codegen_expression(node->children[0]);
    if(value == NULL)
    {
        printf("Codegen: Failed to generate expression for cast\n");
        return NULL;
    }

    LLVMTypeRef from_type = LLVMTypeOf(value);
    LLVMTypeRef to_type = token_type_to_llvm_type(&ctx, node->type_info.base_type);

    for (int i = 0; i < node->type_info.pointer_level; i++) {
        to_type = LLVMPointerType(to_type, 0);
    }

    if (!are_types_compatible(from_type, to_type)) {
        printf("Codegen: Incompatible cast\n");
        return NULL;
    }
    
    return generate_cast_instruction(value, from_type, to_type, "cast_result");
}

int are_types_compatible(LLVMTypeRef form_type, LLVMTypeRef to_type)
{
    LLVMTypeKind from_kind = LLVMGetTypeKind(form_type);
    LLVMTypeKind to_kind = LLVMGetTypeKind(to_type);

    if(from_kind == to_kind)
        return 1;

    if (from_kind == LLVMPointerTypeKind && to_kind == LLVMPointerTypeKind)
        return 1;

    if ((from_kind == LLVMFloatTypeKind || from_kind == LLVMDoubleTypeKind) && (to_kind == LLVMFloatTypeKind || to_kind == LLVMDoubleTypeKind))
        return 1;

    if ((from_kind == LLVMIntegerTypeKind && (to_kind == LLVMFloatTypeKind || to_kind == LLVMDoubleTypeKind)) || ((from_kind == LLVMFloatTypeKind || from_kind == LLVMDoubleTypeKind) && to_kind == LLVMIntegerTypeKind))
        return 1;

    if ((from_kind == LLVMIntegerTypeKind && to_kind == LLVMPointerTypeKind) || (from_kind == LLVMPointerTypeKind && to_kind == LLVMIntegerTypeKind)) 
        return 1;
    
    return 0;
}

LLVMValueRef generate_cast_instruction(LLVMValueRef value, LLVMTypeRef from_type, LLVMTypeRef to_type, const char* name) {
    if (from_type == to_type) return value;
    
    LLVMTypeKind from_kind = LLVMGetTypeKind(from_type);
    LLVMTypeKind to_kind = LLVMGetTypeKind(to_type);
    
    if (from_kind == LLVMPointerTypeKind && to_kind == LLVMPointerTypeKind)
        return LLVMBuildBitCast(ctx.builder, value, to_type, name);
    
    if (from_kind == LLVMIntegerTypeKind && to_kind == LLVMPointerTypeKind)
        return LLVMBuildIntToPtr(ctx.builder, value, to_type, name);
    
    if (from_kind == LLVMPointerTypeKind && to_kind == LLVMIntegerTypeKind)
        return LLVMBuildPtrToInt(ctx.builder, value, to_type, name);
    
    if (from_kind == LLVMFloatTypeKind && to_kind == LLVMDoubleTypeKind)
        return LLVMBuildFPExt(ctx.builder, value, to_type, name);
    
    if (from_kind == LLVMDoubleTypeKind && to_kind == LLVMFloatTypeKind)
        return LLVMBuildFPTrunc(ctx.builder, value, to_type, name);
    
    if (from_kind == LLVMIntegerTypeKind && (to_kind == LLVMFloatTypeKind || to_kind == LLVMDoubleTypeKind))
        return LLVMBuildSIToFP(ctx.builder, value, to_type, name);
    
    if ((from_kind == LLVMFloatTypeKind || from_kind == LLVMDoubleTypeKind) && to_kind == LLVMIntegerTypeKind)
        return LLVMBuildFPToSI(ctx.builder, value, to_type, name);
    
    return NULL;
}

LLVMValueRef codegen_expression(ASTNode *node) {
    if (!node) return NULL;

    switch (node->type) {
        case AST_ADDRESS_OF:
            return codegen_address_of(node);

        case AST_DEREFERENCE:
            return codegen_dereference(node);

        case AST_CAST:
            return codegen_cast(node);

        case AST_EXPRESSION:
            if (node->name)
                return codegen_variable_load(node->name);
            return codegen_constant(node);

        default:
            printf("Unhandled expression type: %d\n", node->type);
            return NULL;
    }
}

void codegen_statement(ASTNode *node) {
    switch (node->type)
    {
        case AST_RETURN:        codegen_return(node); break;
        case AST_VAR_DECL:      codegen_variable_declaration(node); break;
        case AST_ASSIGN:        codegen_assign(node); break;;
        default:                return;
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

    LLVMTypeRef var_type = token_type_to_llvm_type(&ctx, node->type_info.base_type);
    for (int i = 0; i < node->type_info.pointer_level; i++) {
        var_type = LLVMPointerType(var_type, 0);
    }

    LLVMValueRef alloca = LLVMBuildAlloca(ctx.builder, var_type, node->name);

    if(node->child_count > 0)
    {
        LLVMValueRef init_val = codegen_expression(node->children[0]);
        if (init_val == NULL) 
            return;

        LLVMBuildStore(ctx.builder, init_val, alloca);
    }

    add_variable(&ctx, node->name, alloca, 0, node->type_info.base_type, node->type_info.pointer_level);
}

LLVMValueRef codegen_dereference(ASTNode* node)
{
    if (node->type != AST_DEREFERENCE)
        return NULL;

    if (node->child_count == 0 || !node->children[0])
        return NULL;

    ASTNode* ptr_expr = node->children[0];
    
    if (ptr_expr->type == AST_EXPRESSION && ptr_expr->name) {
        Variable* var = get_variable(&ctx, ptr_expr->name);
        if (!var || !var->alloc) 
            return NULL;

        LLVMValueRef ptr_val;
        if (var->is_global) 
            ptr_val = LLVMBuildLoad2(ctx.builder, LLVMGlobalGetValueType(var->alloc), var->alloc, "ptr_load");
        else 
            ptr_val = LLVMBuildLoad2(ctx.builder, LLVMGetAllocatedType(var->alloc), var->alloc, "ptr_load");
        
        LLVMTypeRef pointed_type = token_type_to_llvm_type(&ctx, var->base_type);
        return LLVMBuildLoad2(ctx.builder, pointed_type, ptr_val, "def");
    }
    return NULL;
}

LLVMValueRef codegen_address_of(ASTNode* node)
{
    if(node->type != AST_ADDRESS_OF)
        return NULL;

    if(node->child_count > 0)
    {
        ASTNode *addressed_of_node = node->children[0];
        Variable *var = get_variable(&ctx, addressed_of_node->name);
            if (!var) return NULL;
        return var->alloc;
    }
    return NULL;
}

void codegen_global_variable_declaration(ASTNode *node) {
    if(node->type != AST_VAR_DECL)
        return;

    LLVMTypeRef var_type = token_type_to_llvm_type(&ctx, node->type_info.base_type);
    for (int i = 0; i < node->type_info.pointer_level; i++) {
        var_type = LLVMPointerType(var_type, 0);
    }

    LLVMValueRef global_var = LLVMAddGlobal(ctx.module, var_type, node->name);

    if(node->child_count > 0)
    {
        LLVMValueRef init_val = codegen_expression(node->children[0]);
        if (init_val != NULL) 
        {
            LLVMSetInitializer(global_var, init_val);
        }
    }
    add_variable(&ctx, node->name, global_var, 1, node->type_info.base_type, node->type_info.pointer_level);
}

void codegen_assign(ASTNode* node)
{
    if(node->type != AST_ASSIGN)
        return ;

    if(node->child_count >= 2)
    {
        ASTNode* lhs = node->children[0];
        ASTNode* rhs = node->children[1];

        LLVMValueRef new_val = codegen_expression(rhs);
        if(!new_val) return;

        if(lhs->type == AST_EXPRESSION && lhs->name) 
        {
            Variable* v = get_variable(&ctx, lhs->name);
            if(v) 
                LLVMBuildStore(ctx.builder, new_val, v->alloc); 
        }

        else if(lhs->type == AST_DEREFERENCE) {
            LLVMValueRef ptr = codegen_expression(lhs->children[0]);
            if(ptr) {
                LLVMBuildStore(ctx.builder, new_val, ptr);
            }
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

    ASTNode* params_node = node->children[0];
    if(params_node->type != AST_PARAM_LIST) {
        printf("Codegen: Expected parameter list node\n");
        return;
    }

    LLVMTypeRef param_types[params_node->child_count] = { };

    for(int i = 0; i<params_node->child_count; i++)
    {
        ASTNode* param = params_node->children[i];
        LLVMTypeRef type = token_type_to_llvm_type(&ctx, param->type_info.base_type);

        const int pointer_level = param->type_info.pointer_level;
        for(int j = 0; j < pointer_level; j++)
            type = LLVMPointerType(type, 0);

        param_types[i] = type; 
    }
    
    LLVMTypeRef ret_type = token_type_to_llvm_type(&ctx, node->type_info.base_type);    

    LLVMTypeRef func_type = LLVMFunctionType(ret_type, param_types, params_node->child_count, 0);
    LLVMValueRef function = LLVMAddFunction(ctx.module, node->name, func_type);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(ctx.context, function, "entry");
    LLVMPositionBuilderAtEnd(ctx.builder, entry);

    for(int i = 0; i < params_node->child_count; i++) {
        codegen_variable_declaration(params_node->children[i]);
    }

    if (node->child_count > 0) {
        codegen_block(node->children[1]);
    }

    if (LLVMVerifyFunction(function, LLVMPrintMessageAction)) {
        printf("Codegen: Function verification failed for %s\n", node->name);
    }
}

void codegen_module_id(const char* name)
{
    if (name == NULL)
        return;

    char* module_id = malloc(strlen("namespace.") + strlen(name) + 1);
    sprintf(module_id, "namespace.%s", name);
    LLVMSetModuleIdentifier(ctx.module, module_id, strlen(module_id));
    free(module_id);
}

void codegen_program(ASTNode* node)
{
    if(node->type != AST_PROGRAM)
    {
        printf("Codegen: Expected program node\n");
        return;
    }

    codegen_module_id(node->name);

    for (int i = 0; i < node->child_count; i++) {

        switch (node->children[i]->type) {
            case AST_FUNCTION:      codegen_function(node->children[i]); break;
            case AST_VAR_DECL:      codegen_global_variable_declaration(node->children[i]); break;
            default: break;
        }
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