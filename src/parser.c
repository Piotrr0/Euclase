#include "parser.h"
#include "ast_layout.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Parser parser;

ASTNode* create_program_node(char* name) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_PROGRAM;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.program.name = name;
    node->as.program.functions = NULL;
    node->as.program.function_count = 0;
    node->as.program.structs = NULL;
    node->as.program.struct_count = 0;
    node->as.program.globals = NULL;
    node->as.program.global_count = 0;
    return node;
}

ASTNode* create_function_node(char* name, TypeInfo return_type) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_FUNCTION;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.function.name = name;
    node->as.function.return_type = return_type;
    node->as.function.params = NULL;
    node->as.function.param_count = 0;
    node->as.function.body = NULL;
    return node;
}

ASTNode* create_block_node() {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_BLOCK;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.block.statements = NULL;
    node->as.block.statement_count = 0;
    return node;
}

ASTNode* create_param_node(char* name, TypeInfo type) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_PARAM_LIST;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.param.name = name;
    node->as.param.type = type;
    return node;
}

ASTNode* create_return_node(ASTNode* value) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_RETURN;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.return_stmt.value = value;
    return node;
}

ASTNode* create_var_decl_node(char* name, TypeInfo type, ASTNode* initializer) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_VAR_DECL;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.var_decl.name = name;
    node->as.var_decl.type = type;
    node->as.var_decl.initializer = initializer;
    return node;
}

ASTNode* create_assign_node(ASTNode* target, ASTNode* value) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_ASSIGN;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.assign.target = target;
    node->as.assign.value = value;
    return node;
}

ASTNode* create_if_node(ASTNode* condition, ASTNode* then_branch, ASTNode* else_branch) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_IF;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.if_stmt.condition = condition;
    node->as.if_stmt.then_branch = then_branch;
    node->as.if_stmt.else_branch = else_branch;
    return node;
}

ASTNode* create_for_node(ASTNode* init, ASTNode* condition, ASTNode* increment, ASTNode* body) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_FOR;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.for_stmt.init = init;
    node->as.for_stmt.condition = condition;
    node->as.for_stmt.update = increment;
    node->as.for_stmt.body = body;
    return node;
}

ASTNode* create_while_node(ASTNode* condition, ASTNode* body) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_WHILE;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.while_stmt.condition = condition;
    node->as.while_stmt.body = body;
    return node;
}

ASTNode* create_identifier_node(char* name) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_IDENTIFIER;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.identifier.name = name;
    return node;
}

ASTNode* create_int_literal_node(long long value) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_INT_LITERAL;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.int_literal.value = value;
    return node;
}

ASTNode* create_float_literal_node(float value) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_FLOAT_LITERAL;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.float_literal.value = value;
    return node;
}

ASTNode* create_double_literal_node(double value) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_DOUBLE_LITERAL;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.double_literal.value = value;
    return node;
}

ASTNode* create_string_literal_node(char* value) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_STRING_LITERAL;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.string_literal.value = value;
    node->as.string_literal.length = strlen(value);
    return node;
}

ASTNode* create_char_literal_node(char value) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_CHAR_LITERAL;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.char_literal.value = value;
    return node;
}

ASTNode* create_func_call_node(char* name) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_FUNC_CALL;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.func_call.name = name;
    node->as.func_call.args = NULL;
    node->as.func_call.arg_count = 0;
    return node;
}

ASTNode* create_unary_op_node(UnaryOP op, ASTNode* operand) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_UNARY_OP;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.unary_op.op = op;
    node->as.unary_op.operand = operand;
    return node;
}

ASTNode* create_binary_op_node(BinaryOp op, ASTNode* left, ASTNode* right) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_BINARY_OP;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.binary_op.op = op;
    node->as.binary_op.left = left;
    node->as.binary_op.right = right;
    return node;
}

ASTNode* create_cast_node(TypeInfo target_type, ASTNode* expr) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_CAST;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.cast.target_type = target_type;
    node->as.cast.expr = expr;
    return node;
}

ASTNode* create_member_access_node(ASTNode* object, char* member) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_MEMBER_ACCESS;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.member_access.object = object;
    node->as.member_access.member = member;
    return node;
}

ASTNode* create_struct_decl_node(char* name) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
        return NULL;

    node->type = AST_STRUCT_DECL;
    node->line = current_token()->line;
    node->column = current_token()->column;
    node->as.struct_decl.name = name;
    node->as.struct_decl.members = NULL;
    node->as.struct_decl.member_count = 0;
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

static void free_node_array(ASTNode** array, int count) {
    if (array == NULL)
        return;
    
    for (int i = 0; i < count; i++) {
        free_ast(array[i]);
    }

    free(array);
}

