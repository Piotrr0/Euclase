#include "codegen_stmt_visitor.h"
#include <stdio.h>

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
    
    if (lhs->type == AST_IDENTIFIER) {
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
    else if (lhs->type == AST_UNARY_OP && lhs->as.unary_op.op == OP_DEREF) {
        UnaryOpNode unary_node = lhs->as.unary_op;
        LLVMValueRef ptr = visit_expression(visitor, unary_node.operand);
        if (ptr == NULL) 
            return;

        LLVMBuildStore(visitor->ctx->builder, new_val, ptr);
    }
}