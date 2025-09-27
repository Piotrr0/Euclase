#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
    AST_BLOCK,
    AST_RETURN,
    AST_EXPRESSION,
    AST_VAR_DECL,
    AST_ASSIGN
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    char* name;

    TokenType decl_type;
    Value value;

    struct ASTNode** children;
    int child_count;
} ASTNode;


ASTNode* new_node(ASTNodeType type);
void add_child(ASTNode* parent, ASTNode* child);
void advance();
int match(TokenType type);

int is_func_declaration();

ASTNode* parse_expression();
ASTNode* parse_statement();
ASTNode* parse_block();
ASTNode* parse_function();
ASTNode* parse_program();
void print_ast(ASTNode* node, int indent);


#endif