void free_ast(ASTNode* node) {
    if (node == NULL)
        return;
    
    switch (node->type) {
        case AST_PROGRAM:
            free(node->as.program.name);
            free_node_array(node->as.program.functions, node->as.program.function_count);
            free_node_array(node->as.program.structs, node->as.program.struct_count);
            free_node_array(node->as.program.globals, node->as.program.global_count);
            break;
            
        case AST_FUNCTION:
            free(node->as.function.name);
            free_node_array(node->as.function.params, node->as.function.param_count);
            free_ast(node->as.function.body);
            break;
            
        case AST_BLOCK:
            free_node_array(node->as.block.statements, node->as.block.statement_count);
            break;
            
        case AST_PARAM_LIST:
            free(node->as.param.name);
            break;
            
        case AST_RETURN:
            free_ast(node->as.return_stmt.value);
            break;
            
        case AST_VAR_DECL:
            free(node->as.var_decl.name);
            free_ast(node->as.var_decl.initializer);
            break;
            
        case AST_ASSIGN:
            free_ast(node->as.assign.target);
            free_ast(node->as.assign.value);
            break;
            
        case AST_IF:
            free_ast(node->as.if_stmt.condition);
            free_ast(node->as.if_stmt.then_branch);
            free_ast(node->as.if_stmt.else_branch);
            break;
            
        case AST_FOR:
            free_ast(node->as.for_stmt.init);
            free_ast(node->as.for_stmt.condition);
            free_ast(node->as.for_stmt.update);
            free_ast(node->as.for_stmt.body);
            break;
            
        case AST_WHILE:
            free_ast(node->as.while_stmt.condition);
            free_ast(node->as.while_stmt.body);
            break;
            
        case AST_IDENTIFIER:
            free(node->as.identifier.name);
            break;
            
        case AST_STRING_LITERAL:
            free(node->as.string_literal.value);
            break;
            
        case AST_FUNC_CALL:
            free(node->as.func_call.name);
            free_node_array(node->as.func_call.args, node->as.func_call.arg_count);
            break;
            
        case AST_UNARY_OP:
            free_ast(node->as.unary_op.operand);
            break;
            
        case AST_BINARY_OP:
            free_ast(node->as.binary_op.left);
            free_ast(node->as.binary_op.right);
            break;
            
        case AST_CAST:
            free_ast(node->as.cast.expr);
            break;
            
        case AST_MEMBER_ACCESS:
            free_ast(node->as.member_access.object);
            free(node->as.member_access.member);
            break;
            
        case AST_STRUCT_DECL:
            free(node->as.struct_decl.name);
            free_node_array(node->as.struct_decl.members, node->as.struct_decl.member_count);
            break;
            
        default:
            break;
    }
    
    free(node);
}

void advance() {
    if (parser.current_token < parser.tokens->token_count - 1)
        parser.current_token++;
}

int match(TokenType type) {
    if (current_token()->type == type) {
        advance();
        return 1;
    }
    return 0;
}

int check(TokenType type) {
    return current_token()->type == type;
}

int is_type(TokenType t) {
    switch(t) {
        case TOK_INT: case TOK_UINT: case TOK_FLOAT: case TOK_UFLOAT:
        case TOK_DOUBLE: case TOK_UDOUBLE: case TOK_CHAR: case TOK_UCHAR:
        case TOK_VOID: return 1;
        default: return 0;
    }
}

int parse_pointer_level() {
    int level = 0;
    while (match(TOK_MULTIPLICATION)) {
        level++;
    }
    return level;
}

ASTNode* parse_compound_operators() {
    ASTNode* lhs = parse_expression();
    if (lhs == NULL)
        return NULL;

    if (lhs->type != AST_IDENTIFIER && lhs->type != AST_MEMBER_ACCESS && (lhs->type != AST_UNARY_OP || lhs->as.unary_op.op != OP_DEREF)) {
        free_ast(lhs);
        return NULL;
    }

    TokenType op_tok = current_token()->type;
    BinaryOp op;

    switch(op_tok) {
        case TOK_ASSIGNMENT_ADDITION:       op = OP_ADD; break;
        case TOK_ASSIGNMENT_SUBTRACTION:    op = OP_SUB; break;
        case TOK_ASSIGNMENT_MULTIPLICATION: op = OP_MUL; break;
        case TOK_ASSIGNMENT_DIVISION:       op = OP_DIV; break;
        case TOK_ASSIGNMENT_MODULO:         op = OP_MOD; break;
        default: free_ast(lhs); return NULL;
    }
    
    advance();
    ASTNode* rhs = parse_expression();
    if (rhs == NULL) {
        free_ast(lhs);
        return NULL;
    }

    if (!match(TOK_SEMICOLON)) {
        free_ast(lhs);
        free_ast(rhs);
        return NULL;
    }
    
    ASTNode* lhs_copy = NULL;
    if (lhs->type == AST_IDENTIFIER) {
        lhs_copy = create_identifier_node(strdup(lhs->as.identifier.name));
    }
    
    if (lhs_copy == NULL) {
        free_ast(lhs);
        free_ast(rhs);
        return NULL;
    }
    
    ASTNode* binary_node = create_binary_op_node(op, lhs_copy, rhs);
    ASTNode* assign_node = create_assign_node(lhs, binary_node);
    return assign_node;
}

ASTNode* parse_expression() 
{
    return parse_assignment();
}

ASTNode* parse_assignment()
{
    ASTNode* left = parse_equality();
    if (left == NULL) 
        return NULL;
    
    while (check(TOK_ASSIGNMENT) || is_compound_token(current_token()->type))
    {
        if (left->type != AST_IDENTIFIER && left->type != AST_MEMBER_ACCESS && (left->type != AST_UNARY_OP || left->as.unary_op.op != OP_DEREF)) {
            free_ast(left);
            return NULL;
        }
        
        TokenType op_tok = current_token()->type;
        advance();
        
        ASTNode* right = parse_assignment();
        if (right == NULL) {
            free_ast(left);
            return NULL;
        }

        BinaryOp op;
        switch(op_tok) {
            case TOK_ASSIGNMENT_ADDITION:       op = OP_ADD; break;
            case TOK_ASSIGNMENT_SUBTRACTION:    op = OP_SUB; break;
            case TOK_ASSIGNMENT_MULTIPLICATION: op = OP_MUL; break;
            case TOK_ASSIGNMENT_DIVISION:       op = OP_DIV; break;
            case TOK_ASSIGNMENT_MODULO:         op = OP_MOD; break;
            case TOK_ASSIGNMENT:                return create_assign_node(left, right);
            default: free_ast(left); free_ast(right); return NULL;
        }
        
        if (left->type != AST_IDENTIFIER && left->type != AST_MEMBER_ACCESS && (left->type != AST_UNARY_OP || left->as.unary_op.op != OP_DEREF)) {
            free_ast(left);
            return NULL;
        }

        ASTNode* left_copy = create_identifier_node(strdup(left->as.identifier.name));
        if (left_copy == NULL) {
            free_ast(left);
            free_ast(right);
            return NULL;
        }
        
        ASTNode* binary_node = create_binary_op_node(op, left_copy, right);
        return create_assign_node(left, binary_node);
    }
    
    return left;
}

