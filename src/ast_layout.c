#include "ast_layout.h"
#include "parser.h"
#include <stdlib.h>
#include <string.h>

ASTNode* create_program_node(char* name) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_PROGRAM;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.program = (ProgramNode) {
        .name = name,
        .functions = NULL,
        .function_count = 0,
        .structs = NULL,
        .struct_count = 0,
        .globals = NULL,
        .global_count = 0
    };
    
    return node;
}

ASTNode* create_function_node(char* name, TypeInfo return_type) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_FUNCTION;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.function = (FunctionNode) {
        .name = name,
        .return_type = return_type,
        .params = NULL,
        .param_count = 0,
        .body = NULL
    };

    return node;
}

ASTNode* create_block_node() {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_BLOCK;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.block = (BlockNode) {
        .statements = NULL,
        .statement_count = 0
    };

    return node;
}

ASTNode* create_param_node(char* name, TypeInfo type) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_PARAM_LIST;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.param = (ParamNode) {
        .name = name,
        .type = type
    };

    return node;
}

ASTNode* create_return_node(ASTNode* value) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_RETURN;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.return_stmt = (ReturnNode) {
        .value = value
    };

    return node;
}

ASTNode* create_var_decl_node(char* name, TypeInfo type, ASTNode* initializer) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_VAR_DECL;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.var_decl = (VarDeclNode) {
        .name = name,
        .type = type,
        .initializer = initializer
    };

    return node;
}

ASTNode* create_assign_node(ASTNode* target, ASTNode* value) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_ASSIGN;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.assign = (AssignNode) {
        .target = target,
        .value = value
    };

    return node;
}

ASTNode* create_if_node(ASTNode* condition, ASTNode* then_branch, ASTNode* else_branch) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_IF;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.if_stmt = (IfNode) {
        .condition = condition,
        .then_branch = then_branch,
        .else_branch = else_branch
    };

    return node;
}

ASTNode* create_for_node(ASTNode* init, ASTNode* condition, ASTNode* increment, ASTNode* body) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_FOR;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.for_stmt = (ForNode) {
        .init = init,
        .condition = condition,
        .update = increment,
        .body = body
    };

    return node;
}

ASTNode* create_while_node(ASTNode* condition, ASTNode* body) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_WHILE;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.while_stmt = (WhileNode) {
        .condition = condition,
        .body = body
    };

    return node;
}

ASTNode* create_identifier_node(char* name) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_IDENTIFIER;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.identifier = (IdentifierNode) {
        .name = name
    };

    return node;
}

ASTNode* create_int_literal_node(long long value) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_INT_LITERAL;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.int_literal = (IntLiteralNode) {
        .value = value
    };

    return node;
}

ASTNode* create_float_literal_node(float value) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_FLOAT_LITERAL;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.float_literal = (FloatLiteralNode) {
        .value = value
    };

    return node;
}

ASTNode* create_double_literal_node(double value) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_DOUBLE_LITERAL;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.double_literal = (DoubleLiteralNode) {
        .value = value
    };

    return node;
}

ASTNode* create_string_literal_node(char* value) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_STRING_LITERAL;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.string_literal = (StringLiteralNode) {
        .value = value,
        .length = strlen(value)
    };

    return node;
}

ASTNode* create_char_literal_node(char value) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_CHAR_LITERAL;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.char_literal = (CharLiteralNode) {
        .value = value
    };

    return node;
}

ASTNode* create_func_call_node(char* name) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_FUNC_CALL;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.func_call = (FuncCallNode) {
        .name = name,
        .args = NULL,
        .arg_count = 0
    };

    return node;
}

ASTNode* create_unary_op_node(UnaryOP op, ASTNode* operand) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_UNARY_OP;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.unary_op = (UnaryOpNode) {
        .op = op,
        .operand = operand
    };

    return node;
}

ASTNode* create_binary_op_node(BinaryOp op, ASTNode* left, ASTNode* right) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_BINARY_OP;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.binary_op = (BinaryOpNode) {
        .op = op,
        .left = left,
        .right = right
    };

    return node;
}

ASTNode* create_cast_node(TypeInfo target_type, ASTNode* expr) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_CAST;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.cast = (CastNode) {
        .target_type = target_type,
        .expr = expr
    };

    return node;
}

ASTNode* create_member_access_node(ASTNode* object, char* member) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_MEMBER_ACCESS;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.member_access = (MemberAccessNode) {
        .object = object,
        .member = member
    };

    return node;
}

ASTNode* create_struct_decl_node(char* type) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_STRUCT_DECL;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.struct_decl = (StructDeclNode) {
        .type = type,
        .members = NULL,
        .member_count = 0
    };

    return node;
}

void add_param_to_function(ASTNode* func, ASTNode* param) {
    if (func == NULL || param == NULL)
        return;

    func->as.function.params = realloc(func->as.function.params, sizeof(ASTNode*) * (func->as.function.param_count + 1));
    func->as.function.params[func->as.function.param_count++] = param;
}

void add_member_to_struct(ASTNode* struct_decl, ASTNode* member) {
    if (struct_decl == NULL || member == NULL)
        return;
    
    struct_decl->as.struct_decl.members = realloc(struct_decl->as.struct_decl.members, sizeof(ASTNode*) * (struct_decl->as.struct_decl.member_count + 1));
    struct_decl->as.struct_decl.members[struct_decl->as.struct_decl.member_count++] = member;
}

void add_arg_to_func_call(ASTNode* func_call, ASTNode* arg) {
    if (func_call == NULL || arg == NULL) 
        return;

    func_call->as.func_call.args = realloc(func_call->as.func_call.args, sizeof(ASTNode*) * (func_call->as.func_call.arg_count + 1));
    func_call->as.func_call.args[func_call->as.func_call.arg_count++] = arg;
}

void add_function_to_program(ASTNode* program, ASTNode* function)
{
    if (program == NULL || function == NULL)
        return;

    program->as.program.functions = realloc(program->as.program.functions, sizeof(ASTNode*) * (program->as.program.function_count + 1));
    program->as.program.functions[program->as.program.function_count++] = function;
}

void add_struct_to_program(ASTNode* program, ASTNode* struct_decl) {
    if (program == NULL || struct_decl == NULL)
        return;

    program->as.program.structs = realloc(program->as.program.structs, sizeof(ASTNode*) * (program->as.program.struct_count + 1));
    program->as.program.structs[program->as.program.struct_count++] = struct_decl;
}

void add_global_to_program(ASTNode* program, ASTNode* global) {
    if (program == NULL || global == NULL)
        return;

    program->as.program.globals = realloc(program->as.program.globals, sizeof(ASTNode*) * (program->as.program.global_count + 1));
    program->as.program.globals[program->as.program.global_count++] = global;
}