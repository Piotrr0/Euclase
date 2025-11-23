#include "codegen_expr_visitor.h"
#include <stdio.h>

int does_type_kind_match(LLVMValueRef left, LLVMValueRef right, LLVMTypeKind* out_kind) {
    if (left == NULL || right == NULL)
        return 0;

    LLVMTypeRef left_type = LLVMTypeOf(left);
    LLVMTypeKind left_kind = LLVMGetTypeKind(left_type);

    LLVMTypeRef right_type = LLVMTypeOf(right);
    LLVMTypeKind right_kind = LLVMGetTypeKind(right_type);

    if (left_kind == right_kind) {
        *out_kind = left_kind;
        return 1;
    }

    return 0;
}

LLVMValueRef build_addition(CodegenVisitor* visitor, LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type) {
    if (type == LLVMFloatTypeKind || type == LLVMDoubleTypeKind)
        return LLVMBuildFAdd(visitor->ctx->builder, left, right, "fadd");
    else
        return LLVMBuildAdd(visitor->ctx->builder, left, right, "add");
}

LLVMValueRef build_subtraction(CodegenVisitor* visitor, LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type) {
    if (type == LLVMFloatTypeKind || type == LLVMDoubleTypeKind)
        return LLVMBuildFSub(visitor->ctx->builder, left, right, "fsub");
    else
        return LLVMBuildSub(visitor->ctx->builder, left, right, "sub");
}

LLVMValueRef build_multiplication(CodegenVisitor* visitor, LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type) {
    if (type == LLVMFloatTypeKind || type == LLVMDoubleTypeKind)
        return LLVMBuildFMul(visitor->ctx->builder, left, right, "fmul");
    else
        return LLVMBuildMul(visitor->ctx->builder, left, right, "mul");
}

LLVMValueRef build_division(CodegenVisitor* visitor, LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type) {
    if (type == LLVMFloatTypeKind || type == LLVMDoubleTypeKind)
        return LLVMBuildFDiv(visitor->ctx->builder, left, right, "fdiv");
    else
        return LLVMBuildSDiv(visitor->ctx->builder, left, right, "sdiv");
}

LLVMValueRef build_modulo(CodegenVisitor* visitor, LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type) {
    if (type == LLVMFloatTypeKind || type == LLVMDoubleTypeKind)
        return LLVMBuildFRem(visitor->ctx->builder, left, right, "frem");
    else
        return LLVMBuildSRem(visitor->ctx->builder, left, right, "srem");
}

LLVMValueRef build_compare(CodegenVisitor* visitor, LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type, int is_not_equal) {
    if (type == LLVMFloatTypeKind || type == LLVMDoubleTypeKind)
        return LLVMBuildFCmp(visitor->ctx->builder, is_not_equal ? LLVMRealONE : LLVMRealOEQ, left, right, "fcmp");
    else
        return LLVMBuildICmp(visitor->ctx->builder, is_not_equal ? LLVMIntNE : LLVMIntEQ, left, right, "icmp");
}

LLVMValueRef build_greater(CodegenVisitor* visitor, LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type) {
    if (type == LLVMFloatTypeKind || type == LLVMDoubleTypeKind)
        return LLVMBuildFCmp(visitor->ctx->builder, LLVMRealOGT, left, right, "fgr");
    else
        return LLVMBuildICmp(visitor->ctx->builder, LLVMIntSGT, left, right, "lgr");
}

LLVMValueRef build_less(CodegenVisitor* visitor, LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type) {
    if (type == LLVMFloatTypeKind || type == LLVMDoubleTypeKind)
        return LLVMBuildFCmp(visitor->ctx->builder, LLVMRealOLT, left, right, "flt");
    else
        return LLVMBuildICmp(visitor->ctx->builder, LLVMIntSLT, left, right, "llt");
}

LLVMValueRef build_less_equal(CodegenVisitor* visitor, LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type) {
    if (type == LLVMFloatTypeKind || type == LLVMDoubleTypeKind)
        return LLVMBuildFCmp(visitor->ctx->builder, LLVMRealOLE, left, right, "fle");
    else
        return LLVMBuildICmp(visitor->ctx->builder, LLVMIntSLE, left, right, "lle");
}

LLVMValueRef build_greater_equal(CodegenVisitor* visitor, LLVMValueRef left, LLVMValueRef right, LLVMTypeKind type) {
    if (type == LLVMFloatTypeKind || type == LLVMDoubleTypeKind)
        return LLVMBuildFCmp(visitor->ctx->builder, LLVMRealOGE, left, right, "fge");
    else
        return LLVMBuildICmp(visitor->ctx->builder, LLVMIntSGE, left, right, "lle");
}

