#include "codegen_stmt_visitor.h"
#include <stdio.h>
#include <string.h>

void visit_return_stmt(CodegenVisitor* visitor, ASTNode* node)
{
    ReturnNode return_node = node->as.return_stmt;

    if (return_node.value == NULL) {
        LLVMBuildRetVoid(visitor->ctx->builder);
        return;
    }

    LLVMValueRef return_value = visit_expression(visitor, return_node.value);
    if (return_value == NULL)
        LLVMBuildRetVoid(visitor->ctx->builder);
    else
        LLVMBuildRet(visitor->ctx->builder, return_value);
}

void visit_if_stmt(CodegenVisitor* visitor, ASTNode* node)
{
    IfNode if_node = node->as.if_stmt;

    LLVMValueRef condition = visit_expression(visitor, if_node.condition);
    LLVMBasicBlockRef thenBB = LLVMAppendBasicBlock(visitor->ctx->current_function, "then");
    LLVMBasicBlockRef mergeBB = LLVMAppendBasicBlock(visitor->ctx->current_function, "if_cont");

    LLVMBasicBlockRef elseBB = NULL;
    if (if_node.else_branch)
    {
        elseBB = LLVMAppendBasicBlock(visitor->ctx->current_function, "else");
        LLVMBuildCondBr(visitor->ctx->builder, condition, thenBB, elseBB);
    } 
    else {
        LLVMBuildCondBr(visitor->ctx->builder, condition, thenBB, mergeBB);
    }

    LLVMPositionBuilderAtEnd(visitor->ctx->builder, thenBB);
    visit_block_stmt(visitor, if_node.then_branch);

    LLVMBasicBlockRef current_block = LLVMGetInsertBlock(visitor->ctx->builder);
    if (LLVMGetBasicBlockTerminator(current_block) == NULL)
        LLVMBuildBr(visitor->ctx->builder, mergeBB);

    if (if_node.else_branch != NULL) {
        LLVMPositionBuilderAtEnd(visitor->ctx->builder, elseBB);
        visit_block_stmt(visitor, if_node.else_branch);
        
        current_block = LLVMGetInsertBlock(visitor->ctx->builder);
        if (LLVMGetBasicBlockTerminator(current_block) == NULL)
            LLVMBuildBr(visitor->ctx->builder, mergeBB);
    }

    LLVMPositionBuilderAtEnd(visitor->ctx->builder, mergeBB);
}

void visit_while_stmt(CodegenVisitor* visitor, ASTNode* node)
{
    WhileNode while_node = node->as.while_stmt;

    LLVMBasicBlockRef condBB = LLVMAppendBasicBlock(visitor->ctx->current_function, "while_cond");
    LLVMBasicBlockRef bodyBB = LLVMAppendBasicBlock(visitor->ctx->current_function, "while_body");
    LLVMBasicBlockRef afterBB = LLVMAppendBasicBlock(visitor->ctx->current_function, "while_after");

    LLVMBuildBr(visitor->ctx->builder, condBB);

    LLVMPositionBuilderAtEnd(visitor->ctx->builder, condBB);
    LLVMValueRef cond_val = visit_expression(visitor, while_node.condition);
    LLVMBuildCondBr(visitor->ctx->builder, cond_val, bodyBB, afterBB);

    LLVMPositionBuilderAtEnd(visitor->ctx->builder, bodyBB);
    visit_block_stmt(visitor, while_node.body);

    if (LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(visitor->ctx->builder)) == NULL)
        LLVMBuildBr(visitor->ctx->builder, condBB);

    LLVMPositionBuilderAtEnd(visitor->ctx->builder, afterBB);
}

void visit_for_stmt(CodegenVisitor* visitor, ASTNode* node)
{
    ForNode for_node = node->as.for_stmt;
    push_scope(visitor->ctx->symbol_table);

    if (for_node.init != NULL)
        visit_statement(visitor, for_node.init);

    LLVMBasicBlockRef updBB = LLVMAppendBasicBlock(visitor->ctx->current_function, "for_upd");
    LLVMBasicBlockRef condBB = LLVMAppendBasicBlock(visitor->ctx->current_function, "for_cond");
    LLVMBasicBlockRef bodyBB = LLVMAppendBasicBlock(visitor->ctx->current_function, "for_body");
    LLVMBasicBlockRef afterBB = LLVMAppendBasicBlock(visitor->ctx->current_function, "for_after");

    LLVMBuildBr(visitor->ctx->builder, condBB);
    LLVMPositionBuilderAtEnd(visitor->ctx->builder, condBB);
    LLVMValueRef cond_val = visit_expression(visitor, for_node.condition);
    LLVMBuildCondBr(visitor->ctx->builder, cond_val, bodyBB, afterBB);

    LLVMPositionBuilderAtEnd(visitor->ctx->builder, bodyBB);
    visit_block_stmt(visitor, for_node.body);
    if (LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(visitor->ctx->builder)) == NULL)
        LLVMBuildBr(visitor->ctx->builder, updBB);

    LLVMPositionBuilderAtEnd(visitor->ctx->builder, updBB);
    if (for_node.update != NULL)
        visit_statement(visitor, for_node.update);

    LLVMBuildBr(visitor->ctx->builder, condBB);

    LLVMPositionBuilderAtEnd(visitor->ctx->builder, afterBB);
    pop_scope(visitor->ctx->symbol_table);
}

void visit_block_stmt(CodegenVisitor* visitor, ASTNode* node)
{
    BlockNode block_node = node->as.block;

    push_scope(visitor->ctx->symbol_table);
    for (int i = 0; i < block_node.statement_count; i++)
    {
        visit_statement(visitor, block_node.statements[i]);
    }
    pop_scope(visitor->ctx->symbol_table);
}

