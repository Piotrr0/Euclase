#include "codegen.h"
#include "parser.h"
#include "token.h"
#include <llvm-c/Core.h>
#include <llvm-c/Types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CodegenContext ctx;
SymbolTable* st;
LLVMValueRef current_function;

void init_codegen(CodegenContext* ctx, const char* module_name) {
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();
    
    ctx->context = LLVMContextCreate();
    ctx->module = LLVMModuleCreateWithNameInContext(module_name, ctx->context);
    ctx->builder = LLVMCreateBuilderInContext(ctx->context);

    st = init_symbol_table();
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

LLVMValueRef codegen_function_call(ASTNode* node)
{
    if(node->type != AST_FUNC_CALL)
        return NULL;

    FuncCallNode function_call_node = node->as.func_call;
    LLVMValueRef args[function_call_node.arg_count];

    for(int i = 0; i < function_call_node.arg_count; i++) 
    {
        args[i] = codegen_expression(function_call_node.args[i]);
        if (args[i] == NULL) {
            printf("Codegen: Failed to generate argument %d for function call\n", i);
            return NULL;
        }
    }

    LLVMValueRef func = LLVMGetNamedFunction(ctx.module, function_call_node.name);
    if(func == NULL)
        return NULL;

    LLVMTypeRef func_type = LLVMGlobalGetValueType(func);
    if(func_type == NULL)
        return NULL;

    return LLVMBuildCall2(ctx.builder, func_type, func, args, function_call_node.arg_count, "func_call");
}

void codegen_while_loop(ASTNode* node) {
    if (node->type != AST_WHILE)
        return;

    WhileNode while_node = node->as.while_stmt;

    LLVMBasicBlockRef condBB = LLVMAppendBasicBlock(current_function, "while_cond");
    LLVMBasicBlockRef bodyBB = LLVMAppendBasicBlock(current_function, "while_body");
    LLVMBasicBlockRef afterBB= LLVMAppendBasicBlock(current_function, "while_after");

    LLVMBuildBr(ctx.builder, condBB);

    LLVMPositionBuilderAtEnd(ctx.builder, condBB);
    LLVMValueRef cond_val = codegen_expression(while_node.condition);
    LLVMBuildCondBr(ctx.builder, cond_val, bodyBB, afterBB);

    LLVMPositionBuilderAtEnd(ctx.builder, bodyBB);
    codegen_block(while_node.body);

    if (LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(ctx.builder)) == NULL)
        LLVMBuildBr(ctx.builder, condBB);

    LLVMPositionBuilderAtEnd(ctx.builder, afterBB);
}

void codegen_for_loop(ASTNode* node)
{
    if (node->type != AST_FOR)
        return;

    ForNode for_node = node->as.for_stmt;

    push_scope(st);

    if (for_node.init != NULL)
        codegen_statement(for_node.init);

    LLVMBasicBlockRef updBB  = LLVMAppendBasicBlock(current_function, "for_upd");
    LLVMBasicBlockRef condBB = LLVMAppendBasicBlock(current_function, "for_cond");
    LLVMBasicBlockRef bodyBB = LLVMAppendBasicBlock(current_function, "for_body");
    LLVMBasicBlockRef afterBB= LLVMAppendBasicBlock(current_function, "for_after");

    LLVMBuildBr(ctx.builder, condBB);
    LLVMPositionBuilderAtEnd(ctx.builder, condBB);
    LLVMValueRef cond_val = codegen_expression(for_node.condition);
    LLVMBuildCondBr(ctx.builder, cond_val, bodyBB, afterBB);

    LLVMPositionBuilderAtEnd(ctx.builder, bodyBB);
    codegen_block(for_node.body);
    if (LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(ctx.builder)) == NULL)
        LLVMBuildBr(ctx.builder, updBB);

    LLVMPositionBuilderAtEnd(ctx.builder, updBB);
    if (for_node.update != NULL)
        codegen_statement(for_node.update);
    LLVMBuildBr(ctx.builder, condBB);

    LLVMPositionBuilderAtEnd(ctx.builder, afterBB);
    pop_scope(st);
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