ASTNode* parse_equality()
{
    ASTNode* left = parse_additive();
    if (left == NULL) 
        return NULL;
    
    while (check(TOK_EQUAL) || check(TOK_NOT_EQUAL) || check(TOK_LESS) || check(TOK_GREATER) || check(TOK_LESS_EQUALS) || check(TOK_GREATER_EQUALS))
    {
        TokenType op = current_token()->type;
        advance();
        
        ASTNode* right = parse_additive();
        if (right == NULL) {
            free_ast(left);
            return NULL;
        }
        
        BinaryOp binary_op;
        switch(op) {
            case TOK_EQUAL:          binary_op = OP_EQ; break;
            case TOK_NOT_EQUAL:      binary_op = OP_NE; break;
            case TOK_LESS:           binary_op = OP_LT; break;
            case TOK_GREATER:        binary_op = OP_GT; break;
            case TOK_LESS_EQUALS:    binary_op = OP_LE; break;
            case TOK_GREATER_EQUALS: binary_op = OP_GE; break;
            default:                 binary_op = OP_EQ; break;
        }
        
        left = create_binary_op_node(binary_op, left, right);
    }
    
    return left;
}

ASTNode* parse_additive()
{
    ASTNode* left = parse_multiplicative();
    if (left == NULL) 
        return NULL;
    
    while (check(TOK_ADDITION) || check(TOK_SUBTRACTION))
    {
        TokenType op = current_token()->type;
        advance();
        
        ASTNode* right = parse_multiplicative();
        if (right == NULL) {
            free_ast(left);
            return NULL;
        }
        
        BinaryOp binary_op;
        switch(op) 
        {
            case TOK_ADDITION:      binary_op = OP_ADD; break;
            case TOK_SUBTRACTION:   binary_op = OP_SUB; break;
            default:                binary_op = OP_ADD; break;
        }

        left = create_binary_op_node(binary_op, left, right);
    }
    
    return left;
}

ASTNode* parse_multiplicative() 
{
    ASTNode* left = parse_unary();
    if (left == NULL) 
        return NULL;
    
    while (check(TOK_MULTIPLICATION) || check(TOK_DIVISION) || check(TOK_MODULO))
    {
        TokenType op = current_token( )->type;
        advance();
        
        ASTNode* right = parse_unary();
        if (right == NULL) {
            free_ast(left);
            return NULL;
        }
        
        BinaryOp binary_op;
        switch(op)
        {
            case TOK_MULTIPLICATION: binary_op = OP_MUL; break;
            case TOK_DIVISION:       binary_op = OP_DIV; break;
            case TOK_MODULO:         binary_op = OP_MOD; break;
            default:                 binary_op = OP_MUL; break;
        }
        
        left = create_binary_op_node(binary_op, left, right);
    }
    
    return left;
}

ASTNode* parse_negation() {
    advance();
    ASTNode* unary_expression = parse_unary();
    if (unary_expression == NULL) {
        return NULL;
    }
        
    ASTNode* unary_minus_node = create_unary_op_node(OP_NEG, unary_expression);
    if (unary_minus_node == NULL) {
        free_ast(unary_expression);
        return NULL;
    }

    return unary_minus_node;
}

ASTNode* parse_pre_decrement() {
    if (!match(TOK_INCREMENT))
        return NULL;

    ASTNode* node = parse_unary();
    if (node == NULL)
        return NULL;

    ASTNode* dec = create_unary_op_node(OP_PRE_DEC, node);
    if (dec == NULL)
        free_ast(node);
    
    return dec;
}

ASTNode* parse_pre_increment() {
    if (!match(TOK_DECREMENT))
        return NULL;

    ASTNode* node = parse_unary();
    if (node == NULL)
        return NULL;

    ASTNode* inc = create_unary_op_node(OP_PRE_INC, node);
    if (inc == NULL)
        free_ast(node);
    
    return inc;
}

ASTNode* parse_unary()
{
    switch (current_token()->type) {
        case TOK_SUBTRACTION:       return parse_negation();
        case TOK_MULTIPLICATION:    return parse_dereference();
        case TOK_AMPERSAND:         return parse_address_of();
        case TOK_INCREMENT:         return parse_pre_increment();
        case TOK_DECREMENT:         return parse_pre_decrement();

        default: break;
    }

    if (is_casting())
        return parse_casting();

    return parse_postfix();
}

ASTNode* parse_postfix()
{
    ASTNode* node = NULL;
    if (is_func_call())
        node = parse_function_call();
    else
         node = parse_primary();

    while (check(TOK_DOT)) {
        advance();
        
        if (!check(TOK_IDENTIFIER)) {
            free_ast(node);
            return NULL;
        }
        
        ASTNode* member_access = create_member_access_node(node, sv_to_owned_cstr(current_token()->lexeme));
        if (member_access == NULL) {
            free_ast(node);
            return NULL;
        }

        advance();
        node = member_access;
    }
    
    return node;
}

