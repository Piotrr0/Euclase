#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
    AST_BLOCK,
    AST_RETURN,
    AST_IDENTIFIER,
    AST_EXPRESSION,
    AST_VAR_DECL,
    AST_ASSIGN,
    AST_DEREFERENCE,
    AST_ADDRESS_OF,
    AST_CAST,
    AST_PARAM_LIST,
    AST_FUNC_CALL,
    AST_ADDITION,
    AST_SUBTRACTION,
    AST_MULTIPLICATION,
    AST_DIVISION,
    AST_MODULO
} ASTNodeType;

typedef struct TypeInfo {
    TokenType base_type;
    int pointer_level;
} TypeInfo;

typedef struct ASTNode {
    ASTNodeType type;
    char* name;

    TypeInfo type_info;
    Value value;

    struct ASTNode** children;
    int child_count;
} ASTNode;


ASTNode* new_node(ASTNodeType type);
void add_child(ASTNode* parent, ASTNode* child);
void free_ast(ASTNode* node);

void advance();
int match(TokenType type);
int is_type(TokenType t);
int parse_pointer_level();


ASTNode* parse_expression();
ASTNode* parse_additive();
ASTNode* parse_multiplicative();
ASTNode* parse_unary();
ASTNode* parse_postfix();
ASTNode* parse_primary();
ASTNode* parse_primary_expression();
ASTNode* parse_identifier_expression();
ASTNode* parse_dereference();
ASTNode* parse_address_of();
ASTNode* parse_function_call();
ASTNode* parse_casting();
TypeInfo parse_type();
ASTNode* parse_assignment();
ASTNode* parse_variable_declaration();
ASTNode* parse_return();
ASTNode* parse_statement();
ASTNode* parse_block();
int check_pointer_level(int offset);
ASTNode* parse_parameters();
ASTNode* parse_function();
ASTNode* parse_namespace(ASTNodeType type);
ASTNode* parse_program();

int is_func_call();
int is_casting();
int is_func_declaration();
void print_ast(ASTNode* node, int indent);

#endif