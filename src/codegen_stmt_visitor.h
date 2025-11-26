#ifndef CODEGEN_STMT_VISITOR_H
#define CODEGEN_STMT_VISITOR_H

#include "codegen_visitor.h"

void visit_return_stmt(CodegenVisitor* visitor, ASTNode* node);
void visit_if_stmt(CodegenVisitor* visitor, ASTNode* node);
void visit_while_stmt(CodegenVisitor* visitor, ASTNode* node);
void visit_for_stmt(CodegenVisitor* visitor, ASTNode* node);
void visit_block_stmt(CodegenVisitor* visitor, ASTNode* node);
void visit_assign_stmt(CodegenVisitor* visitor, ASTNode* node);

#endif