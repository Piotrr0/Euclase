#include "codegen.h"
#include "parser.h"
#include "ast_layout.h"
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CodegenContext ctx;
LLVMValueRef current_function;

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
    for(int i = ctx->var_count - 1; i >= 0; i--)
    {
        if (strcmp(ctx->variables[i].name, name) == 0) {
            return &ctx->variables[i];
        }
    }
    return NULL;
}

LLVMValueRef codegen_function_call(ASTNode* node)
{
    if(node->type != AST_FUNC_CALL)
        return NULL;

    const int args_count = node->child_count;
    LLVMValueRef args[args_count];

    for(int i = 0; i<node->child_count; i++) 
    {
        args[i] = codegen_expression(node->children[i]);
        if (args[i] == NULL) {
            printf("Codegen: Failed to generate argument %d for function call\n", i);
            return NULL;
        }
    }

    LLVMValueRef func = LLVMGetNamedFunction(ctx.module, node->name);
    if(func == NULL)
        return NULL;

    LLVMTypeRef func_type = LLVMGlobalGetValueType(func);
    if(func_type == NULL)
        return NULL;

    return LLVMBuildCall2(ctx.builder, func_type, func, args, args_count, "func_call");
}

void codegen_while_loop(ASTNode* node) {
    if (node->type != AST_WHILE)
        return;

    ASTNode* cond_node = node->children[WHILE_CONDITION];
    ASTNode* body_node = node->children[WHILE_BLOCK];

    LLVMBasicBlockRef condBB = LLVMAppendBasicBlock(current_function, "while_cond");
    LLVMBasicBlockRef bodyBB = LLVMAppendBasicBlock(current_function, "while_body");
    LLVMBasicBlockRef afterBB= LLVMAppendBasicBlock(current_function, "while_after");

    LLVMBuildBr(ctx.builder, condBB);

    LLVMPositionBuilderAtEnd(ctx.builder, condBB);
    LLVMValueRef cond_val = codegen_expression(cond_node);
    LLVMBuildCondBr(ctx.builder, cond_val, bodyBB, afterBB);

    LLVMPositionBuilderAtEnd(ctx.builder, bodyBB);
    codegen_block(body_node);

    if (LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(ctx.builder)) == NULL)
        LLVMBuildBr(ctx.builder, condBB);

    LLVMPositionBuilderAtEnd(ctx.builder, afterBB);
}

void codegen_for_loop(ASTNode* node)
{
    if (node->type != AST_FOR)
        return;

    ASTNode* init_node = node->children[FOR_INIT];
    ASTNode* cond_node = node->children[FOR_CONDITION];
    ASTNode* update_node = node->children[FOR_UPDATE];
    ASTNode* body_node = node->children[FOR_BLOCK];

    if (init_node != NULL)
        codegen_statement(init_node);

    LLVMBasicBlockRef updBB  = LLVMAppendBasicBlock(current_function, "for_upd");
    LLVMBasicBlockRef condBB = LLVMAppendBasicBlock(current_function, "for_cond");
    LLVMBasicBlockRef bodyBB = LLVMAppendBasicBlock(current_function, "for_body");
    LLVMBasicBlockRef afterBB= LLVMAppendBasicBlock(current_function, "for_after");

    LLVMBuildBr(ctx.builder, condBB);
    LLVMPositionBuilderAtEnd(ctx.builder, condBB);
    LLVMValueRef cond_val = codegen_expression(cond_node);
    LLVMBuildCondBr(ctx.builder, cond_val, bodyBB, afterBB);

    LLVMPositionBuilderAtEnd(ctx.builder, bodyBB);
    codegen_block(body_node);
    if (LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(ctx.builder)) == NULL)
        LLVMBuildBr(ctx.builder, updBB);

    LLVMPositionBuilderAtEnd(ctx.builder, updBB);
    if (update_node != NULL)
        codegen_statement(update_node);
    LLVMBuildBr(ctx.builder, condBB);

    LLVMPositionBuilderAtEnd(ctx.builder, afterBB);
}

