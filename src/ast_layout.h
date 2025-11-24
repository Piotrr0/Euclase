#ifndef AST_LAYOUT_H
#define AST_LAYOUT_H

#include "token.h"

typedef struct ASTNode ASTNode;

typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_EQ,
    OP_NE,
    OP_LT,
    OP_GT,
    OP_LE,
    OP_GE
} BinaryOp;

typedef enum {
    OP_ADDR,
    OP_DEREF,
    OP_NEG,
    OP_PRE_INC,
    OP_PRE_DEC,
    OP_POST_INC,
    OP_POST_DEC,
} UnaryOP;

typedef struct TypeInfo {
    TokenType base_type;
    char* type;
    int pointer_level;
} TypeInfo;


typedef struct {
    char* name;

    ASTNode** functions;
    int function_count;

    ASTNode** structs;
    int struct_count;
    
    ASTNode** globals;
    int global_count;
} ProgramNode;

typedef struct {
    char* name;
    TypeInfo return_type;

    ASTNode** params;
    int param_count;

    ASTNode* body;
} FunctionNode;

typedef struct {
    ASTNode** statements;
    int statement_count;
} BlockNode;

typedef struct {
    char* name;
    TypeInfo type;
} ParamNode;

typedef struct {
    ASTNode* value;
} ReturnNode;

typedef struct {
    char* name;
    TypeInfo type;
    ASTNode* initializer;
} VarDeclNode;

typedef struct {
    ASTNode* target;
    ASTNode* value;
} AssignNode;

typedef struct {
    ASTNode* condition;
    ASTNode* then_branch;
    ASTNode* else_branch;
} IfNode;

typedef struct {
    ASTNode* init;
    ASTNode* condition;
    ASTNode* update;
    ASTNode* body;
} ForNode;

typedef struct {
    ASTNode* condition;
    ASTNode* body;
} WhileNode;

typedef struct {
    char* name;
} IdentifierNode;

typedef struct {
    long long value;
} IntLiteralNode;

typedef struct {
    float value;
} FloatLiteralNode;

typedef struct {
    double value;
} DoubleLiteralNode;

typedef struct {
    char* value;
    int length;
} StringLiteralNode;

typedef struct {
    char value;
} CharLiteralNode;

typedef struct {
    char* name;
    
    ASTNode** args;
    int arg_count;
} FuncCallNode;

typedef struct {
    UnaryOP op;
    ASTNode* operand;
} UnaryOpNode;

typedef struct {
    BinaryOp op;
    ASTNode* left;
    ASTNode* right;
} BinaryOpNode;

typedef struct {
    TypeInfo target_type;
    ASTNode* expr;
} CastNode;

typedef struct {
    ASTNode* object;
    char* member;
} MemberAccessNode;

typedef struct {
    char* type;

    ASTNode** members;
    int member_count;
} StructDeclNode;

ASTNode* create_program_node(char* name);
ASTNode* create_function_node(char* name, TypeInfo return_type);
ASTNode* create_block_node();
ASTNode* create_param_node(char* name, TypeInfo type);
ASTNode* create_return_node(ASTNode* value);
ASTNode* create_var_decl_node(char* name, TypeInfo type, ASTNode* initializer);
ASTNode* create_assign_node(ASTNode* target, ASTNode* value);
ASTNode* create_if_node(ASTNode* condition, ASTNode* then_branch, ASTNode* else_branch);
ASTNode* create_for_node(ASTNode* init, ASTNode* condition, ASTNode* increment, ASTNode* body);
ASTNode* create_while_node(ASTNode* condition, ASTNode* body);
ASTNode* create_identifier_node(char* name);
ASTNode* create_int_literal_node(long long value);
ASTNode* create_float_literal_node(float value);
ASTNode* create_double_literal_node(double value);
ASTNode* create_string_literal_node(char* value);
ASTNode* create_char_literal_node(char value);
ASTNode* create_func_call_node(char* name);
ASTNode* create_unary_op_node(UnaryOP op, ASTNode* operand);
ASTNode* create_binary_op_node(BinaryOp op, ASTNode* left, ASTNode* right);
ASTNode* create_cast_node(TypeInfo target_type, ASTNode* expr);
ASTNode* create_member_access_node(ASTNode* object, char* member);
ASTNode* create_struct_decl_node(char* type);

void add_param_to_function(ASTNode* func, ASTNode* param);
void add_member_to_struct(ASTNode* struct_decl, ASTNode* member);
void add_arg_to_func_call(ASTNode* func_call, ASTNode* arg);
void add_statement_to_block(ASTNode* block, ASTNode* stmt);
void add_function_to_program(ASTNode* program, ASTNode* function);
void add_struct_to_program(ASTNode* program, ASTNode* struct_decl);
void add_global_to_program(ASTNode* program, ASTNode* global);

#endif