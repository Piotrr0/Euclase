#ifndef AST_LAYOUT_H
#define AST_LAYOUT_H

#include "lexer.h"

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
    char* name;

    ASTNode** members;
    int member_count;
} StructDeclNode;

#endif