int does_type_kind_match(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind* out_kind)
{

    if(left == NULL || right == NULL)
        return 0;

    LLVMTypeRef left_type = LLVMTypeOf(left);
    LLVMTypeKind left_kind = LLVMGetTypeKind(left_type);

    LLVMTypeRef right_type = LLVMTypeOf(right);
    LLVMTypeKind right_kind = LLVMGetTypeKind(right_type);

    if(left_kind == right_kind) {
        *out_kind = left_kind;
        return 1;
    }

    return 0;
}

void codegen_condition(ASTNode* node)
{
    if(node->type != AST_IF)
        return;

    ASTNode* node_condition = NULL;
    ASTNode* node_then_block = NULL;
    ASTNode* node_else_block = NULL;

    switch(node->child_count) {
        case 3: node_else_block = node->children[IF_ELSE_BLOCK];
        case 2: node_then_block = node->children[IF_THEN_BLOCK];
        case 1: node_condition = node->children[IF_CONDITION]; break; 
        default: return;
    }

    if(node_condition == NULL || node_then_block == NULL)
        return;

    LLVMValueRef condition = codegen_expression(node_condition);
    LLVMBasicBlockRef thenBB  = LLVMAppendBasicBlock(current_function, "then");
    LLVMBasicBlockRef mergeBB = LLVMAppendBasicBlock(current_function, "if_cont");

    LLVMBasicBlockRef elseBB = NULL;
    if (node_else_block != NULL) {
        elseBB = LLVMAppendBasicBlock(current_function, "else");
        LLVMBuildCondBr(ctx.builder, condition, thenBB, elseBB);
    }
    else
        LLVMBuildCondBr(ctx.builder, condition, thenBB, mergeBB);

    LLVMPositionBuilderAtEnd(ctx.builder, thenBB);
    codegen_then_block(node_then_block, mergeBB);

    if (node_else_block != NULL) {
        LLVMPositionBuilderAtEnd(ctx.builder, elseBB);
        codegen_else_block(node_else_block, mergeBB);
    }

    LLVMPositionBuilderAtEnd(ctx.builder, mergeBB);
}

void codegen_then_block(ASTNode* node_block, LLVMBasicBlockRef mergeBB)
{
    if(node_block == NULL || mergeBB == NULL)
        return;

    codegen_block(node_block);

    LLVMBasicBlockRef current_block = LLVMGetInsertBlock(ctx.builder);
    if (LLVMGetBasicBlockTerminator(current_block) == NULL)
        LLVMBuildBr(ctx.builder, mergeBB);
}

void codegen_else_block(ASTNode* node_else, LLVMBasicBlockRef mergeBB)
{
    if (node_else == NULL || mergeBB == NULL) 
        return;
    
    codegen_block(node_else);

    LLVMBasicBlockRef current_block = LLVMGetInsertBlock(ctx.builder);
    if (LLVMGetBasicBlockTerminator(current_block) == NULL)
        LLVMBuildBr(ctx.builder, mergeBB);
}

LLVMValueRef codegen_equality(ASTNode* node) {
    if (node->type != AST_EQUAL && node->type != AST_NOT_EQUAL)
        return NULL;

    LLVMValueRef left  = codegen_expression(node->children[BIN_LEFT]);
    LLVMValueRef right = codegen_expression(node->children[BIN_RIGHT]);

    LLVMTypeKind type;
    if (!does_type_kind_match(left, right, &type))
        return NULL;

    int is_not_equal = (node->type == AST_NOT_EQUAL);
    return codegen_compare(left, right, type, is_not_equal);
}

LLVMValueRef codegen_relational(ASTNode* node) {
    if(node->type != AST_LESS && node->type != AST_GREATER)
        return NULL;

    LLVMValueRef left  = codegen_expression(node->children[BIN_LEFT]);
    LLVMValueRef right = codegen_expression(node->children[BIN_RIGHT]);

    LLVMTypeKind type;
    if (!does_type_kind_match(left, right, &type))
        return NULL;

    return node->type == AST_LESS ? codegen_less(left, right, type) : codegen_greater(left, right, type);
}

LLVMValueRef codegen_compare(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type, int is_not_equal)
{
    if (type == LLVMFloatTypeKind || type == LLVMDoubleTypeKind)
        return LLVMBuildFCmp(ctx.builder, is_not_equal ? LLVMRealONE : LLVMRealOEQ, left, right, "fcmp");
    else
        return LLVMBuildICmp(ctx.builder, is_not_equal ? LLVMIntNE : LLVMIntEQ, left, right, "icmp");
}

