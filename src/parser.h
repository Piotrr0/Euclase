#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast_layout.h"

typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
    AST_BLOCK,
    AST_PARAM_LIST,
    
    AST_RETURN,
    AST_VAR_DECL,
    AST_ASSIGN,
    AST_IF,
    AST_FOR,
    AST_WHILE,
    
    AST_IDENTIFIER,
    AST_STRING_LITERAL,
    AST_CHAR_LITERAL,
    AST_INT_LITERAL,
    AST_FLOAT_LITERAL,
    AST_DOUBLE_LITERAL,

    AST_EXPRESSION,
    AST_FUNC_CALL,
    AST_CAST,

    AST_BINARY_OP,
    AST_UNARY_OP,
    
    AST_STRUCT_DECL,
    AST_MEMBER_ACCESS
} ASTNodeType;

struct ASTNode {
    ASTNodeType type;
    int line;
    int column;

    union {
        ProgramNode program;
        FunctionNode function;
        ParamNode param;
        BlockNode block;
        StructDeclNode struct_decl;

        VarDeclNode var_decl;
        AssignNode assign;
        ReturnNode return_stmt;
        IfNode if_stmt;
        ForNode for_stmt;
        WhileNode while_stmt;

        FuncCallNode func_call;
        UnaryOpNode unary_op;
        BinaryOpNode binary_op;
        CastNode cast;
        MemberAccessNode member_access;
        IdentifierNode identifier;

        IntLiteralNode int_literal;
        FloatLiteralNode float_literal;
        DoubleLiteralNode double_literal;
        StringLiteralNode string_literal;
        CharLiteralNode char_literal;
    } as;
};

typedef struct Parser {
    Tokens* tokens;
    int current_token;
} Parser;

extern Parser parser;
void init_parser(Tokens* tokens);

void advance();
int match(TokenType type);
int check(TokenType type);

Token* current_token();
Token* peek_token(int offset);

void free_ast(ASTNode* node);

ASTNode* parse_pre_increment();
ASTNode* parse_pre_decrement();
ASTNode* parse_post_increment(ASTNode* operand);
ASTNode* parse_post_decrement(ASTNode* operand);

ASTNode* parse_compound_operators();
ASTNode* parse_expression();
ASTNode* parse_equality();
ASTNode* parse_additive();
ASTNode* parse_multiplicative();
ASTNode* parse_unary();
ASTNode* parse_postfix();
ASTNode* parse_primary();
ASTNode* parse_parens();
ASTNode* parse_primary_expression();
ASTNode* parse_number_literal();
ASTNode* parse_string_literal();
ASTNode* parse_char_literal();
ASTNode* parse_identifier_expression();
ASTNode* parse_dereference();
ASTNode* parse_address_of();
ASTNode* parse_function_call();
ASTNode* parse_casting();
TypeInfo parse_type();
ASTNode* parse_while_loop();
ASTNode* parse_for_loop();
ASTNode* parse_loop_init();
ASTNode* parse_loop_condition();
ASTNode* parse_loop_update();
ASTNode* parse_if();
ASTNode* parse_else();
ASTNode* parse_assignment();
ASTNode* parse_variable_declaration();
char* parse_struct_name();
ASTNode* parse_struct_declaration();
ASTNode* parse_struct_member();
ASTNode* parse_return();
ASTNode* parse_statement();
ASTNode* parse_block();
int check_pointer_level(int offset);
ASTNode* parse_parameters(ASTNode* func);
ASTNode* parse_function();
char* parse_namespace_name();
ASTNode* parse_program();

int is_func_call();
int is_casting();
int is_func_declaration();
int is_compound_token(TokenType type);
int parse_pointer_level();
int is_type(TokenType t);

void print_ast(ASTNode* node, int indent);

#endif