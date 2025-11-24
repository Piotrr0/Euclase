#include "parser.h"
#include "ast_layout.h"
#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
            free(node->as.struct_decl.type);
            free_node_array(node->as.struct_decl.members, node->as.struct_decl.member_count);
            break;
            
        default:
            break;
    }
    
    free(node);
}

void init_parser(Parser* parser, Tokens* tokens) {
    if (parser == NULL || tokens == NULL)
        return;

    parser->tokens = tokens;
    parser->current_token = 0;
}

void advance(Parser* parser) {
    if (parser == NULL)
        return;

    if (parser->current_token < parser->tokens->token_count - 1)
        parser->current_token++;
}

Token* current_token(Parser* parser)
{
    if (parser->current_token >= parser->tokens->token_count)
        return &parser->tokens->tokens[parser->tokens->token_count - 1];

    return &parser->tokens->tokens[parser->current_token];
}

Token* peek_token(Parser* parser, int offset)
{
    int pos = parser->current_token + offset;
    if (pos >= parser->tokens->token_count)
        return &parser->tokens->tokens[parser->tokens->token_count - 1];

    return &parser->tokens->tokens[pos];
}

int match(Parser* parser, TokenType type) {
    if (current_token(parser)->type == type) {
        advance(parser);
        return 1;
    }
    return 0;
}

int check(Parser* parser, TokenType type) {
    return current_token(parser)->type == type;
}

int is_type(Parser* parser, TokenType t) {
    if (t == TOK_IDENTIFIER && peek_token(parser, 1)->type == TOK_IDENTIFIER)
        return 1;

    switch(t) {
        case TOK_INT: case TOK_UINT: case TOK_FLOAT: case TOK_UFLOAT:
        case TOK_DOUBLE: case TOK_UDOUBLE: case TOK_CHAR: case TOK_UCHAR:
        case TOK_VOID: return 1;
        default: return 0;
    }
}

int parse_pointer_level(Parser* parser) {
    int level = 0;
    while (match(parser, TOK_MULTIPLICATION)) {
        level++;
    }
    return level;
}

ASTNode* parse_compound_operators(Parser* parser) {
    ASTNode* lhs = parse_expression(parser);
    if (lhs == NULL)
        return NULL;

    if (lhs->type != AST_IDENTIFIER && lhs->type != AST_MEMBER_ACCESS && (lhs->type != AST_UNARY_OP || lhs->as.unary_op.op != OP_DEREF)) {
        free_ast(lhs);
        return NULL;
    }

    TokenType op_tok = current_token(parser)->type;
    BinaryOp op;

    switch(op_tok) {
        case TOK_ASSIGNMENT_ADDITION:       op = OP_ADD; break;
        case TOK_ASSIGNMENT_SUBTRACTION:    op = OP_SUB; break;
        case TOK_ASSIGNMENT_MULTIPLICATION: op = OP_MUL; break;
        case TOK_ASSIGNMENT_DIVISION:       op = OP_DIV; break;
        case TOK_ASSIGNMENT_MODULO:         op = OP_MOD; break;
        default: free_ast(lhs); return NULL;
    }

    advance(parser);
    ASTNode* rhs = parse_expression(parser);
    if (rhs == NULL) {
        free_ast(lhs);
        return NULL;
    }

    if (!match(parser, TOK_SEMICOLON)) {
        free_ast(lhs);
        free_ast(rhs);
        return NULL;
    }
    
    ASTNode* lhs_copy = NULL;
    if (lhs->type == AST_IDENTIFIER) {
        lhs_copy = create_identifier_node(strdup(lhs->as.identifier.name), current_token(parser)->line, current_token(parser)->column);
    }
    
    if (lhs_copy == NULL) {
        free_ast(lhs);
        free_ast(rhs);
        return NULL;
    }
    
    ASTNode* binary_node = create_binary_op_node(op, lhs_copy, rhs, current_token(parser)->line, current_token(parser)->column);
    ASTNode* assign_node = create_assign_node(lhs, binary_node, current_token(parser)->line, current_token(parser)->column);
    return assign_node;
}

ASTNode* parse_expression(Parser* parser) 
{
    return parse_assignment(parser);
}

