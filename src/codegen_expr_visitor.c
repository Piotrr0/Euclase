#include "codegen_expr_visitor.h"
#include <stdio.h>
#include <string.h>

LLVMValueRef visit_int_literal_expr(CodegenVisitor* visitor, ASTNode* node) {
    IntLiteralNode int_node = node->as.int_literal;
    return LLVMConstInt(LLVMInt32TypeInContext(visitor->ctx->context), int_node.value, 0);
}

LLVMValueRef visit_float_literal_expr(CodegenVisitor* visitor, ASTNode* node) {
    FloatLiteralNode float_node = node->as.float_literal;
    return LLVMConstReal(LLVMFloatTypeInContext(visitor->ctx->context), float_node.value);
}

LLVMValueRef visit_double_literal_expr(CodegenVisitor* visitor, ASTNode* node) {
    DoubleLiteralNode double_node = node->as.double_literal;
    return LLVMConstReal(LLVMDoubleTypeInContext(visitor->ctx->context), double_node.value);
}

LLVMValueRef visit_string_literal_expr(CodegenVisitor* visitor, ASTNode* node) {
    StringLiteralNode string_node = node->as.string_literal;
    return LLVMBuildGlobalStringPtr(visitor->ctx->builder, string_node.value, "str");
}

LLVMValueRef visit_char_literal_expr(CodegenVisitor* visitor, ASTNode* node) {
    CharLiteralNode char_node = node->as.char_literal;
    return LLVMConstInt(LLVMInt8TypeInContext(visitor->ctx->context), char_node.value, 0);
}

LLVMValueRef visit_identifier_expr(CodegenVisitor* visitor, ASTNode* node)
{
    const char* name = node->as.identifier.name;

    if (name == NULL)
        return NULL;

    SymbolEntry* entry = lookup_symbol(visitor->ctx->symbol_table, name);
    if (entry == NULL) {
        printf("Codegen: Undefined variable '%s'\n", name);
        return NULL;
    }

    if (entry->symbol_data.kind != SYMBOL_VARIABLE) {
        printf("Codegen: '%s' is not a variable\n", name);
        return NULL;
    }

    VariableSymbolData var_data = entry->symbol_data.as.variable;
    LLVMValueRef alloca = var_data.alloc;

    if (var_data.is_global) {
        return LLVMBuildLoad2(visitor->ctx->builder, LLVMGlobalGetValueType(alloca), alloca, "global_load");
    } else {
        return LLVMBuildLoad2(visitor->ctx->builder, LLVMGetAllocatedType(alloca), alloca, "local_load");
    }
}

LLVMValueRef visit_func_call_expr(CodegenVisitor* visitor, ASTNode* node)
{
    FuncCallNode func_call = node->as.func_call;
    LLVMValueRef args[func_call.arg_count];

    for (int i = 0; i < func_call.arg_count; i++)
    {
        args[i] = visit_expression(visitor, func_call.args[i]);
        if (args[i] == NULL) {
            printf("Codegen: Failed to generate argument %d for function call\n", i);
            return NULL;
        }
    }

    LLVMValueRef func = LLVMGetNamedFunction(visitor->ctx->module, func_call.name);
    if (func == NULL)
        return NULL;

    LLVMTypeRef func_type = LLVMGlobalGetValueType(func);
    if (func_type == NULL)
        return NULL;

    return LLVMBuildCall2(visitor->ctx->builder, func_type, func, args, func_call.arg_count, "func_call");
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

LLVMValueRef generate_cast_instruction(CodegenVisitor* visitor, LLVMValueRef value, LLVMTypeRef from_type, LLVMTypeRef to_type, const char* name)
{
    if (from_type == to_type) 
        return value;

    LLVMTypeKind from_kind = LLVMGetTypeKind(from_type);
    LLVMTypeKind to_kind = LLVMGetTypeKind(to_type);

    if (from_kind == LLVMPointerTypeKind && to_kind == LLVMPointerTypeKind)
        return LLVMBuildBitCast(visitor->ctx->builder, value, to_type, name);

    if (from_kind == LLVMIntegerTypeKind && to_kind == LLVMPointerTypeKind)
        return LLVMBuildIntToPtr(visitor->ctx->builder, value, to_type, name);

    if (from_kind == LLVMPointerTypeKind && to_kind == LLVMIntegerTypeKind)
        return LLVMBuildPtrToInt(visitor->ctx->builder, value, to_type, name);

    if (from_kind == LLVMFloatTypeKind && to_kind == LLVMDoubleTypeKind)
        return LLVMBuildFPExt(visitor->ctx->builder, value, to_type, name);

    if (from_kind == LLVMDoubleTypeKind && to_kind == LLVMFloatTypeKind)
        return LLVMBuildFPTrunc(visitor->ctx->builder, value, to_type, name);

    if (from_kind == LLVMIntegerTypeKind && (to_kind == LLVMFloatTypeKind || to_kind == LLVMDoubleTypeKind))
        return LLVMBuildSIToFP(visitor->ctx->builder, value, to_type, name);
    
    if ((from_kind == LLVMFloatTypeKind || from_kind == LLVMDoubleTypeKind) && to_kind == LLVMIntegerTypeKind)
        return LLVMBuildFPToSI(visitor->ctx->builder, value, to_type, name);

    return NULL;
}

LLVMValueRef visit_cast_expr(CodegenVisitor* visitor, ASTNode* node) {
    CastNode cast_node = node->as.cast;

    LLVMValueRef value = visit_expression(visitor, cast_node.expr);
    if (value == NULL) {
        printf("Codegen: Failed to generate expression for cast\n");
        return NULL;
    }

    LLVMTypeRef from_type = LLVMTypeOf(value);
    LLVMTypeRef to_type = token_type_to_llvm_type(visitor->ctx, cast_node.target_type.base_type);

    for (int i = 0; i < cast_node.target_type.pointer_level; i++) {
        to_type = LLVMPointerType(to_type, 0);
    }

    if (!are_types_compatible(from_type, to_type)) {
        printf("Codegen: Incompatible cast\n");
        return NULL;
    }

    return generate_cast_instruction(visitor, value, from_type, to_type, "cast_result");
}

LLVMValueRef visit_member_access_expr(CodegenVisitor* visitor, ASTNode* node) {
    return NULL;
}