ASTNode* parse_primary()
{
    if (check(TOK_IDENTIFIER))
        return parse_identifier_expression();
    
    return parse_primary_expression();
}

ASTNode* parse_parens() {
    if (!check(TOK_LPAREN)) 
        return NULL;

    advance();
    ASTNode* node = parse_expression();
    if (node == NULL)
        return NULL;

    if (!match(TOK_RPAREN)) {
        printf("Parse error: expected ')'\n");
        free_ast(node);
        return NULL;
    }
    return node;
}

ASTNode* parse_primary_expression() {
    ASTNode* paren = parse_parens();
    if(paren != NULL)
        return paren;

    switch (current_token()->type)
    {
        case TOK_CHAR_LITERAL:      return parse_char_literal();
        case TOK_STRING_LITERAL:    return parse_string_literal();
        case TOK_NUMBER_INT:
        case TOK_NUMBER_FLOAT:
        case TOK_NUMBER_DOUBLE:     return parse_number_literal();
        default: printf("Parse error: expected primary expression\n");
    }
    return NULL;
}

ASTNode* parse_number_literal() {
    if (!(check(TOK_NUMBER_INT) || check(TOK_NUMBER_DOUBLE) || check(TOK_NUMBER_FLOAT)))
        return NULL;
    
    const char* number = sv_to_owned_cstr(current_token()->lexeme);

    ASTNode* result = NULL;
    switch (current_token()->type)
    {
        case TOK_NUMBER_INT: {
            long long int_val = atoll(number);
            result = create_int_literal_node(int_val);
            break;
        }
        case TOK_NUMBER_FLOAT: {
            float float_val = atof(number);
            result = create_float_literal_node(float_val);
            break;
        }
        case TOK_NUMBER_DOUBLE: {
            double double_val = atof(number);
            result = create_double_literal_node(double_val);
            break;
        }

        default: break;
    }
    
    advance();
    free((char*)number);
    return result;
}

ASTNode* parse_string_literal() {
    if (!check(TOK_STRING_LITERAL))
        return NULL;

    char* string = sv_to_owned_cstr(current_token()->lexeme);
    advance();

    return create_string_literal_node(string);;
}

ASTNode* parse_char_literal() {
    if (!check(TOK_CHAR_LITERAL))
        return NULL;

    char ch = current_token()->lexeme.data[0];
    advance();

    return create_char_literal_node(ch);
}

ASTNode* parse_identifier_expression()
{
    ASTNode* node = create_identifier_node(sv_to_owned_cstr(current_token()->lexeme));
    advance();
    return node;
}

ASTNode* parse_dereference()
{
    if(!check(TOK_MULTIPLICATION))
        return NULL;

    advance();
    ASTNode* expr = parse_unary();
    if (expr == NULL) {
        return NULL;
    }

    ASTNode* node = create_unary_op_node(OP_DEREF, expr);
    if(node == NULL) {
        free_ast(expr);
        return NULL;
    }
    
    return node;
}

ASTNode* parse_address_of() {
    if(!check(TOK_AMPERSAND))
        return NULL;

    advance();
    ASTNode* expr = parse_unary();
    ASTNode* node = create_unary_op_node(OP_ADDR, expr);
    if (node == NULL) {
        free_ast(expr);
        return NULL;
    }

    return node;
}

// int x = sum(10, 10); 
// int x = sum(10, &a);
ASTNode* parse_function_call()
{
    if(!check(TOK_IDENTIFIER)) {
        return NULL;
    }

    char* name = sv_to_owned_cstr(current_token()->lexeme);
    if (name == NULL)
        return NULL;

    advance();

    if(!match(TOK_LPAREN)) {
        free(name);
        return NULL;
    }

    ASTNode* func_call = create_func_call_node(name);
    if (func_call == NULL) {
        free(name);
        return NULL;
    }

    while (!check(TOK_RPAREN) && !check(TOK_EOF))
    {
        ASTNode* arg = parse_expression();
        if(arg == NULL) {
            printf("Parse error: expected expression in function argument\n");
            return func_call;
        }

        add_arg_to_func_call(func_call, arg);

        if (!match(TOK_COMMA)) {
            break;
        }
    }

    if(!match(TOK_RPAREN)) {
        printf("Parse error: expected ')' after function arguments\n");
        return func_call;
    }

    return func_call;
}

// float pi = 3.14f;
// int v = (int) pi;
ASTNode* parse_casting()
{
    if(!match(TOK_LPAREN)) {
        printf("Parse error: expected: '('\n");
        return NULL;
    }

    if(!is_type(current_token()->type)) {
        printf("Parse error: expected: 'TYPE'\n");
        return NULL;
    }

    TypeInfo cast_type = parse_type(); 

    if(!match(TOK_RPAREN)) {
        printf("Parse error: expected: ')'\n");
        return NULL;
    }

    ASTNode* expr = parse_unary();
    if(expr == NULL) {
        printf("Parse error: expected expression after cast\n");
        return NULL;
    }

    ASTNode* cast_node = create_cast_node(cast_type, expr);
    if(cast_node == NULL) {
        free_ast(expr);
        printf("Memory allocation failed\n");
        return NULL;
    }

    return cast_node;
}

TypeInfo parse_type()
{
    if (!is_type(current_token()->type)) {
        printf("Parse error: expected return type\n");
        exit(1);
    }

    TokenType ret_type = current_token()->type;
    advance();

    int pointer_level = parse_pointer_level();

    TypeInfo info = { .base_type = ret_type, .pointer_level = pointer_level };
    return info;
}