ASTNode* parse_assignment(Parser* parser)
{
    ASTNode* left = parse_equality(parser);
    if (left == NULL) 
        return NULL;
    
    while (check(parser, TOK_ASSIGNMENT) || is_compound_token(current_token(parser)->type))
    {
        if (left->type != AST_IDENTIFIER && left->type != AST_MEMBER_ACCESS && (left->type != AST_UNARY_OP || left->as.unary_op.op != OP_DEREF)) {
            free_ast(left);
            return NULL;
        }

        TokenType op_tok = current_token(parser)->type;
        advance(parser);
        
        ASTNode* right = parse_assignment(parser);
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
            case TOK_ASSIGNMENT:                return create_assign_node(left, right, current_token(parser)->line, current_token(parser)->column);
            default: free_ast(left); free_ast(right); return NULL;
        }
        
        if (left->type != AST_IDENTIFIER && left->type != AST_MEMBER_ACCESS && (left->type != AST_UNARY_OP || left->as.unary_op.op != OP_DEREF)) {
            free_ast(left);
            return NULL;
        }

        ASTNode* left_copy = create_identifier_node(strdup(left->as.identifier.name), current_token(parser)->line, current_token(parser)->column);
        if (left_copy == NULL) {
            free_ast(left);
            free_ast(right);
            return NULL;
        }
        
        ASTNode* binary_node = create_binary_op_node(op, left_copy, right, current_token(parser)->line, current_token(parser)->column);
        return create_assign_node(left, binary_node, current_token(parser)->line, current_token(parser)->column);
    }
    
    return left;
}

ASTNode* parse_equality(Parser* parser)
{
    ASTNode* left = parse_additive(parser);
    if (left == NULL) 
        return NULL;

    while (check(parser, TOK_EQUAL) || check(parser, TOK_NOT_EQUAL) || check(parser, TOK_LESS) || check(parser, TOK_GREATER) || check(parser, TOK_LESS_EQUALS) || check(parser, TOK_GREATER_EQUALS))
    {
        TokenType op = current_token(parser)->type;
        advance(parser);

        ASTNode* right = parse_additive(parser);
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
        
        left = create_binary_op_node(binary_op, left, right, current_token(parser)->line, current_token(parser)->column);
    }
    
    return left;
}

ASTNode* parse_additive(Parser* parser)
{
    ASTNode* left = parse_multiplicative(parser);
    if (left == NULL) 
        return NULL;

    while (check(parser, TOK_ADDITION) || check(parser, TOK_SUBTRACTION))
    {
        TokenType op = current_token(parser)->type;
        advance(parser);
        
        ASTNode* right = parse_multiplicative(parser);
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

        left = create_binary_op_node(binary_op, left, right, current_token(parser)->line, current_token(parser)->column);
    }
    
    return left;
}

ASTNode* parse_multiplicative(Parser* parser) 
{
    ASTNode* left = parse_unary(parser);
    if (left == NULL) 
        return NULL;
    
    while (check(parser, TOK_MULTIPLICATION) || check(parser, TOK_DIVISION) || check(parser, TOK_MODULO))
    {
        TokenType op = current_token(parser)->type;
        advance(parser);

        ASTNode* right = parse_unary(parser);
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
        
        left = create_binary_op_node(binary_op, left, right, current_token(parser)->line, current_token(parser)->column);
    }

    return left;
}

ASTNode* parse_negation(Parser* parser) {
    advance(parser);
    ASTNode* unary_expression = parse_unary(parser);
    if (unary_expression == NULL) {
        return NULL;
    }

    ASTNode* unary_minus_node = create_unary_op_node(OP_NEG, unary_expression, current_token(parser)->line, current_token(parser)->column);
    if (unary_minus_node == NULL) {
        free_ast(unary_expression);
        return NULL;
    }

    return unary_minus_node;
}

ASTNode* parse_pre_decrement(Parser* parser) {
    if (!match(parser, TOK_DECREMENT))
        return NULL;

    ASTNode* node = parse_unary(parser);
    if (node == NULL)
        return NULL;

    ASTNode* dec = create_unary_op_node(OP_PRE_DEC, node, current_token(parser)->line, current_token(parser)->column);
    if (dec == NULL)
        free_ast(node);
    
    return dec;
}