LLVMValueRef visit_binary_op_expr(CodegenVisitor* visitor, ASTNode* node)
{
    BinaryOpNode binary_node = node->as.binary_op;

    LLVMValueRef left = visit_expression(visitor, binary_node.left);
    LLVMValueRef right = visit_expression(visitor, binary_node.right);

    if (left == NULL || right == NULL) 
        return NULL;

    LLVMTypeKind type;
    if (!does_type_kind_match(left, right, &type))
        return NULL;

    switch (binary_node.op) {
        case OP_ADD: return build_addition(visitor, left, right, type);
        case OP_SUB: return build_subtraction(visitor, left, right, type);
        case OP_MUL: return build_multiplication(visitor, left, right, type);
        case OP_DIV: return build_division(visitor, left, right, type);
        case OP_MOD: return build_modulo(visitor, left, right, type);
        case OP_EQ:  return build_compare(visitor, left, right, type, 0);
        case OP_NE:  return build_compare(visitor, left, right, type, 1);
        case OP_LT:  return build_less(visitor, left, right, type);
        case OP_GT:  return build_greater(visitor, left, right, type);
        case OP_LE:  return build_less_equal(visitor, left, right, type);
        case OP_GE:  return build_greater_equal(visitor, left, right, type);
        default:     return NULL;
    }
}

LLVMValueRef visit_negation(CodegenVisitor* visitor, UnaryOpNode node)
{
    LLVMValueRef expression = visit_expression(visitor, node.operand);
    if (expression == NULL)
        return NULL;

    LLVMTypeRef type = LLVMTypeOf(expression);
    LLVMTypeKind type_kind = LLVMGetTypeKind(type);

    if (type_kind == LLVMFloatTypeKind || type_kind == LLVMDoubleTypeKind)
        return LLVMBuildFNeg(visitor->ctx->builder, expression, "fneg");
    else if (type_kind == LLVMIntegerTypeKind)
        return LLVMBuildNeg(visitor->ctx->builder, expression, "neg");

    return NULL;
}

LLVMValueRef visit_address_of(CodegenVisitor* visitor, UnaryOpNode node)
{
    ASTNode* addressed_node = node.operand;
    if (addressed_node->type != AST_IDENTIFIER) 
        return NULL;
    
    SymbolEntry* entry = lookup_symbol(visitor->ctx->symbol_table, addressed_node->as.identifier.name);
    if (entry == NULL || entry->symbol_data.kind != SYMBOL_VARIABLE)
        return NULL;
    
    return entry->symbol_data.as.variable.alloc;
}

LLVMValueRef visit_dereference(CodegenVisitor* visitor, UnaryOpNode node)
{
    ASTNode* ptr_expr = node.operand;
    if (ptr_expr->type != AST_IDENTIFIER) 
        return NULL;

    SymbolEntry* entry = lookup_symbol(visitor->ctx->symbol_table, ptr_expr->as.identifier.name);
    if (entry == NULL || entry->symbol_data.kind != SYMBOL_VARIABLE) 
        return NULL;

    VariableSymbolData var_data = entry->symbol_data.as.variable;
    LLVMValueRef alloca = var_data.alloc;

    if (alloca == NULL)
        return NULL;

    if (var_data.type.pointer_level <= 0) {
        printf("Codegen: Cannot dereference non-pointer variable '%s'\n", ptr_expr->as.identifier.name);
        return NULL;
    }

    LLVMValueRef ptr_val;
    if (var_data.is_global)
        ptr_val = LLVMBuildLoad2(visitor->ctx->builder, LLVMGlobalGetValueType(alloca), alloca, "ptr_load");
    else
        ptr_val = LLVMBuildLoad2(visitor->ctx->builder, LLVMGetAllocatedType(alloca), alloca, "ptr_load");

    LLVMTypeRef pointed_type = token_type_to_llvm_type(visitor->ctx, var_data.type.base_type);
    for (int j = 0; j < var_data.type.pointer_level - 1; j++) {
        pointed_type = LLVMPointerType(pointed_type, 0);
    }

    return LLVMBuildLoad2(visitor->ctx->builder, pointed_type, ptr_val, "deref");
}

LLVMValueRef visit_pre_inc(CodegenVisitor* visitor, UnaryOpNode node)
{
    if (node.operand->type != AST_IDENTIFIER) 
        return NULL;

    const char* var_name = node.operand->as.identifier.name;
    SymbolEntry* entry = lookup_symbol(visitor->ctx->symbol_table, var_name);

    if (entry == NULL || entry->symbol_data.kind != SYMBOL_VARIABLE) 
        return NULL;

    LLVMValueRef alloca = entry->symbol_data.as.variable.alloc;
    LLVMTypeRef type = LLVMGetAllocatedType(alloca);

    LLVMValueRef value = LLVMBuildLoad2(visitor->ctx->builder, type, alloca, "load");

    LLVMTypeKind kind = LLVMGetTypeKind(type);
    LLVMValueRef one;
    if (kind == LLVMIntegerTypeKind)
        one = LLVMConstInt(type, 1, 0);
    else if (kind == LLVMFloatTypeKind || kind == LLVMDoubleTypeKind)
        one = LLVMConstReal(type, 1.0);

    LLVMValueRef result = build_addition(visitor, value, one, kind);
    LLVMBuildStore(visitor->ctx->builder, result, alloca);

    return result;
}