int is_func_call()
{
    if(!check(TOK_IDENTIFIER))
        return 0;

    Token* lparen_token = peek_token(1);
    if(lparen_token->type != TOK_LPAREN) {
        return 0;
    }

    return 1;
}

int is_casting() {
    if(!check(TOK_LPAREN))
        return 0;

    Token* type_token = peek_token(1);
    if(!is_type(type_token->type))
        return 0;

    int pointer_level = check_pointer_level(2);
    Token* rparen_token = peek_token(2 + pointer_level);

    return (rparen_token->type == TOK_RPAREN);
}

int check_pointer_level(int offset) 
{
    int starting_offset = offset; 
    Token* next_token = peek_token(offset);

    while(next_token->type == TOK_MULTIPLICATION) {
        next_token = peek_token(++offset);
    }
    
    return offset - starting_offset;
}

ASTNode* parse_while_loop() {
    if (!match(TOK_WHILE))
        return NULL;

    if (!match(TOK_LPAREN))
        return NULL;

    ASTNode* condition = parse_expression();
    if(condition == NULL) {
        free_ast(condition);
        return NULL;
    }

    if (!match(TOK_RPAREN)) {
        free_ast(condition);
        return NULL;
    }

    ASTNode* body = parse_block();
    if(body == NULL) {
        free_ast(condition);
        return NULL;
    }

    return create_while_node(condition, body);
}

ASTNode* parse_for_loop() {
    if (!match(TOK_FOR))
        return NULL;

    if (!match(TOK_LPAREN))
        return NULL;

    ASTNode* init = NULL;
    if (!check(TOK_SEMICOLON)) {
        init = parse_loop_init();
        if (init == NULL) {
            return NULL;
        }
    }

    if (init == NULL || init->type != AST_VAR_DECL) {
        if (!match(TOK_SEMICOLON)) {
            free_ast(init);
            return NULL;
        }
    }
        
    ASTNode* condition = NULL;
    if (!check(TOK_SEMICOLON)) {
        condition = parse_expression();
        if (condition == NULL) {
            free_ast(init);
            return NULL;
        }
    }

    if (!match(TOK_SEMICOLON)) {
        free_ast(init);
        free_ast(condition);
        return NULL;
    }

    ASTNode* update = NULL;
    if (!check(TOK_RPAREN)) {
        update = parse_loop_update();
        if (update == NULL) {
            free_ast(init);
            free_ast(condition);
            return NULL;
        }
    }

    if(!match(TOK_RPAREN)) {
        free_ast(init);
        free_ast(condition);
        free_ast(update);
        return NULL;
    }

    ASTNode* body = parse_block();
    if(body == NULL) {
        free_ast(init);
        free_ast(condition);
        free_ast(update);
        return NULL;
    }

    return create_for_node(init, condition, update, body);
}

ASTNode* parse_loop_init() {
    ASTNode* init = NULL;

    if (is_type(current_token()->type))
        init = parse_variable_declaration();
    else if (check(TOK_IDENTIFIER) || check(TOK_MULTIPLICATION))
        init = parse_assignment();

    return init;
}

ASTNode* parse_loop_condition() {

    ASTNode* condition = NULL;
    if (!check(TOK_SEMICOLON))
    {
        condition = parse_expression();
        if(condition == NULL)
            return NULL;
    }
    
    if (!match(TOK_SEMICOLON)) {
        free_ast(condition);
        return NULL;
    }

    return condition;
}

ASTNode* parse_loop_update()
{
    ASTNode* lhs = parse_expression();
    if (lhs == NULL)
        return NULL;
    
    if (!match(TOK_ASSIGNMENT)) {
        return lhs;
    }
    
    ASTNode* rhs = parse_expression();
    if (rhs == NULL) {
        free_ast(lhs);
        return NULL;
    }
    
    return create_assign_node(lhs, rhs);
}

ASTNode* parse_if() {
    if(!match(TOK_IF))
        return NULL;

    if(!match(TOK_LPAREN))
        return NULL;

    ASTNode* condition = parse_expression();
    if(condition == NULL)
        return NULL;

    if(!match(TOK_RPAREN)) {
        free_ast(condition);
        return NULL;
    }
    
    ASTNode* then_branch = parse_block();
    if(then_branch == NULL) {
        free_ast(condition);
        return NULL;
    }
    
    if (!check(TOK_ELSE))
        return create_if_node(condition, then_branch, NULL);

    advance();

    ASTNode* else_branch = parse_block();
    if (else_branch == NULL) {
        free_ast(condition);
        free_ast(then_branch);
        return NULL;
    }
    
    return create_if_node(condition, then_branch, else_branch);
}

ASTNode* parse_else() {
    if(!match(TOK_ELSE))
        return NULL;

    ASTNode* node_else = parse_block();
    if(node_else == NULL)
        return NULL;

    return node_else;
}

int is_compound_token(TokenType type) {
    if (type == TOK_ASSIGNMENT_ADDITION ||
        type == TOK_ASSIGNMENT_SUBTRACTION ||
        type == TOK_ASSIGNMENT_MULTIPLICATION ||
        type == TOK_ASSIGNMENT_DIVISION ||
        type == TOK_ASSIGNMENT_MODULO)
            return 1;
    return 0;
}