ASTNode* parse_pre_increment(Parser* parser) {
    if (!match(parser, TOK_INCREMENT))
        return NULL;

    ASTNode* node = parse_unary(parser);
    if (node == NULL)
        return NULL;

    ASTNode* inc = create_unary_op_node(OP_PRE_INC, node, current_token(parser)->line, current_token(parser)->column);
    if (inc == NULL)
        free_ast(node);
    
    return inc;
}

ASTNode* parse_post_increment(Parser* parser, ASTNode* operand) {
    if (!match(parser, TOK_INCREMENT))
        return operand;

    ASTNode* inc = create_unary_op_node(OP_POST_INC, operand, current_token(parser)->line, current_token(parser)->column);
    if (inc == NULL) {
        free_ast(operand);
        return NULL;
    }
    
    return inc;
}

ASTNode* parse_post_decrement(Parser* parser, ASTNode* operand) {
    if (!match(parser, TOK_DECREMENT))
        return operand;

    ASTNode* dec = create_unary_op_node(OP_POST_DEC, operand, current_token(parser)->line, current_token(parser)->column);
    if (dec == NULL) {
        free_ast(operand);
        return NULL;
    }
    
    return dec;
}

ASTNode* parse_unary(Parser* parser)
{
    switch (current_token(parser)->type) {
        case TOK_SUBTRACTION:       return parse_negation(parser);
        case TOK_MULTIPLICATION:    return parse_dereference(parser);
        case TOK_AMPERSAND:         return parse_address_of(parser);
        case TOK_INCREMENT:         return parse_pre_increment(parser);
        case TOK_DECREMENT:         return parse_pre_decrement(parser);

        default: break;
    }

    if (is_casting(parser))
        return parse_casting(parser);

    return parse_postfix(parser);
}

ASTNode* parse_postfix(Parser* parser)
{
    ASTNode* node = NULL;
    if (is_func_call(parser))
        node = parse_function_call(parser);
    else
         node = parse_primary(parser);

    while (check(parser, TOK_DOT)) {
        advance(parser);
        
        if (!check(parser, TOK_IDENTIFIER)) {
            free_ast(node);
            return NULL;
        }
        
        ASTNode* member_access = create_member_access_node(node, sv_to_owned_cstr(current_token(parser)->lexeme), current_token(parser)->line, current_token(parser)->column);
        if (member_access == NULL) {
            free_ast(node);
            return NULL;
        }

        advance(parser);
        node = member_access;
    }
    
    if (check(parser, TOK_INCREMENT))
        node = parse_post_increment(parser, node);
    else if (check(parser, TOK_DECREMENT))
        node = parse_post_decrement(parser, node);
    
    return node;
}

ASTNode* parse_primary(Parser* parser)
{
    if (check(parser, TOK_IDENTIFIER))
        return parse_identifier_expression(parser);
    
    return parse_primary_expression(parser);
}

ASTNode* parse_parens(Parser* parser) {
    if (!check(parser, TOK_LPAREN)) 
        return NULL;

    advance(parser);
    ASTNode* node = parse_expression(parser);
    if (node == NULL)
        return NULL;

    if (!match(parser, TOK_RPAREN)) {
        printf("Parse error: expected ')'\n");
        free_ast(node);
        return NULL;
    }
    return node;
}

ASTNode* parse_primary_expression(Parser* parser) {
    ASTNode* paren = parse_parens(parser);
    if(paren != NULL)
        return paren;

    switch (current_token(parser)->type)
    {
        case TOK_CHAR_LITERAL:      return parse_char_literal(parser);
        case TOK_STRING_LITERAL:    return parse_string_literal(parser);
        case TOK_NUMBER_INT:
        case TOK_NUMBER_FLOAT:
        case TOK_NUMBER_DOUBLE:     return parse_number_literal(parser);
        default: printf("Parse error: expected primary expression\n");
    }
    return NULL;
}