LLVMValueRef visit_pre_dec(CodegenVisitor* visitor, UnaryOpNode node)
{
    if (node.operand->type != AST_IDENTIFIER) 
        return NULL;

    const char* var_name = node.operand->as.identifier.name;
    SymbolEntry* entry = lookup_symbol(visitor->ctx->symbol_table, var_name);

    if (entry == NULL || entry->symbol_data.kind != SYMBOL_VARIABLE) 
        return NULL;

    LLVMValueRef alloca = entry->symbol_data.as.variable.alloc;
    LLVMTypeRef type = LLVMGetAllocatedType(alloca);

    LLVMValueRef value = LLVMBuildLoad2(visitor->ctx->builder, type, alloca, "load");

    LLVMTypeKind kind = LLVMGetTypeKind(type);
    LLVMValueRef one;
    if (kind == LLVMIntegerTypeKind)
        one = LLVMConstInt(type, 1, 0);
    else if (kind == LLVMFloatTypeKind || kind == LLVMDoubleTypeKind)
        one = LLVMConstReal(type, 1.0);

    LLVMValueRef result = build_subtraction(visitor, value, one, kind);
    LLVMBuildStore(visitor->ctx->builder, result, alloca);

    return result;
}

LLVMValueRef visit_post_inc(CodegenVisitor* visitor, UnaryOpNode node)
{
    if (node.operand->type != AST_IDENTIFIER) 
        return NULL;

    const char* var_name = node.operand->as.identifier.name;
    SymbolEntry* entry = lookup_symbol(visitor->ctx->symbol_table, var_name);

    if (entry == NULL || entry->symbol_data.kind != SYMBOL_VARIABLE) 
        return NULL;

    LLVMValueRef alloca = entry->symbol_data.as.variable.alloc;
    LLVMTypeRef type = LLVMGetAllocatedType(alloca);

    LLVMValueRef original_value = LLVMBuildLoad2(visitor->ctx->builder, type, alloca, "post_inc_load");

    LLVMTypeKind kind = LLVMGetTypeKind(type);
    LLVMValueRef one;
    if (kind == LLVMIntegerTypeKind)
        one = LLVMConstInt(type, 1, 0);
    else if (kind == LLVMFloatTypeKind || kind == LLVMDoubleTypeKind)
        one = LLVMConstReal(type, 1.0);

    LLVMValueRef incremented = build_addition(visitor, original_value, one, kind);
    LLVMBuildStore(visitor->ctx->builder, incremented, alloca);

    return original_value;
}

LLVMValueRef visit_post_dec(CodegenVisitor* visitor, UnaryOpNode node)
{
    if (node.operand->type != AST_IDENTIFIER) 
        return NULL;

    const char* var_name = node.operand->as.identifier.name;
    SymbolEntry* entry = lookup_symbol(visitor->ctx->symbol_table, var_name);

    if (entry == NULL || entry->symbol_data.kind != SYMBOL_VARIABLE) 
        return NULL;

    LLVMValueRef alloca = entry->symbol_data.as.variable.alloc;
    LLVMTypeRef type = LLVMGetAllocatedType(alloca);

    LLVMValueRef original_value = LLVMBuildLoad2(visitor->ctx->builder, type, alloca, "post_dec_load");

    LLVMTypeKind kind = LLVMGetTypeKind(type);
    LLVMValueRef one;
    if (kind == LLVMIntegerTypeKind)
        one = LLVMConstInt(type, 1, 0);
    else if (kind == LLVMFloatTypeKind || kind == LLVMDoubleTypeKind)
        one = LLVMConstReal(type, 1.0);

    LLVMValueRef decremented = build_subtraction(visitor, original_value, one, kind);
    LLVMBuildStore(visitor->ctx->builder, decremented, alloca);

    return original_value;
}

LLVMValueRef visit_unary_op_expr(CodegenVisitor* visitor, ASTNode* node)
{
    UnaryOpNode unary_node = node->as.unary_op;

    switch (unary_node.op) {
        case OP_ADDR:       return visit_address_of(visitor, unary_node);
        case OP_DEREF:      return visit_dereference(visitor, unary_node);
        case OP_NEG:        return visit_negation(visitor, unary_node);
        case OP_PRE_INC:    return visit_pre_inc(visitor, unary_node);
        case OP_PRE_DEC:    return visit_pre_dec(visitor, unary_node);
        case OP_POST_INC:   return visit_post_inc(visitor, unary_node);
        case OP_POST_DEC:   return visit_post_dec(visitor, unary_node);
        default: break;
    }
    return NULL;
}