ASTNode* parse_variable_declaration() {
    TypeInfo type = parse_type();
    if (current_token()->type != TOK_IDENTIFIER) {
        printf("Parse error: expected variable name\n");
        return NULL;
    }
    
    char* name = sv_to_owned_cstr(current_token()->lexeme);
    if (name == NULL)
        return NULL;

    advance();

    ASTNode* expr = NULL;
    if (match(TOK_ASSIGNMENT))
    {
        expr = parse_expression();
        if(expr == NULL) {
            free(name);
            return NULL;
        }
    }
    
    if (!match(TOK_SEMICOLON)) {
        printf("Parse error: expected ';'\n");
        free(name);
        free_ast(expr);
        return NULL;
    }

    return create_var_decl_node(name, type, expr);
}

ASTNode* parse_struct_member() {
    TypeInfo type = parse_type();
    
    if (!check(TOK_IDENTIFIER))
        return NULL;
    
    char* name = sv_to_owned_cstr(current_token()->lexeme);
    advance();
    
    if (!match(TOK_SEMICOLON)) {
        free(name);
        return NULL;
    }
    
    return create_var_decl_node(name, type, NULL);
}

char* parse_struct_name() {
    if (!match(TOK_STRUCT))
        return NULL;
    
    if (!check(TOK_IDENTIFIER))
        return NULL;
    
    char* name = sv_to_owned_cstr(current_token()->lexeme);
    advance();

    return name;
}

ASTNode* parse_struct_declaration() {
    if (!check(TOK_STRUCT))
        return NULL;
    
    char* name = parse_struct_name();

    if (!match(TOK_LBRACE)) {
        free(name);
        return NULL;
    }
    
    ASTNode* struct_decl = create_struct_decl_node(name);
    if (struct_decl == NULL) {
        free(name);
        return NULL;
    }
    
    while (!check(TOK_RBRACE) && !check(TOK_EOF))
    {
        if (is_type(current_token()->type)) {
            ASTNode* member = parse_struct_member();
            if(member == NULL)
                continue;

            add_member_to_struct(struct_decl, member);
        }
        else
            advance();
    }
    
    if (!match(TOK_RBRACE)) {
        free_ast(struct_decl);
        return NULL;
    }

    if (!match(TOK_SEMICOLON)) {
        free_ast(struct_decl);
        return NULL;
    }

    return struct_decl;
}

ASTNode* parse_return() {
    if (!match(TOK_RETURN)) {
        printf("Parse error: expected 'return'\n");
        return NULL;
    }

    ASTNode* expr = NULL;
    if (!check(TOK_SEMICOLON)) {
        expr = parse_expression();
        if(expr == NULL) {
            return NULL;
        }
    }

    if (!match(TOK_SEMICOLON)) {
        printf("Parse error: expected ';' after return\n");
        free_ast(expr);
        return NULL;
    }
    return create_return_node(expr);
}

ASTNode* parse_statement() {

    switch (current_token()->type)
    {
        case TOK_RETURN:            return parse_return();
        case TOK_IF:                return parse_if();
        case TOK_FOR:               return parse_for_loop();
        case TOK_WHILE:             return parse_while_loop();
        case TOK_LBRACE:            return parse_block();
        case TOK_STRUCT:            return parse_struct_declaration();
        default: break;
    }

    if (is_type(current_token()->type))
        return parse_variable_declaration();

    ASTNode* node = parse_expression();
    if (node == NULL)
        return NULL;

    if (!match(TOK_SEMICOLON)) {
        printf("Parse error: expected ';'\n");
        free_ast(node);
        return NULL;
    }
    return node;
}

void add_statement_to_block(ASTNode* block, ASTNode* stmt) {
    if (block == NULL || stmt == NULL)
        return;

    block->as.block.statements = realloc(block->as.block.statements, sizeof(ASTNode*) * (block->as.block.statement_count + 1));
    block->as.block.statements[block->as.block.statement_count++] = stmt;
}

ASTNode* parse_block()
{
    if (!match(TOK_LBRACE)) {
        printf("Parse error: expected '{'\n");
        return NULL;
    }

    ASTNode* block = create_block_node();
    if (block == NULL)
        return NULL;

    while (!check(TOK_RBRACE) && !check(TOK_EOF)) {
        ASTNode* stmt = parse_statement();
        if(stmt == NULL)
            continue;

        add_statement_to_block(block, stmt);
    }

    if (!match(TOK_RBRACE)) {
        printf("Parse error: expected '}'\n");
        free(block);
        return NULL;
    }
    return block;
}

int is_func_declaration() {
    if(!is_type(current_token()->type))
        return 0;

    Token* next_token = peek_token(1);
    if (next_token->type != TOK_IDENTIFIER)
        return 0;

    Token* third_token = peek_token(2);
    return (third_token->type == TOK_LPAREN);
}

ASTNode* parse_parameters(ASTNode* func)
{
    if (!match(TOK_LPAREN)) {
        printf("Parse error: expected '('\n");
        return NULL;
    }

    while (!check(TOK_RPAREN) && !check(TOK_EOF)) 
    {
        if (!is_type(current_token()->type)) {
            printf("Parse error: expected type in parameter list\n");
            return NULL;
        }

        TypeInfo type = parse_type();
        if (!check(TOK_IDENTIFIER)) {
            printf("Parse error: expected parameter name\n");
            return NULL;
        }

        char* param_name = sv_to_owned_cstr(current_token()->lexeme);
        if(param_name == NULL)
            return NULL;

        advance();

        ASTNode* param = create_param_node(param_name, type);
        if (param == NULL)
            return NULL;

        add_param_to_function(func, param);

        if (!match(TOK_COMMA))
            break;
    }

    if (!match(TOK_RPAREN)) {
        printf("Parse error: expected ')'\n");
        return NULL;
    }

    return func;
}