ASTNode* parse_number_literal(Parser* parser) {
    if (!(check(parser, TOK_NUMBER_INT) || check(parser, TOK_NUMBER_DOUBLE) || check(parser, TOK_NUMBER_FLOAT)))
        return NULL;
    
    const char* number = sv_to_owned_cstr(current_token(parser)->lexeme);

    ASTNode* result = NULL;
    switch (current_token(parser)->type)
    {
        case TOK_NUMBER_INT: {
            long long int_val = atoll(number);
            result = create_int_literal_node(int_val, current_token(parser)->line, current_token(parser)->column);
            break;
        }
        case TOK_NUMBER_FLOAT: {
            float float_val = atof(number);
            result = create_float_literal_node(float_val, current_token(parser)->line, current_token(parser)->column);
            break;
        }
        case TOK_NUMBER_DOUBLE: {
            double double_val = atof(number);
            result = create_double_literal_node(double_val, current_token(parser)->line, current_token(parser)->column);
            break;
        }

        default: break;
    }
    
    advance(parser);
    free((char*)number);
    return result;
}

ASTNode* parse_string_literal(Parser* parser) {
    if (!check(parser, TOK_STRING_LITERAL))
        return NULL;

    char* string = sv_to_owned_cstr(current_token(parser)->lexeme);
    advance(parser);

    return create_string_literal_node(string, current_token(parser)->line, current_token(parser)->column);;
}

ASTNode* parse_char_literal(Parser* parser) {
    if (!check(parser, TOK_CHAR_LITERAL))
        return NULL;

    char ch = current_token(parser)->lexeme.data[0];
    advance(parser);

    return create_char_literal_node(ch, current_token(parser)->line, current_token(parser)->column);
}

ASTNode* parse_identifier_expression(Parser* parser)
{
    ASTNode* node = create_identifier_node(sv_to_owned_cstr(current_token(parser)->lexeme), current_token(parser)->line, current_token(parser)->column);
    advance(parser);
    return node;
}

ASTNode* parse_dereference(Parser* parser)
{
    if(!check(parser, TOK_MULTIPLICATION))
        return NULL;

    advance(parser);
    ASTNode* expr = parse_unary(parser);
    if (expr == NULL) {
        return NULL;
    }

    ASTNode* node = create_unary_op_node(OP_DEREF, expr, current_token(parser)->line, current_token(parser)->column);
    if(node == NULL) {
        free_ast(expr);
        return NULL;
    }
    
    return node;
}

ASTNode* parse_address_of(Parser* parser) {
    if(!check(parser, TOK_AMPERSAND))
        return NULL;

    advance(parser);
    ASTNode* expr = parse_unary(parser);
    ASTNode* node = create_unary_op_node(OP_ADDR, expr, current_token(parser)->line, current_token(parser)->column);
    if (node == NULL) {
        free_ast(expr);
        return NULL;
    }

    return node;
}

// int x = sum(10, 10); 
// int x = sum(10, &a);
ASTNode* parse_function_call(Parser* parser)
{
    if(!check(parser, TOK_IDENTIFIER)) {
        return NULL;
    }

    char* name = sv_to_owned_cstr(current_token(parser)->lexeme);
    if (name == NULL)
        return NULL;

    advance(parser);

    if(!match(parser, TOK_LPAREN)) {
        free(name);
        return NULL;
    }

    ASTNode* func_call = create_func_call_node(name, current_token(parser)->line, current_token(parser)->column);
    if (func_call == NULL) {
        free(name);
        return NULL;
    }

    while (!check(parser, TOK_RPAREN) && !check(parser, TOK_EOF))
    {
        ASTNode* arg = parse_expression(parser);
        if(arg == NULL) {
            printf("Parse error: expected expression in function argument\n");
            return func_call;
        }

        add_arg_to_func_call(func_call, arg);

        if (!match(parser, TOK_COMMA)) {
            break;
        }
    }

    if(!match(parser, TOK_RPAREN)) {
        printf("Parse error: expected ')' after function arguments\n");
        return func_call;
    }

    return func_call;
}

