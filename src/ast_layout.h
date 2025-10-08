#ifndef AST_LAYOUT_H
#define AST_LAYOUT_H

typedef enum {
    FUNC_PARAMS = 0,
    FUNC_BODY = 1
} ASTFuncChild;

typedef enum {
    ASSIGN_TARGET = 0,
    ASSIGN_VALUE = 1
} ASTAssignChild;

typedef enum {
    VAR_DECL_INITIALIZER = 0
} ASTVarDeclChild;

typedef enum {
    UNARY_OPERAND = 0
} ASTUnaryChild;

typedef enum {
    BIN_LEFT = 0,
    BIN_RIGHT = 1
} ASTBinaryChild;

typedef enum {
    IF_CONDITION = 0,
    IF_THEN_BLOCK = 1,
    IF_ELSE_BLOCK = 2
} ASTIfChild;

typedef enum {
    FOR_INIT = 0,
    FOR_CONDITION = 1,
    FOR_UPDATE = 2,
    FOR_BLOCK = 3
} ASTForChild;

typedef enum {
    WHILE_CONDITION = 0,
    WHILE_BLOCK = 1
} ASTWhileChild;

typedef enum {
    RETURN_VALUE = 0
} ASTReturnChild;

#endif