void assign_to_identifier(CodegenVisitor* visitor, ASTNode* lhs, LLVMValueRef new_val)
{
    const char* lhs_name = lhs->as.identifier.name;
    SymbolEntry* entry = lookup_symbol(visitor->ctx->symbol_table, lhs_name);

    if (entry == NULL) {
        printf("Codegen: Undefined variable '%s'\n", lhs_name);
        return;
    }

    if (entry->symbol_data.kind != SYMBOL_VARIABLE) {
        printf("Codegen: '%s' is not a variable\n", lhs_name);
        return;
    }

    LLVMBuildStore(visitor->ctx->builder, new_val, entry->symbol_data.as.variable.alloc);
}

void assign_to_dereference(CodegenVisitor* visitor, ASTNode* lhs, LLVMValueRef new_val)
{
    UnaryOpNode unary_node = lhs->as.unary_op;
    LLVMValueRef ptr = visit_expression(visitor, unary_node.operand);
    if (ptr == NULL)
        return;

    LLVMBuildStore(visitor->ctx->builder, new_val, ptr);
}

void assign_to_member_access(CodegenVisitor* visitor, ASTNode* lhs, LLVMValueRef new_val)
{
    MemberAccessNode access_node = lhs->as.member_access;
    if (access_node.object == NULL)
        return;
    
    const char* member_name = access_node.member;
    if (member_name == NULL && access_node.object->type != AST_IDENTIFIER) {
        return;
    }

    const char* var_name = access_node.object->as.identifier.name;
    SymbolEntry* var_entry = lookup_symbol(visitor->ctx->symbol_table, var_name);
    TypeInfo var_type = var_entry->symbol_data.as.variable.type;
    if (var_entry == NULL || var_entry->symbol_data.kind != SYMBOL_VARIABLE) {
        return;
    }

    const char* struct_type_name = var_type.type;
    LLVMValueRef struct_ptr = var_entry->symbol_data.as.variable.alloc;
    if (struct_ptr == NULL)
        return;

    LLVMTypeRef struct_type = lookup_struct_type(visitor->ctx->symbol_table, struct_type_name);
    if (struct_type == NULL)
        return;

    int member_index = get_struct_member_index(visitor->ctx->symbol_table, struct_type_name, member_name);
    if (member_index < 0)
        return;

    LLVMValueRef indices[2];
    indices[0] = LLVMConstInt(LLVMInt32TypeInContext(visitor->ctx->context), 0, 0);
    indices[1] = LLVMConstInt(LLVMInt32TypeInContext(visitor->ctx->context), member_index, 0);

    LLVMValueRef member_ptr = LLVMBuildGEP2(
        visitor->ctx->builder,
        struct_type,
        struct_ptr,
        indices,
        2,
        "member_ptr"
    );

    LLVMBuildStore(visitor->ctx->builder, new_val, member_ptr);
}

void visit_assign_stmt(CodegenVisitor* visitor, ASTNode* node)
{
    AssignNode assign_node = node->as.assign;
    ASTNode* lhs = assign_node.target;
    ASTNode* rhs = assign_node.value;

    if (lhs == NULL || rhs == NULL) 
        return;

    LLVMValueRef new_val = visit_expression(visitor, rhs);
    if (new_val == NULL)
        return;

    switch (lhs->type) {
        case AST_IDENTIFIER:    assign_to_identifier(visitor, lhs, new_val); break;
        case AST_UNARY_OP:
            if (lhs->as.unary_op.op == OP_DEREF)
                assign_to_dereference(visitor, lhs, new_val);
            break;

        case AST_MEMBER_ACCESS: assign_to_member_access(visitor, lhs, new_val); break;
        default: break;
    }
}

void visit_print_stmt(CodegenVisitor* visitor, ASTNode* node) {
    LLVMValueRef value_to_print = visit_expression(visitor, node->as.print.expression);
    if (value_to_print == NULL)
        return;
    
    LLVMTypeRef type = LLVMTypeOf(value_to_print);
    LLVMTypeKind type_kind = LLVMGetTypeKind(type);
    
    const char* format = "%d\n";
    if (type_kind == LLVMIntegerTypeKind) {
        format = "%d\n";
    }
    else if (type_kind == LLVMPointerTypeKind) {
        format = "%s\n";
    }
    else if (type_kind == LLVMDoubleTypeKind) {
        format = "%f\n";
    }
    else if (type_kind == LLVMFloatTypeKind) {
        value_to_print = LLVMBuildFPExt(visitor->ctx->builder, value_to_print, LLVMDoubleTypeInContext(visitor->ctx->context), "promote_float");
        format = "%f\n";
    }

    LLVMValueRef format_str = LLVMBuildGlobalStringPtr(visitor->ctx->builder, format, "print_fmt");

    LLVMValueRef args[] = { format_str, value_to_print };
    LLVMValueRef printf_func = get_printf_func(visitor->ctx);

    LLVMTypeRef param_types[] = { LLVMPointerType(LLVMInt8TypeInContext(visitor->ctx->context), 0) };
    LLVMTypeRef printf_type = LLVMFunctionType(LLVMInt32TypeInContext(visitor->ctx->context), param_types, 1, 1);

    LLVMBuildCall2(
        visitor->ctx->builder, 
        LLVMGetElementType(LLVMTypeOf(printf_func)), 
        printf_func, 
        args, 
        2, 
        ""
    );
}