// float pi = 3.14f;
// int v = (int) pi;
ASTNode* parse_casting(Parser* parser)
{
    if(!match(parser, TOK_LPAREN)) {
        printf("Parse error: expected: '('\n");
        return NULL;
    }

    if(!is_type(parser, current_token(parser)->type)) {
        printf("Parse error: expected: 'TYPE'\n");
        return NULL;
    }

    TypeInfo cast_type = parse_type(parser); 

    if(!match(parser, TOK_RPAREN)) {
        printf("Parse error: expected: ')'\n");
        return NULL;
    }

    ASTNode* expr = parse_unary(parser);
    if(expr == NULL) {
        printf("Parse error: expected expression after cast\n");
        return NULL;
    }

    ASTNode* cast_node = create_cast_node(cast_type, expr, current_token(parser)->line, current_token(parser)->column);
    if(cast_node == NULL) {
        free_ast(expr);
        printf("Memory allocation failed\n");
        return NULL;
    }

    return cast_node;
}

TypeInfo parse_type(Parser* parser)
{
    if (!is_type(parser, current_token(parser)->type)) {
        printf("Parse error: expected return type\n");
        exit(1);
    }

    TokenType ret_type = current_token(parser)->type;
    char* type = sv_to_owned_cstr(current_token(parser)->lexeme);
    advance(parser);

    int pointer_level = parse_pointer_level(parser);

    TypeInfo info = { 
        .base_type = ret_type, 
        .pointer_level = pointer_level, 
        .type = type 
    };

    return info;
}

int is_func_call(Parser* parser)
{
    if(!check(parser, TOK_IDENTIFIER))
        return 0;

    Token* lparen_token = peek_token(parser, 1);
    if(lparen_token->type != TOK_LPAREN) {
        return 0;
    }

    return 1;
}

int check_pointer_level(Parser* parser, int offset) 
{
    int starting_offset = offset; 
    Token* next_token = peek_token(parser, offset);

    while(next_token->type == TOK_MULTIPLICATION) {
        next_token = peek_token(parser, ++offset);
    }
    
    return offset - starting_offset;
}

int is_casting(Parser* parser) {
    if(!check(parser, TOK_LPAREN))
        return 0;

    Token* type_token = peek_token(parser, 1);
    if(!is_type(parser, type_token->type))
        return 0;

    int pointer_level = check_pointer_level(parser, 2);
    Token* rparen_token = peek_token(parser, 2 + pointer_level);

    return (rparen_token->type == TOK_RPAREN);
}

ASTNode* parse_while_loop(Parser* parser) {
    if (!match(parser, TOK_WHILE))
        return NULL;

    if (!match(parser, TOK_LPAREN))
        return NULL;

    ASTNode* condition = parse_expression(parser);
    if(condition == NULL) {
        free_ast(condition);
        return NULL;
    }

    if (!match(parser, TOK_RPAREN)) {
        free_ast(condition);
        return NULL;
    }

    ASTNode* body = parse_block(parser);
    if(body == NULL) {
        free_ast(condition);
        return NULL;
    }

    return create_while_node(condition, body, current_token(parser)->line, current_token(parser)->column);
}

ASTNode* parse_for_loop(Parser* parser) {
    if (!match(parser, TOK_FOR))
        return NULL;

    if (!match(parser, TOK_LPAREN))
        return NULL;

    ASTNode* init = NULL;
    if (!check(parser, TOK_SEMICOLON)) {
        init = parse_loop_init(parser);
        if (init == NULL) {
            return NULL;
        }
    }

    if (init == NULL || init->type != AST_VAR_DECL) {
        if (!match(parser, TOK_SEMICOLON)) {
            free_ast(init);
            return NULL;
        }
    }
        
    ASTNode* condition = NULL;
    if (!check(parser, TOK_SEMICOLON)) {
        condition = parse_expression(parser);
        if (condition == NULL) {
            free_ast(init);
            return NULL;
        }
    }

    if (!match(parser, TOK_SEMICOLON)) {
        free_ast(init);
        free_ast(condition);
        return NULL;
    }

    ASTNode* update = NULL;
    if (!check(parser, TOK_RPAREN)) {
        update = parse_loop_update(parser);
        if (update == NULL) {
            free_ast(init);
            free_ast(condition);
            return NULL;
        }
    }

    if(!match(parser, TOK_RPAREN)) {
        free_ast(init);
        free_ast(condition);
        free_ast(update);
        return NULL;
    }

    ASTNode* body = parse_block(parser);
    if(body == NULL) {
        free_ast(init);
        free_ast(condition);
        free_ast(update);
        return NULL;
    }

    return create_for_node(init, condition, update, body, current_token(parser)->line, current_token(parser)->column);
}