ASTNode* parse_function()
{
    TypeInfo return_type = parse_type();
    if (!check(TOK_IDENTIFIER)) {
        printf("Parse error: expected function name\n");
        return NULL;
    }

    char* name = sv_to_owned_cstr(current_token()->lexeme);
    if(name == NULL)
        return NULL;

    advance();

    ASTNode* func = create_function_node(name, return_type);
    if (func == NULL) {
        free(name);
        return NULL;
    }
    
    if (parse_parameters(func) == NULL) {
        free_ast(func);
        return NULL;
    }
    
    ASTNode* body = parse_block();
    if(body == NULL) {
        free_ast(func);
        return NULL;
    }

    func->as.function.body = body;
    return func;
}

/* namespace main */
char* parse_namespace_name()
{
    if (!match(TOK_NAMESPACE)) {
        printf("Parse error: expected 'namespace'\n");
        return NULL;
    }
    
    if (!check(TOK_IDENTIFIER)) {
        printf("Parse error: expected namespace name\n");
        return NULL;
    }
    
    char* namespace_name = sv_to_owned_cstr(current_token()->lexeme);
    advance();

    return namespace_name;
}

ASTNode* parse_program() {
    if(!check(TOK_NAMESPACE))
        return NULL;

    char* namespace_name = parse_namespace_name();

    if (!match(TOK_LBRACE)) {
        printf("Parse error: expected '{'\n");
        free(namespace_name);
        return NULL;
    }
    
    ASTNode* program = create_program_node(namespace_name);
    if (program == NULL) {
        free(namespace_name);
        return NULL;
    }

    while (!check(TOK_RBRACE) && !check(TOK_EOF)) {

        if (is_func_declaration())
        {
            ASTNode* func = parse_function();
            if(func == NULL)
                continue;

            add_function_to_program(program, func);
        }
        else if (check(TOK_STRUCT)) {
            ASTNode* struct_node = parse_struct_declaration();
            if(struct_node == NULL)
                continue;

            add_struct_to_program(program, struct_node);
        }
        else if (is_type(current_token()->type))
        {
            ASTNode* var_decl = parse_variable_declaration();
            if(var_decl == NULL)
                continue;

            add_global_to_program(program, var_decl);
        }
        else
        {
            printf("Parse error: unexpected token in namespace\n");
            advance();
        }
    }

    if (!match(TOK_RBRACE)) {
        printf("Parse error: expected '}'\n");
        return NULL;
    }

    return program;
}

void init_parser(Tokens* tokens) {
    parser.tokens = tokens;
    parser.current_token = 0;
}

Token* current_token()
{
    if (parser.current_token >= parser.tokens->token_count)
        return &parser.tokens->tokens[parser.tokens->token_count - 1];

    return &parser.tokens->tokens[parser.current_token];
}

Token* peek_token(int offset)
{
    int pos = parser.current_token + offset;
    if (pos >= parser.tokens->token_count)
        return &parser.tokens->tokens[parser.tokens->token_count - 1];

    return &parser.tokens->tokens[pos];
}