LLVMValueRef codegen_greater(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type) {
    if(type == LLVMFloatTypeKind || type == LLVMDoubleTypeKind)
        return LLVMBuildFCmp(ctx.builder, LLVMRealOGT, left, right, "fgr");
    else
        return LLVMBuildICmp(ctx.builder,LLVMIntSGT, left, right, "lgr");
}

LLVMValueRef codegen_less(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type) {
    if (type == LLVMFloatTypeKind || type == LLVMDoubleTypeKind)
        return LLVMBuildFCmp(ctx.builder, LLVMRealOLT, left, right, "flt");
    else
        return LLVMBuildICmp(ctx.builder, LLVMIntSLT, left, right, "llt");
}

LLVMValueRef codegen_arithmetic_op(ASTNode* node) {
    LLVMValueRef left = codegen_expression(node->children[BIN_LEFT]);
    LLVMValueRef right = codegen_expression(node->children[BIN_RIGHT]);

    if(left == NULL || right == NULL)
        return NULL;

    LLVMTypeKind type;
    if(!does_type_kind_match(left, right, &type))
        return  NULL;

    switch (node->type) {
        case AST_ADDITION:          return codegen_addition(left, right, type);
        case AST_SUBTRACTION:       return codegen_subtraction(left, right, type);
        case AST_MULTIPLICATION:    return codegen_multiplication(left, right, type);
        case AST_DIVISION:          return codegen_division(left, right, type);
        case AST_MODULO:            return codegen_modulo(left, right, type);
        default: return NULL;
    }
}

LLVMValueRef codegen_addition(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type)
{
    if (type == LLVMFloatTypeKind || type == LLVMDoubleTypeKind)
        return LLVMBuildFAdd(ctx.builder, left, right, "fadd");
    else
        return LLVMBuildAdd(ctx.builder, left, right, "add");
}

LLVMValueRef codegen_subtraction(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type)
{
    if (type == LLVMFloatTypeKind || type == LLVMDoubleTypeKind)
        return LLVMBuildFSub(ctx.builder, left, right, "fsub");
    else
        return LLVMBuildSub(ctx.builder, left, right, "sub");
}

LLVMValueRef codegen_multiplication(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type)
{
    if (type == LLVMFloatTypeKind || type == LLVMDoubleTypeKind)
        return LLVMBuildFMul(ctx.builder, left, right, "fmul");
    else
        return LLVMBuildMul(ctx.builder, left, right, "mul");
}

LLVMValueRef codegen_division(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type) {
    if (type == LLVMFloatTypeKind || type == LLVMDoubleTypeKind)
        return LLVMBuildFDiv(ctx.builder, left, right, "fdiv");
    else
        return LLVMBuildSDiv(ctx.builder, left, right, "sdiv");
}