ASTNode* parse_loop_init(Parser* parser) {
    ASTNode* init = NULL;

    if (is_type(parser, current_token(parser)->type))
        init = parse_variable_declaration(parser);
    else if (check(parser, TOK_IDENTIFIER) || check(parser, TOK_MULTIPLICATION))
        init = parse_assignment(parser);

    return init;
}

ASTNode* parse_loop_condition(Parser* parser) {

    ASTNode* condition = NULL;
    if (!check(parser, TOK_SEMICOLON))
    {
        condition = parse_expression(parser);
        if(condition == NULL)
            return NULL;
    }
    
    if (!match(parser, TOK_SEMICOLON)) {
        free_ast(condition);
        return NULL;
    }

    return condition;
}

ASTNode* parse_loop_update(Parser* parser)
{
    ASTNode* lhs = parse_expression(parser);
    if (lhs == NULL)
        return NULL;
    
    if (!match(parser, TOK_ASSIGNMENT)) {
        return lhs;
    }
    
    ASTNode* rhs = parse_expression(parser);
    if (rhs == NULL) {
        free_ast(lhs);
        return NULL;
    }
    
    return create_assign_node(lhs, rhs, current_token(parser)->line, current_token(parser)->column);
}

ASTNode* parse_if(Parser* parser) {
    if(!match(parser, TOK_IF))
        return NULL;

    if(!match(parser, TOK_LPAREN))
        return NULL;

    ASTNode* condition = parse_expression(parser);
    if(condition == NULL)
        return NULL;

    if(!match(parser, TOK_RPAREN)) {
        free_ast(condition);
        return NULL;
    }
    
    ASTNode* then_branch = parse_block(parser);
    if(then_branch == NULL) {
        free_ast(condition);
        return NULL;
    }
    
    if (!check(parser, TOK_ELSE))
        return create_if_node(condition, then_branch, NULL, current_token(parser)->line, current_token(parser)->column);

    advance(parser);

    ASTNode* else_branch = parse_block(parser);
    if (else_branch == NULL) {
        free_ast(condition);
        free_ast(then_branch);
        return NULL;
    }
    
    return create_if_node(condition, then_branch, else_branch, current_token(parser)->line, current_token(parser)->column);
}