void print_ast(ASTNode* node, int level) 
{
    if(node == NULL) return;
    
    for (int i = 0; i < level; i++) 
        printf("  ");
    
    switch (node->type) {
        case AST_PROGRAM:
            printf("Program (namespace %s)\n", node->as.program.name ? node->as.program.name : "(unnamed)");
            for (int i = 0; i < node->as.program.struct_count; i++)
                print_ast(node->as.program.structs[i], level + 1);
            for (int i = 0; i < node->as.program.global_count; i++)
                print_ast(node->as.program.globals[i], level + 1);
            for (int i = 0; i < node->as.program.function_count; i++)
                print_ast(node->as.program.functions[i], level + 1);
            break;
            
        case AST_FUNCTION:
            printf("Function (return type %s, name %s)\n", 
                token_type_name(node->as.function.return_type.base_type), 
                node->as.function.name);
            for (int i = 0; i < node->as.function.param_count; i++)
                print_ast(node->as.function.params[i], level + 1);
            if (node->as.function.body)
                print_ast(node->as.function.body, level + 1);
            break;
            
        case AST_PARAM_LIST:
            printf("Parameter (type %s, name %s, level %d)\n", 
                token_type_name(node->as.param.type.base_type),
                node->as.param.name,
                node->as.param.type.pointer_level);
            break;
            
        case AST_BLOCK:
            printf("Block\n");
            for (int i = 0; i < node->as.block.statement_count; i++)
                print_ast(node->as.block.statements[i], level + 1);
            break;
            
        case AST_RETURN:
            printf("Return\n");
            if (node->as.return_stmt.value)
                print_ast(node->as.return_stmt.value, level + 1);
            break;
            
        case AST_ASSIGN:
            printf("Assign\n");
            if (node->as.assign.target)
                print_ast(node->as.assign.target, level + 1);
            if (node->as.assign.value)
                print_ast(node->as.assign.value, level + 1);
            break;
            
        case AST_VAR_DECL:
            printf("VariableDecl(type %s, name %s, level %d)\n", 
                token_type_name(node->as.var_decl.type.base_type), 
                node->as.var_decl.name, 
                node->as.var_decl.type.pointer_level);
            if (node->as.var_decl.initializer)
                print_ast(node->as.var_decl.initializer, level + 1);
            break;

        case AST_UNARY_OP: {
                const char* op_name = NULL;
                switch (node->as.unary_op.op) {
                    case OP_DEREF: op_name = "Dereference"; break;
                    case OP_ADDR:  op_name = "Address Of"; break;
                    case OP_NEG:   op_name = "Negation"; break;
                    default:       op_name = "Unknown"; break;
                }
                printf("UnaryOp(%s)\n", op_name);
                if (node->as.unary_op.operand)
                    print_ast(node->as.unary_op.operand, level + 1);
                break;
            }

        case AST_FUNC_CALL:
            printf("Function call(name: %s, args: %d)\n", 
                node->as.func_call.name,
                node->as.func_call.arg_count);
            for (int i = 0; i < node->as.func_call.arg_count; i++)
                print_ast(node->as.func_call.args[i], level + 1);
            break;
            
        case AST_IDENTIFIER:
            printf("Identifier(%s)\n", node->as.identifier.name);
            break;
            
        case AST_BINARY_OP: {
            const char* op_name = NULL;
            switch (node->as.binary_op.op) {
                case OP_ADD: op_name = "Addition"; break;
                case OP_SUB: op_name = "Subtraction"; break;
                case OP_MUL: op_name = "Multiplication"; break;
                case OP_DIV: op_name = "Division"; break;
                case OP_MOD: op_name = "Modulo"; break;
                case OP_EQ:  op_name = "Equal"; break;
                case OP_NE:  op_name = "NotEqual"; break;
                case OP_LT:  op_name = "Less"; break;
                case OP_GT:  op_name = "Greater"; break;
                case OP_LE:  op_name = "LessEqual"; break;
                case OP_GE:  op_name = "GreaterEqual"; break;
                default:     op_name = "Unknown"; break;
            }
            printf("BinaryOp(%s)\n", op_name);
            if (node->as.binary_op.left)
                print_ast(node->as.binary_op.left, level + 1);
            if (node->as.binary_op.right)
                print_ast(node->as.binary_op.right, level + 1);
            break;
        }
            
        case AST_IF:
            printf("If\n");
            if (node->as.if_stmt.condition) {
                for (int i = 0; i < level + 1; i++) printf("  ");
                printf("Condition:\n");
                print_ast(node->as.if_stmt.condition, level + 2);
            }
            if (node->as.if_stmt.then_branch) {
                for (int i = 0; i < level + 1; i++) printf("  ");
                printf("Then:\n");
                print_ast(node->as.if_stmt.then_branch, level + 2);
            }
            if (node->as.if_stmt.else_branch) {
                for (int i = 0; i < level + 1; i++) printf("  ");
                printf("Else:\n");
                print_ast(node->as.if_stmt.else_branch, level + 2);
            }
            break;
            
        case AST_FOR:
            printf("For\n");
            if (node->as.for_stmt.init) {
                for (int i = 0; i < level + 1; i++) printf("  ");
                printf("Init:\n");
                print_ast(node->as.for_stmt.init, level + 2);
            }
            if (node->as.for_stmt.condition) {
                for (int i = 0; i < level + 1; i++) printf("  ");
                printf("Condition:\n");
                print_ast(node->as.for_stmt.condition, level + 2);
            }
            if (node->as.for_stmt.update) {
                for (int i = 0; i < level + 1; i++) printf("  ");
                printf("Increment:\n");
                print_ast(node->as.for_stmt.update, level + 2);
            }
            if (node->as.for_stmt.body) {
                for (int i = 0; i < level + 1; i++) printf("  ");
                printf("Body:\n");
                print_ast(node->as.for_stmt.body, level + 2);
            }
            break;
            
        case AST_WHILE:
            printf("While\n");
            if (node->as.while_stmt.condition) {
                for (int i = 0; i < level + 1; i++) printf("  ");
                printf("Condition:\n");
                print_ast(node->as.while_stmt.condition, level + 2);
            }
            if (node->as.while_stmt.body) {
                for (int i = 0; i < level + 1; i++) printf("  ");
                printf("Body:\n");
                print_ast(node->as.while_stmt.body, level + 2);
            }
            break;
            
        case AST_STRUCT_DECL:
            printf("Struct %s\n", node->as.struct_decl.name);
            for (int i = 0; i < node->as.struct_decl.member_count; i++)
                print_ast(node->as.struct_decl.members[i], level + 1);
            break;
            
        case AST_MEMBER_ACCESS:
            printf("MemberAccess(.%s)\n", node->as.member_access.member);
            if (node->as.member_access.object)
                print_ast(node->as.member_access.object, level + 1);
            break;
            
        case AST_CAST:
            printf("Cast(to %s", token_type_name(node->as.cast.target_type.base_type));
            if (node->as.cast.target_type.pointer_level > 0) {
                for (int i = 0; i < node->as.cast.target_type.pointer_level; i++)
                    printf("*");
            }
            printf(")\n");
            if (node->as.cast.expr)
                print_ast(node->as.cast.expr, level + 1);
            break;
            
        case AST_STRING_LITERAL:
            printf("StringLiteral(\"%s\", length: %d)\n", 
                node->as.string_literal.value ? node->as.string_literal.value : "",
                node->as.string_literal.length);
            break;
            
        case AST_CHAR_LITERAL:
            printf("CharLiteral('%c')\n", node->as.char_literal.value);
            break;
            
        case AST_INT_LITERAL:
            printf("IntLiteral(%lld)\n", node->as.int_literal.value);
            break;
            
        case AST_FLOAT_LITERAL:
            printf("FloatLiteral(%f)\n", node->as.float_literal.value);
            break;
            
        case AST_DOUBLE_LITERAL:
            printf("DoubleLiteral(%lf)\n", node->as.double_literal.value);
            break;
            
        default:
            printf("UnknownNodeType(%d)\n", node->type);
            break;
    }
}