void codegen_if(ASTNode* node)
{
    if(node->type != AST_IF)
        return;

    IfNode if_node = node->as.if_stmt;

    LLVMValueRef condition = codegen_expression(if_node.condition);
    LLVMBasicBlockRef thenBB  = LLVMAppendBasicBlock(current_function, "then");
    LLVMBasicBlockRef mergeBB = LLVMAppendBasicBlock(current_function, "if_cont");

    LLVMBasicBlockRef elseBB = NULL;
    if (if_node.else_branch != NULL) {
        elseBB = LLVMAppendBasicBlock(current_function, "else");
        LLVMBuildCondBr(ctx.builder, condition, thenBB, elseBB);
    }
    else
        LLVMBuildCondBr(ctx.builder, condition, thenBB, mergeBB);

    LLVMPositionBuilderAtEnd(ctx.builder, thenBB);
    codegen_then_block(if_node.then_branch, mergeBB);

    if (if_node.else_branch != NULL) {
        LLVMPositionBuilderAtEnd(ctx.builder, elseBB);
        codegen_else_block(if_node.else_branch, mergeBB);
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

LLVMValueRef codegen_binary_op(ASTNode* node) {
    if (node->type != AST_BINARY_OP)
        return NULL;

    BinaryOpNode binary_node = node->as.binary_op;

    LLVMValueRef left = codegen_expression(binary_node.left);
    LLVMValueRef right = codegen_expression(binary_node.right);

    if(left == NULL || right == NULL)
        return NULL;

    LLVMTypeKind type;
    if(!does_type_kind_match(left, right, &type))
        return NULL;

    switch (node->as.binary_op.op) {
        case OP_ADD: return codegen_addition(left, right, type);
        case OP_SUB: return codegen_subtraction(left, right, type);
        case OP_MUL: return codegen_multiplication(left, right, type);
        case OP_DIV: return codegen_division(left, right, type);
        case OP_MOD: return codegen_modulo(left, right, type);
        case OP_EQ:  return codegen_compare(left, right, type, 0);
        case OP_NE:  return codegen_compare(left, right, type, 1);
        case OP_LT:  return codegen_less(left, right, type);
        case OP_GT:  return codegen_greater(left, right, type);
        case OP_LE:  return codegen_less_equal(left, right, type);
        case OP_GE:  return codegen_greater_equal(left, right, type);
        default:     return NULL;
    }
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
        return LLVMBuildICmp(ctx.builder, LLVMIntSGT, left, right, "lgr");
}

LLVMValueRef codegen_less(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type) {
    if (type == LLVMFloatTypeKind || type == LLVMDoubleTypeKind)
        return LLVMBuildFCmp(ctx.builder, LLVMRealOLT, left, right, "flt");
    else
        return LLVMBuildICmp(ctx.builder, LLVMIntSLT, left, right, "llt");
}

LLVMValueRef codegen_less_equal(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type) {
    if (type == LLVMFloatTypeKind || type == LLVMDoubleTypeKind)
        return LLVMBuildFCmp(ctx.builder, LLVMRealOLE, left, right, "fle");
    else
        return LLVMBuildICmp(ctx.builder, LLVMIntSLE, left, right, "lle");
}

LLVMValueRef codegen_greater_equal(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type) {
    if (type == LLVMFloatTypeKind || type == LLVMDoubleTypeKind)
        return LLVMBuildFCmp(ctx.builder, LLVMRealOGE, left, right, "fge");
    else
        return LLVMBuildICmp(ctx.builder, LLVMIntSGE, left, right, "lle");
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
    
    SymbolEntry* entry = lookup_symbol(st, name);
    if (entry == NULL) {
        printf("Codegen: Undefined variable '%s'\n", name);
        return NULL;
    }
    
    SymbolData data = entry->symbol_data;
    if (data.alloc == NULL) {
        return NULL;
    }

    LLVMValueRef alloca = data.alloc;

    if (data.is_global) 
        return LLVMBuildLoad2(ctx.builder, LLVMGlobalGetValueType(alloca), alloca, "global_load");
    else
        return LLVMBuildLoad2(ctx.builder, LLVMGetAllocatedType(alloca), alloca, "local_load");
}

LLVMValueRef codegen_int_literal(ASTNode* node)
{
    if (node->type != AST_INT_LITERAL)
        return NULL;
    return LLVMConstInt(LLVMInt32TypeInContext(ctx.context), node->as.int_literal.value, 0);
}

LLVMValueRef codegen_float_literal(ASTNode* node)
{
    if (node->type != AST_FLOAT_LITERAL)
        return NULL;
    return LLVMConstReal(LLVMFloatTypeInContext(ctx.context), node->as.float_literal.value);
}

LLVMValueRef codegen_double_literal(ASTNode* node)
{
    if (node->type != AST_DOUBLE_LITERAL)
        return NULL;
    return LLVMConstReal(LLVMDoubleTypeInContext(ctx.context), node->as.double_literal.value);
}

LLVMValueRef codegen_string_literal(ASTNode* node) {
    if (node->type != AST_STRING_LITERAL)
        return NULL;
    return LLVMBuildGlobalStringPtr(ctx.builder, node->as.string_literal.value, "str");
}

LLVMValueRef codegen_char_literal(ASTNode* node) {
    if (node->type != AST_CHAR_LITERAL)
        return NULL;
    return LLVMConstInt(LLVMInt8TypeInContext(ctx.context), node->as.char_literal.value, 0);
}

LLVMValueRef codegen_cast(ASTNode* node) 
{
    if(node->type != AST_CAST)
        return NULL;

    CastNode cast_node = node->as.cast;

    LLVMValueRef value = codegen_expression(cast_node.expr);
    if(value == NULL)
    {
        printf("Codegen: Failed to generate expression for cast\n");
        return NULL;
    }

    LLVMTypeRef from_type = LLVMTypeOf(value);
    LLVMTypeRef to_type = token_type_to_llvm_type(&ctx, cast_node.target_type.base_type);

    for (int i = 0; i < cast_node.target_type.pointer_level; i++) {
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

LLVMValueRef codegen_negation(ASTNode* node) {
    if (node->type != AST_NEGATION)
        return NULL;

    UnaryOpNode negation_node =node->as.unary_op;

    LLVMValueRef expression = codegen_expression(negation_node.operand);
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
        case AST_INT_LITERAL:   return codegen_int_literal(node);
        case AST_FLOAT_LITERAL: return codegen_float_literal(node);
        case AST_DOUBLE_LITERAL:return codegen_double_literal(node);
        case AST_STRING_LITERAL:return codegen_string_literal(node);
        case AST_CHAR_LITERAL:  return codegen_char_literal(node);
        case AST_IDENTIFIER:    return codegen_variable_load(node->as.identifier.name);
        case AST_FUNC_CALL:     return codegen_function_call(node);
        case AST_ADDRESS_OF:    return codegen_address_of(node);
        case AST_DEREFERENCE:   return codegen_dereference(node);
        case AST_CAST:          return codegen_cast(node);
        case AST_NEGATION:      return codegen_negation(node);
        case AST_BINARY_OP:     return codegen_binary_op(node);

        default:
            printf("Unhandled expression type: %d\n", node->type);
            return NULL;
    }
}

void codegen_statement(ASTNode *node) {
    if (node == NULL)
        return;
        
    switch (node->type)
    {
        case AST_RETURN:    codegen_return(node); break;
        case AST_VAR_DECL:  codegen_variable_declaration(node); break;
        case AST_ASSIGN:    codegen_assign(node); break;
        case AST_IF:        codegen_if(node); break;
        case AST_FOR:       codegen_for_loop(node); break;
        case AST_WHILE:     codegen_while_loop(node); break;
        case AST_BLOCK:     codegen_block(node); break;
        default:            return;
    }
}

void codegen_return(ASTNode* node) {
    if(node->type != AST_RETURN)
        return;

    ReturnNode return_node = node->as.return_stmt;

    if(node->as.return_stmt.value != NULL)
    {
        LLVMValueRef ret = codegen_expression(return_node.value);
        if(ret)
            LLVMBuildRet(ctx.builder, ret);
        else
            LLVMBuildRetVoid(ctx.builder);
    } 
    else {
        LLVMBuildRetVoid(ctx.builder);
    }
}

void codegen_variable_declaration(ASTNode* node) {
    if(node->type != AST_VAR_DECL)
        return;

    VarDeclNode var_decl_node = node->as.var_decl;
    TypeInfo var_decl_type = var_decl_node.type;

    LLVMTypeRef var_type = token_type_to_llvm_type(&ctx, var_decl_type.base_type);
    for (int i = 0; i < var_decl_type.pointer_level; i++) {
        var_type = LLVMPointerType(var_type, 0);
    }

    LLVMValueRef alloca = LLVMBuildAlloca(ctx.builder, var_type, var_decl_node.name);

    if(var_decl_node.initializer != NULL)
    {
        LLVMValueRef init_val = codegen_expression(var_decl_node.initializer);
        if (init_val == NULL) 
            return;

        LLVMBuildStore(ctx.builder, init_val, alloca);
    }

    SymbolData data = {
        .name = var_decl_node.name,
        .is_global = 0,
        .info = var_decl_node.type,
        .alloc = alloca,
    };
    add_symbol(st, data);
}

void codegen_global_variable_declaration(ASTNode *node) {
    if(node->type != AST_VAR_DECL)
        return;

    VarDeclNode var_decl_node = node->as.var_decl;
    TypeInfo var_decl_type = var_decl_node.type;

    LLVMTypeRef var_type = token_type_to_llvm_type(&ctx, var_decl_type.base_type);
    for (int i = 0; i < var_decl_type.pointer_level; i++) {
        var_type = LLVMPointerType(var_type, 0);
    }

    LLVMValueRef global_var = LLVMAddGlobal(ctx.module, var_type, var_decl_node.name);

    if(var_decl_node.initializer != NULL) {
        LLVMValueRef init_val = codegen_expression(var_decl_node.initializer);
        if (init_val != NULL) {
            LLVMSetInitializer(global_var, init_val);
        }
    }
    
    SymbolData data = {
        .name = var_decl_node.name,
        .is_global = 1,
        .info = var_decl_node.type,
        .alloc = global_var,
    };
    add_symbol(st, data);
}

void codegen_struct_declaration(ASTNode* node) {
    if (node->type != AST_STRUCT_DECL)
        return;
    
    StructDeclNode struct_decl_node = node->as.struct_decl;

    LLVMTypeRef structType = LLVMStructCreateNamed(LLVMGetGlobalContext(), struct_decl_node.name);
    LLVMTypeRef* field_types = malloc(sizeof(LLVMTypeRef) * struct_decl_node.member_count);
    
    for (int i = 0; i < struct_decl_node.member_count; i++)
    {
        ASTNode* field = struct_decl_node.members[i];
        VarDeclNode filed_decl_node = field->as.var_decl;

        LLVMTypeRef field_type = token_type_to_llvm_type(&ctx, filed_decl_node.type.base_type);
        for (int j = 0; j < filed_decl_node.type.pointer_level; j++)
            field_type = LLVMPointerType(field_type, 0);

        field_types[i] = field_type;
    }

    LLVMStructSetBody(structType, field_types, struct_decl_node.member_count, 0);
    free(field_types);
}

LLVMValueRef codegen_dereference(ASTNode* node)
{
    if (node->type != AST_DEREFERENCE)
        return NULL;

    UnaryOpNode dereference_node = node->as.unary_op;

    if (dereference_node.operand == NULL)
        return NULL;

    ASTNode* ptr_expr = dereference_node.operand;
    if (ptr_expr->type != AST_IDENTIFIER) 
        return NULL;

    SymbolEntry* entry = lookup_symbol(st, ptr_expr->as.identifier.name);
    if (entry == NULL)
        return NULL;
        
    SymbolData data = entry->symbol_data;
    LLVMValueRef alloca = data.alloc;

    if (alloca == NULL) {
        return NULL;
    }

    if (entry->symbol_data.info.pointer_level <= 0) {
        printf("Codegen: Cannot dereference non-pointer variable '%s'\n", ptr_expr->as.identifier.name);
        return NULL;
    }

    LLVMValueRef ptr_val;
    if (data.is_global) 
        ptr_val = LLVMBuildLoad2(ctx.builder, LLVMGlobalGetValueType(alloca), alloca, "ptr_load");
    else 
        ptr_val = LLVMBuildLoad2(ctx.builder, LLVMGetAllocatedType(alloca), alloca, "ptr_load");
        
    LLVMTypeRef pointed_type = token_type_to_llvm_type(&ctx, data.info.base_type);
    for (int j = 0; j < data.info.pointer_level - 1; j++) {
        pointed_type = LLVMPointerType(pointed_type, 0);
    }
        
    return LLVMBuildLoad2(ctx.builder, pointed_type, ptr_val, "deref");
}

LLVMValueRef codegen_address_of(ASTNode* node)
{
    if(node->type != AST_ADDRESS_OF)
        return NULL;

    if(node->as.unary_op.operand == NULL)
        return NULL;

    ASTNode* addressed_of_node = node->as.unary_op.operand;
    if (addressed_of_node->type != AST_IDENTIFIER)
        return NULL;
        
    SymbolEntry* entry = lookup_symbol(st, addressed_of_node->as.identifier.name);
    if (entry == NULL) {
        return NULL;
    }
    return entry->symbol_data.alloc;
}

void codegen_assign(ASTNode* node)
{
    if(node->type != AST_ASSIGN)
        return;

    AssignNode assign_node = node->as.assign;
    ASTNode* lhs = assign_node.target;
    ASTNode* rhs = assign_node.value;

    if (lhs == NULL || rhs == NULL)
        return;

    LLVMValueRef new_val = codegen_expression(rhs);
    if(new_val == NULL) 
        return;

    if(lhs->type == AST_IDENTIFIER) 
    {
        const char* lhs_name = lhs->as.identifier.name;
        SymbolEntry* entry = lookup_symbol(st, lhs_name);
        if(entry == NULL) {
            printf("Codegen: Undefined variable '%s'\n", lhs_name);
            return;
        }

        LLVMBuildStore(ctx.builder, new_val, entry->symbol_data.alloc); 
    }
    else if(lhs->type == AST_DEREFERENCE) {
        UnaryOpNode unary_node = lhs->as.unary_op;
        LLVMValueRef ptr = codegen_expression(unary_node.operand);
        if(ptr == NULL)
            return;

        LLVMBuildStore(ctx.builder, new_val, ptr);
    }
}

void codegen_block(ASTNode* node) {
    if(node->type != AST_BLOCK)
    {
        printf("Codegen: Expected block node\n");
        return;
    }

    BlockNode block_node = node->as.block;
    
    push_scope(st);
    for(int i = 0; i < block_node.statement_count; i++)
    {
        codegen_statement(block_node.statements[i]);
    }
    pop_scope(st);
}

// TODO: clean this mess
void codegen_function(ASTNode* node) {
    if(node->type != AST_FUNCTION)
    {
        printf("Codegen: Expected function node\n");
        return;
    }

    FunctionNode function_node = node->as.function;

    LLVMTypeRef param_types[function_node.param_count];
    for(int i = 0; i < function_node.param_count; i++)
    {
        ASTNode* param = function_node.params[i];
        TypeInfo param_info = param->as.param.type;

        LLVMTypeRef type = token_type_to_llvm_type(&ctx, param_info.base_type);

        const int pointer_level = param_info.pointer_level;
        for(int j = 0; j < pointer_level; j++)
            type = LLVMPointerType(type, 0);

        param_types[i] = type; 
    }
    
    LLVMTypeRef ret_type = token_type_to_llvm_type(&ctx, function_node.return_type.base_type);    

    LLVMTypeRef func_type = LLVMFunctionType(ret_type, param_types, function_node.param_count, 0);
    LLVMValueRef function = LLVMAddFunction(ctx.module, function_node.name, func_type);
    current_function = function;
    
    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(ctx.context, function, "entry");
    LLVMPositionBuilderAtEnd(ctx.builder, entry);

    push_scope(st);
    for(int i = 0; i < function_node.param_count; i++) {
        ASTNode* param = function_node.params[i];
        TypeInfo param_info = param->as.param.type;

        LLVMTypeRef param_type = token_type_to_llvm_type(&ctx, param_info.base_type);
        for (int j = 0; j < param_info.pointer_level; j++) {
            param_type = LLVMPointerType(param_type, 0);
        }
        
        LLVMValueRef alloca = LLVMBuildAlloca(ctx.builder, param_type, param->as.param.name);
        
        SymbolData data = {
            .name = param->as.param.name,
            .is_global = 0,
            .info = param->as.param.type,
            .alloc = alloca,
        };
        add_symbol(st, data);
    
        LLVMValueRef param_val = LLVMGetParam(function, i);
        LLVMBuildStore(ctx.builder, param_val, alloca);
    }

    ASTNode* func_block = node->as.function.body;
    if(func_block != NULL && func_block->type == AST_BLOCK) {
        for(int i = 0; i < func_block->as.block.statement_count; i++) {
            codegen_statement(func_block->as.block.statements[i]);
        }
    }

    pop_scope(st);

    if (LLVMVerifyFunction(function, LLVMPrintMessageAction)) {
        printf("Codegen: Function verification failed for %s\n", node->as.function.name);
    }
}

void codegen_module_id(const char* name)
{
    if (name == NULL)
        return;

    char* module_id = malloc(strlen("namespace.") + strlen(name) + 1);
    if (module_id == NULL)
        return;

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
    ProgramNode program_node = node->as.program;

    codegen_module_id(program_node.name);

    for (int i = 0; i < program_node.struct_count; i++) {
        codegen_struct_declaration(program_node.structs[i]);
    }

    for (int i = 0; i < program_node.global_count; i++) {
        codegen_global_variable_declaration(program_node.globals[i]);
    }

    for (int i = 0; i < program_node.function_count; i++) {
        codegen_function(program_node.functions[i]);
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