ASTNode* parse_else(Parser* parser) {
    if(!match(parser, TOK_ELSE))
        return NULL;

    ASTNode* node_else = parse_block(parser);
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

ASTNode* parse_variable_declaration(Parser* parser) {
    TypeInfo type = parse_type(parser);
    if (current_token(parser)->type != TOK_IDENTIFIER) {
        printf("Parse error: expected variable name\n");
        return NULL;
    }
    
    char* name = sv_to_owned_cstr(current_token(parser)->lexeme);
    if (name == NULL)
        return NULL;

    advance(parser);

    ASTNode* expr = NULL;
    if (match(parser, TOK_ASSIGNMENT))
    {
        expr = parse_expression(parser);
        if(expr == NULL) {
            free(name);
            return NULL;
        }
    }
    
    if (!match(parser, TOK_SEMICOLON)) {
        printf("Parse error: expected ';'\n");
        free(name);
        free_ast(expr);
        return NULL;
    }

    return create_var_decl_node(name, type, expr, current_token(parser)->line, current_token(parser)->column);
}

ASTNode* parse_struct_member(Parser* parser) {
    TypeInfo type = parse_type(parser);
    
    if (!check(parser, TOK_IDENTIFIER))
        return NULL;
    
    char* name = sv_to_owned_cstr(current_token(parser)->lexeme);
    advance(parser);
    
    if (!match(parser, TOK_SEMICOLON)) {
        free(name);
        return NULL;
    }
    
    return create_var_decl_node(name, type, NULL, current_token(parser)->line, current_token(parser)->column);
}

char* parse_struct_name(Parser* parser) {
    if (!match(parser, TOK_STRUCT))
        return NULL;
    
    if (!check(parser, TOK_IDENTIFIER))
        return NULL;
    
    char* name = sv_to_owned_cstr(current_token(parser)->lexeme);
    advance(parser);

    return name;
}

ASTNode* parse_struct_declaration(Parser* parser) {
    if (!check(parser, TOK_STRUCT))
        return NULL;
    
    char* name = parse_struct_name(parser);

    if (!match(parser, TOK_LBRACE)) {
        free(name);
        return NULL;
    }
    
    ASTNode* struct_decl = create_struct_decl_node(name, current_token(parser)->line, current_token(parser)->column);
    if (struct_decl == NULL) {
        free(name);
        return NULL;
    }
    
    while (!check(parser, TOK_RBRACE) && !check(parser, TOK_EOF))
    {
        if (is_type(parser, current_token(parser)->type)) {
            ASTNode* member = parse_struct_member(parser);
            if(member == NULL)
                continue;

            add_member_to_struct(struct_decl, member);
        }
        else
            advance(parser);
    }
    
    if (!match(parser, TOK_RBRACE)) {
        free_ast(struct_decl);
        return NULL;
    }

    if (!match(parser, TOK_SEMICOLON)) {
        free_ast(struct_decl);
        return NULL;
    }

    return struct_decl;
}

ASTNode* parse_return(Parser* parser) {
    if (!match(parser, TOK_RETURN)) {
        printf("Parse error: expected 'return'\n");
        return NULL;
    }

    ASTNode* expr = NULL;
    if (!check(parser, TOK_SEMICOLON)) {
        expr = parse_expression(parser);
        if(expr == NULL) {
            return NULL;
        }
    }

    if (!match(parser, TOK_SEMICOLON)) {
        printf("Parse error: expected ';' after return\n");
        free_ast(expr);
        return NULL;
    }
    return create_return_node(expr, current_token(parser)->line, current_token(parser)->column);
}

ASTNode* parse_statement(Parser* parser) {

    switch (current_token(parser)->type)
    {
        case TOK_RETURN:            return parse_return(parser);
        case TOK_IF:                return parse_if(parser);
        case TOK_FOR:               return parse_for_loop(parser);
        case TOK_WHILE:             return parse_while_loop(parser);
        case TOK_LBRACE:            return parse_block(parser);
        case TOK_STRUCT:            return parse_struct_declaration(parser);
        default: break;
    }

    if (is_type(parser, current_token(parser)->type))
        return parse_variable_declaration(parser);

    ASTNode* node = parse_expression(parser);
    if (node == NULL)
        return NULL;

    if (!match(parser, TOK_SEMICOLON)) {
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

ASTNode* parse_block(Parser* parser)
{
    if (!match(parser, TOK_LBRACE)) {
        printf("Parse error: expected '{'\n");
        return NULL;
    }

    ASTNode* block = create_block_node(current_token(parser)->line, current_token(parser)->column);
    if (block == NULL)
        return NULL;

    while (!check(parser, TOK_RBRACE) && !check(parser, TOK_EOF)) {
        ASTNode* stmt = parse_statement(parser);
        if(stmt == NULL)
            continue;

        add_statement_to_block(block, stmt);
    }

    if (!match(parser, TOK_RBRACE)) {
        printf("Parse error: expected '}'\n");
        free(block);
        return NULL;
    }
    return block;
}

int is_func_declaration(Parser* parser) {
    if(!is_type(parser, current_token(parser)->type))
        return 0;

    Token* next_token = peek_token(parser, 1);
    if (next_token->type != TOK_IDENTIFIER)
        return 0;

    Token* third_token = peek_token(parser, 2);
    return (third_token->type == TOK_LPAREN);
}

ASTNode* parse_parameters(Parser* parser, ASTNode* func)
{
    if (!match(parser, TOK_LPAREN)) {
        printf("Parse error: expected '('\n");
        return NULL;
    }

    while (!check(parser, TOK_RPAREN) && !check(parser, TOK_EOF)) 
    {
        if (!is_type(parser, current_token(parser)->type)) {
            printf("Parse error: expected type in parameter list\n");
            return NULL;
        }

        TypeInfo type = parse_type(parser);
        if (!check(parser, TOK_IDENTIFIER)) {
            printf("Parse error: expected parameter name\n");
            return NULL;
        }

        char* param_name = sv_to_owned_cstr(current_token(parser)->lexeme);
        if(param_name == NULL)
            return NULL;

        advance(parser);

        ASTNode* param = create_param_node(param_name, type, current_token(parser)->line, current_token(parser)->column);
        if (param == NULL)
            return NULL;

        add_param_to_function(func, param);

        if (!match(parser, TOK_COMMA))
            break;
    }

    if (!match(parser, TOK_RPAREN)) {
        printf("Parse error: expected ')'\n");
        return NULL;
    }

    return func;
}

ASTNode* parse_function(Parser* parser)
{
    TypeInfo return_type = parse_type(parser);
    if (!check(parser, TOK_IDENTIFIER)) {
        printf("Parse error: expected function name\n");
        return NULL;
    }

    char* name = sv_to_owned_cstr(current_token(parser)->lexeme);
    if(name == NULL)
        return NULL;

    advance(parser);

    ASTNode* func = create_function_node(name, return_type, current_token(parser)->line, current_token(parser)->column);
    if (func == NULL) {
        free(name);
        return NULL;
    }
    
    if (parse_parameters(parser, func) == NULL) {
        free_ast(func);
        return NULL;
    }
    
    ASTNode* body = parse_block(parser);
    if(body == NULL) {
        free_ast(func);
        return NULL;
    }

    func->as.function.body = body;
    return func;
}

/* namespace main */
char* parse_namespace_name(Parser* parser)
{
    if (!match(parser, TOK_NAMESPACE)) {
        printf("Parse error: expected 'namespace'\n");
        return NULL;
    }
    
    if (!check(parser, TOK_IDENTIFIER)) {
        printf("Parse error: expected namespace name\n");
        return NULL;
    }
    
    char* namespace_name = sv_to_owned_cstr(current_token(parser)->lexeme);
    advance(parser);

    return namespace_name;
}

ASTNode* parse_program(Parser* parser) {
    if(!check(parser, TOK_NAMESPACE))
        return NULL;

    char* namespace_name = parse_namespace_name(parser);

    if (!match(parser, TOK_LBRACE)) {
        printf("Parse error: expected '{'\n");
        free(namespace_name);
        return NULL;
    }
    
    ASTNode* program = create_program_node(namespace_name, current_token(parser)->line, current_token(parser)->column);
    if (program == NULL) {
        free(namespace_name);
        return NULL;
    }

    while (!check(parser, TOK_RBRACE) && !check(parser, TOK_EOF)) {

        if (is_func_declaration(parser))
        {
            ASTNode* func = parse_function(parser);
            if(func == NULL)
                continue;

            add_function_to_program(program, func);
        }
        else if (check(parser, TOK_STRUCT)) {
            ASTNode* struct_node = parse_struct_declaration(parser);
            if(struct_node == NULL)
                continue;

            add_struct_to_program(program, struct_node);
        }
        else if (is_type(parser, current_token(parser)->type))
        {
            ASTNode* var_decl = parse_variable_declaration(parser);
            if(var_decl == NULL)
                continue;

            add_global_to_program(program, var_decl);
        }
        else
        {
            printf("Parse error: unexpected token in namespace\n");
            advance(parser);
        }
    }

    if (!match(parser, TOK_RBRACE)) {
        printf("Parse error: expected '}'\n");
        return NULL;
    }

    return program;
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
                node->as.var_decl.type.type,
                node->as.var_decl.name, 
                node->as.var_decl.type.pointer_level);
            if (node->as.var_decl.initializer)
                print_ast(node->as.var_decl.initializer, level + 1);
            break;

        case AST_UNARY_OP: {
                const char* op_name = NULL;
                switch (node->as.unary_op.op) {
                    case OP_DEREF:      op_name = "Dereference"; break;
                    case OP_ADDR:       op_name = "Address Of"; break;
                    case OP_NEG:        op_name = "Negation"; break;
                    case OP_POST_INC:   op_name = "Post Increment"; break;
                    case OP_POST_DEC:   op_name = "Post Decrement"; break;
                    case OP_PRE_INC:    op_name = "Pre Increment"; break;
                    case OP_PRE_DEC:    op_name = "Pre Decrement"; break;
                    default:            op_name = "Unknown"; break;
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
            printf("Struct %s\n", node->as.struct_decl.type);
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