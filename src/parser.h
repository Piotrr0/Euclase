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

void init_parser(Parser* parser, Tokens* tokens);

void advance(Parser* parser);
int match(Parser* parser, TokenType type);
int check(Parser* parser, TokenType type);

Token* current_token(Parser* parser);
Token* peek_token(Parser* parser, int offset);

ASTNode* parse_pre_increment(Parser* parser);
ASTNode* parse_pre_decrement(Parser* parser);
ASTNode* parse_post_increment(Parser* parser, ASTNode* operand);
ASTNode* parse_post_decrement(Parser* parser, ASTNode* operand);

ASTNode* parse_compound_operators(Parser* parser);
ASTNode* parse_expression(Parser* parser);
ASTNode* parse_equality(Parser* parser);
ASTNode* parse_additive(Parser* parser);
ASTNode* parse_multiplicative(Parser* parser);
ASTNode* parse_unary(Parser* parser);
ASTNode* parse_postfix(Parser* parser);
ASTNode* parse_primary(Parser* parser);
ASTNode* parse_parens(Parser* parser);
ASTNode* parse_primary_expression(Parser* parser);
ASTNode* parse_number_literal(Parser* parser);
ASTNode* parse_string_literal(Parser* parser);
ASTNode* parse_char_literal(Parser* parser);
ASTNode* parse_identifier_expression(Parser* parsern);
ASTNode* parse_dereference(Parser* parser);
ASTNode* parse_address_of(Parser* parser);
ASTNode* parse_function_call(Parser* parser);
ASTNode* parse_casting(Parser* parser);
TypeInfo parse_type(Parser* parser);
ASTNode* parse_while_loop(Parser* parser);
ASTNode* parse_for_loop(Parser* parser);
ASTNode* parse_loop_init(Parser* parser);
ASTNode* parse_loop_condition(Parser* parser);
ASTNode* parse_loop_update(Parser* parser);
ASTNode* parse_if(Parser* parser);
ASTNode* parse_else(Parser* parser);
ASTNode* parse_assignment(Parser* parser);
ASTNode* parse_variable_declaration(Parser* parser);
char* parse_struct_name(Parser* parser);
ASTNode* parse_struct_declaration(Parser* parser);
ASTNode* parse_struct_member(Parser* parser);
ASTNode* parse_return(Parser* parser);
ASTNode* parse_statement(Parser* parser);
ASTNode* parse_block(Parser* parser);
int check_pointer_level(Parser* parser, int offset);
ASTNode* parse_parameters(Parser* parser, ASTNode* func);
ASTNode* parse_function(Parser* parser);
char* parse_namespace_name(Parser* parser);
ASTNode* parse_program(Parser* parser);

int is_func_call(Parser* parser);
int is_casting(Parser* parser);
int is_func_declaration(Parser* parser);
int parse_pointer_level(Parser* parser);
int is_type(Parser* parser, TokenType t);
int is_compound_token(TokenType type);


void free_ast(ASTNode* node);
void print_ast(ASTNode* node, int indent);

#endif