LLVMValueRef codegen_modulo(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type) {
    if (type == LLVMFloatTypeKind || type == LLVMDoubleTypeKind)
        return LLVMBuildFRem(ctx.builder, left, right, "frem");
    else
        return LLVMBuildSRem(ctx.builder, left, right, "srem");
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

    LLVMValueRef value = codegen_expression(node->children[UNARY_OPERAND]);
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

LLVMValueRef codegen_unary_minus(ASTNode* node) {
    if (node->type != AST_UNARY_MINUS)
        return NULL;

    LLVMValueRef expression = codegen_expression(node->children[UNARY_OPERAND]);
    if (expression == NULL)
        return NULL;

    LLVMTypeRef type = LLVMTypeOf(expression);
    LLVMTypeKind type_kind = LLVMGetTypeKind(type);

    if (type_kind == LLVMFloatTypeKind || type_kind == LLVMDoubleTypeKind)
        return LLVMBuildFNeg(ctx.builder, expression, "fneg");
    else if (type_kind == LLVMIntegerTypeKind)
        return LLVMBuildNeg(ctx.builder, expression, "neg");

    return NULL;
}

LLVMValueRef codegen_expression(ASTNode *node) {
    if (node == NULL) 
        return NULL;

    switch (node->type) {
        case AST_EXPRESSION:    return codegen_constant(node);
        case AST_IDENTIFIER:    return codegen_variable_load(node->name);
        case AST_FUNC_CALL:     return codegen_function_call(node);
        case AST_ADDRESS_OF:    return codegen_address_of(node);
        case AST_DEREFERENCE:   return codegen_dereference(node);
        case AST_CAST:          return codegen_cast(node);
        case AST_UNARY_MINUS:   return codegen_unary_minus(node);

        case AST_ADDITION:      
        case AST_SUBTRACTION:
        case AST_MULTIPLICATION:
        case AST_DIVISION:
        case AST_MODULO:        return codegen_arithmetic_op(node);

        case AST_EQUAL:
        case AST_NOT_EQUAL:     return codegen_equality(node);

        case AST_LESS:
        case AST_GREATER:       return codegen_relational(node);

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
        case AST_ASSIGN:        codegen_assign(node); break;
        case AST_IF:            codegen_condition(node); break;
        case AST_FOR:           codegen_for_loop(node); break;
        case AST_WHILE:         codegen_while_loop(node); break;
        default:                return;
    }
}

void codegen_return(ASTNode* node) {
    if(node->type != AST_RETURN)
        return;

    if(node->child_count > 0)
    {
        LLVMValueRef ret = codegen_expression(node->children[RETURN_VALUE]);
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
        LLVMValueRef init_val = codegen_expression(node->children[VAR_DECL_INITIALIZER]);
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

    if (node->child_count == 0 || !node->children[UNARY_OPERAND])
        return NULL;

    ASTNode* ptr_expr = node->children[UNARY_OPERAND];
    
    if (ptr_expr->type == AST_IDENTIFIER) {
        Variable* var = get_variable(&ctx, ptr_expr->name);
        if (!var || !var->alloc) 
            return NULL;

        if (var->pointer_level <= 0) {
           printf("Codegen: Cannot dereference non-pointer variable '%s'\n", ptr_expr->name);
            return NULL;
        }

        LLVMValueRef ptr_val;
        if (var->is_global) 
            ptr_val = LLVMBuildLoad2(ctx.builder, LLVMGlobalGetValueType(var->alloc), var->alloc, "ptr_load");
        else 
            ptr_val = LLVMBuildLoad2(ctx.builder, LLVMGetAllocatedType(var->alloc), var->alloc, "ptr_load");
        
        LLVMTypeRef pointed_type = token_type_to_llvm_type(&ctx, var->base_type);
        for (int j = 0; j < var->pointer_level - 1; j++) {
            pointed_type = LLVMPointerType(pointed_type, 0);
        }
        
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
        ASTNode *addressed_of_node = node->children[UNARY_OPERAND];
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
        LLVMValueRef init_val = codegen_expression(node->children[VAR_DECL_INITIALIZER]);
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
        ASTNode* lhs = node->children[ASSIGN_TARGET];
        ASTNode* rhs = node->children[ASSIGN_VALUE];

        LLVMValueRef new_val = codegen_expression(rhs);
        if(new_val == NULL) 
            return;

        if(lhs->type == AST_IDENTIFIER) 
        {
            Variable* v = get_variable(&ctx, lhs->name);
            if(v != NULL) 
                LLVMBuildStore(ctx.builder, new_val, v->alloc); 
        }

        else if(lhs->type == AST_DEREFERENCE) {
            LLVMValueRef ptr = codegen_expression(lhs->children[UNARY_OPERAND]);
            if(ptr != NULL)
                LLVMBuildStore(ctx.builder, new_val, ptr);
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

    ASTNode* params_node = node->children[FUNC_PARAMS];
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

    current_function = function;
    
    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(ctx.context, function, "entry");
    LLVMPositionBuilderAtEnd(ctx.builder, entry);

    for(int i = 0; i < params_node->child_count; i++) {
        codegen_variable_declaration(params_node->children[i]);
    
        LLVMValueRef param_val = LLVMGetParam(function, i);
        Variable* var = get_variable(&ctx, params_node->children[i]->name);
        if (var && var->alloc) {
            LLVMBuildStore(ctx.builder, param_val, var->alloc);
        }
    }


    ASTNode* func_block = node->children[FUNC_BODY];
    codegen_block(func_block);

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

        ASTNode* child = node->children[i];
        switch (child->type) {
            case AST_FUNCTION:      codegen_function(child); break;
            case AST_VAR_DECL:      codegen_global_variable_declaration(child); break;
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