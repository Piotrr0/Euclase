#include "parser.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Parser parser;

ASTNode* new_node(ASTNodeType type) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
            return NULL;

    node->type = type;
    node->name = NULL;
    node->children = NULL;
    node->child_count = 0;

    node->type_info.base_type = TOK_VOID;
    node->type_info.pointer_level = 0;
    return node;
}

void add_child(ASTNode* parent, ASTNode* child) {
    if(parent == NULL)
        return;

    if(child == NULL)
        return;

    parent->children = realloc(parent->children, sizeof(ASTNode*) * (parent->child_count + 1));
    parent->children[parent->child_count++] = child;
}

void free_ast(ASTNode* node)
{
    if(node == NULL)
        return;

    if(node->name != NULL) {
        free(node->name);
        node->name = NULL;        
    }

    for (int i = 0; i < node->child_count; i++) {
        free_ast(node->children[i]);
    }

    if (node->children != NULL) 
    {
        free(node->children);
        node->children = NULL;
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

    if (lhs->type != AST_IDENTIFIER && lhs->type != AST_DEREFERENCE) {
        free_ast(lhs);
        return NULL;
    }

    TokenType op = current_token()->type;
    ASTNodeType binary_op;
    
    switch(op) {
        case TOK_ASSIGNMENT_ADDITION:       binary_op = AST_ADDITION; break;
        case TOK_ASSIGNMENT_SUBTRACTION:    binary_op = AST_SUBTRACTION; break;
        case TOK_ASSIGNMENT_MULTIPLICATION: binary_op = AST_MULTIPLICATION; break;
        case TOK_ASSIGNMENT_DIVISION:       binary_op = AST_DIVISION; break;
        case TOK_ASSIGNMENT_MODULO:         binary_op = AST_MODULO; break;
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

    ASTNode* binary_node = new_node(binary_op);
    if (binary_node == NULL) {
        free_ast(lhs);
        free_ast(rhs);
        return NULL;
    }
    
    ASTNode* lhs_copy = new_node(AST_IDENTIFIER);
    if (lhs_copy == NULL) {
        free_ast(lhs);
        free_ast(rhs);
        free_ast(binary_node);
        return NULL;
    }
    lhs_copy->name = strdup(lhs->name);
    lhs_copy->type_info = lhs->type_info;
    
    add_child(binary_node, lhs_copy);
    add_child(binary_node, rhs);

    ASTNode* assign_node = new_node(AST_ASSIGN);
    if (assign_node == NULL) {
        free_ast(lhs);
        free_ast(binary_node);
        return NULL;
    }
    
    add_child(assign_node, lhs);
    add_child(assign_node, binary_node);

    return assign_node;
}

ASTNode* parse_expression() 
{
    return parse_equality();
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
        
        ASTNodeType node_type;
        switch(op) 
        {
            case TOK_EQUAL:             node_type = AST_EQUAL; break;
            case TOK_NOT_EQUAL:         node_type = AST_NOT_EQUAL; break;
            case TOK_LESS:              node_type = AST_LESS; break;
            case TOK_GREATER:           node_type = AST_GREATER; break;
            case TOK_LESS_EQUALS:       node_type = AST_LESS_EQUAL; break;
            case TOK_GREATER_EQUALS:    node_type = AST_GREATER_EQUAL; break;
            default:                    node_type = AST_EXPRESSION; break;
        }

        ASTNode* binary_node = new_node(node_type);
        add_child(binary_node, left);
        add_child(binary_node, right);
        
        left = binary_node;
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
        
        ASTNodeType node_type;
        switch(op) 
        {
            case TOK_ADDITION:      node_type = AST_ADDITION; break;
            case TOK_SUBTRACTION:   node_type = AST_SUBTRACTION; break;
            default:                node_type = AST_EXPRESSION; break;
        }

        ASTNode* binary_node = new_node(node_type);
        add_child(binary_node, left);
        add_child(binary_node, right);
        
        left = binary_node;
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
        
        ASTNodeType node_type;
        switch(op) {
            case TOK_MULTIPLICATION:    node_type = AST_MULTIPLICATION; break;
            case TOK_DIVISION:          node_type = AST_DIVISION; break;
            case TOK_MODULO:            node_type = AST_MODULO; break;
            default:                    node_type = AST_EXPRESSION; break;
        }
        
        ASTNode* binary_node = new_node(node_type);
        add_child(binary_node, left);
        add_child(binary_node, right);
        
        left = binary_node;
    }
    
    return left;
}

ASTNode* parse_negative_unary() {
    advance();
    ASTNode* unary_expression = parse_unary();
    if (unary_expression == NULL) {
        return NULL;
    }
        
    ASTNode* unary_minus_node = new_node(AST_UNARY_MINUS);
    if (unary_minus_node == NULL) {
        free_ast(unary_expression);
        return NULL;
    }
    add_child(unary_minus_node, unary_expression);
    return unary_minus_node;
}

ASTNode* parse_unary()
{
    if(check(TOK_SUBTRACTION))
        return parse_negative_unary();

    if (check(TOK_MULTIPLICATION))
        return parse_dereference();

    if (check(TOK_AMPERSAND)) 
        return parse_address_of();

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
        
        ASTNode* member_access = new_node(AST_MEMBER_ACCESS);
        member_access->name = strdup(current_token()->lexme);
        advance();
        
        add_child(member_access, node);
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

    ASTNode* node = new_node(AST_EXPRESSION);
    if (node == NULL)
        return NULL;
    
    node->name = strdup(current_token()->lexme);
    node->type_info.base_type = current_token()->type;
    node->type_info.pointer_level = 0;

    advance();
    return node;
}

ASTNode* parse_string_literal() {
    if (!check(TOK_STRING_LITERAL))
        return NULL;

    ASTNode* node = new_node(AST_STRING_LITERAL);
    if (node == NULL)
        return NULL;

    node->name = strdup(current_token()->lexme);
    node->type_info.base_type = TOK_CHAR;
    node->type_info.pointer_level = 1;

    advance();
    return node;
}

ASTNode* parse_char_literal() {
    if (!check(TOK_CHAR_LITERAL))
        return NULL;

    ASTNode* node = new_node(AST_CHAR_LITERAL);
    if (node == NULL)
        return NULL;

    node->name = strdup(current_token()->lexme);
    node->type_info.base_type = TOK_CHAR;
    node->type_info.pointer_level = 0;

    advance();
    return node;
}

ASTNode* parse_identifier_expression()
{
    ASTNode* node = new_node(AST_IDENTIFIER);
    if (!check(TOK_IDENTIFIER)) {
        return NULL;
    }

    node->name = strdup(current_token()->lexme);
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

    ASTNode* node = new_node(AST_DEREFERENCE);
    if(node == NULL)
        return NULL;

    add_child(node, expr);
    return node;
}

ASTNode* parse_address_of() {
    if(!check(TOK_AMPERSAND))
        return NULL;

    advance();
    ASTNode* node = new_node(AST_ADDRESS_OF);
    ASTNode* expr = parse_unary();
    if (expr == NULL) {
        free_ast(node);
        return NULL;
    }

    add_child(node, expr);
    return node;
}

// int x = sum(10, 10); 
// int x = sum(10, &a);
ASTNode* parse_function_call()
{
    if(!check(TOK_IDENTIFIER)) {
        return NULL;
    }

    char* name = strdup(current_token()->lexme);
    advance();

    if(!match(TOK_LPAREN)) {
        free(name);
        return NULL;
    }

    ASTNode* func_call_node = new_node(AST_FUNC_CALL);
    if(func_call_node == NULL) {
        free(name);
        return NULL;
    }
    func_call_node->name = name;

    while (!check(TOK_RPAREN) && !check(TOK_EOF))
    {
        ASTNode* arg = parse_expression();
        if(arg == NULL) {
            printf("Parse error: expected expression in function argument\n");
            return func_call_node;
        }

        add_child(func_call_node, arg);

        if (!match(TOK_COMMA)) {
            break;
        }
    }

    if(!match(TOK_RPAREN)) {
        printf("Parse error: expected ')' after function arguments\n");
        return func_call_node;
    }

    return func_call_node;
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

    ASTNode* cast_node = new_node(AST_CAST);
    if(cast_node == NULL) {
        free_ast(expr);
        printf("Memory allocation failed\n");
        return NULL;
    }

    cast_node->type_info = cast_type;
    add_child(cast_node, expr);

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

    ASTNode* while_node = new_node(AST_WHILE);
    if (while_node == NULL)
        return NULL;

    ASTNode* condition = parse_expression();
    if(condition == NULL) {
        free_ast(while_node);
        return NULL;
    }

    add_child(while_node, condition);

    if (!match(TOK_RPAREN)) {
        free_ast(while_node);
        return NULL;
    }

    ASTNode* body = parse_block();
    if(body == NULL) {
        free_ast(while_node);
        return NULL;
    }

    add_child(while_node, body);
    return while_node;
}

ASTNode* parse_for_loop() {
    if (!match(TOK_FOR))
        return NULL;

    if (!match(TOK_LPAREN))
        return NULL;

    ASTNode* node_for = new_node(AST_FOR);
    if(node_for == NULL)
        return NULL;

    ASTNode* init = parse_loop_init();
    if (init == NULL) {
        free_ast(node_for);
        return NULL;
    }
        
    add_child (node_for, init);

    ASTNode* condition = parse_loop_condition();
    if (condition == NULL) {
        free_ast(node_for);
        return NULL;
    }

    add_child (node_for, condition);

    ASTNode* update = parse_loop_update();
    if(update == NULL) {
        free_ast(node_for);
        return NULL;
    }

    add_child(node_for, update);

    if(!match(TOK_RPAREN)) {
        free_ast(node_for);
        return NULL;
    }

    ASTNode* body = parse_block();
    if(body == NULL) {
        free_ast(node_for);
        return NULL;
    }

    add_child(node_for, body);
    return node_for;
}

ASTNode* parse_loop_init() {
    ASTNode* init = NULL;

    if (match(TOK_SEMICOLON)) {
        return new_node(AST_EXPRESSION);
    }

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
    else
        condition = new_node(AST_EXPRESSION);
    
    if (!match(TOK_SEMICOLON))
        return NULL;

    return condition;
}

ASTNode* parse_loop_update()
{
    if (check(TOK_RPAREN))
        return new_node(AST_EXPRESSION);

    ASTNode* lhs = parse_expression();
    if (lhs == NULL)
        return new_node(AST_EXPRESSION);

    if (!match(TOK_ASSIGNMENT))
        return lhs;

    ASTNode* rhs = parse_expression();
    if (rhs == NULL) {
        free_ast(lhs);
        return new_node(AST_EXPRESSION);
    }

    ASTNode* update = new_node(AST_ASSIGN);
    add_child(update, lhs);
    add_child(update, rhs);
    return update;
}


ASTNode* parse_condition() {
    if(!check(TOK_IF))
        return NULL;

    ASTNode* node_if = parse_if();
    if(node_if == NULL)
        return NULL;

    if(!check(TOK_ELSE))
        return node_if;

    ASTNode* node_else = parse_else();
    if(node_else == NULL) {
        free_ast(node_if);
        return NULL;
    }

    add_child(node_if, node_else);
    return node_if;
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

    ASTNode* node_if = new_node(AST_IF);
    if(node_if == NULL) {
        free_ast(condition);
        return NULL;
    }

    add_child(node_if, condition);

    ASTNode* body = parse_block();
    if(body == NULL) {
        free_ast(condition);
        return NULL;
    }

    add_child(node_if, body);
    return node_if;
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

ASTNode* parse_assignment() {
    if (check(TOK_IDENTIFIER) || check(TOK_MULTIPLICATION)) {
        Token* next = peek_token(1);
        if (is_compound_token(next->type)) {
            return parse_compound_operators();
        }
    }

    ASTNode* lhs = parse_expression();
    if (lhs == NULL) {
        printf("Parse error: expected lvalue\n");
        return NULL;
    }

    if (lhs->type != AST_IDENTIFIER && lhs->type != AST_DEREFERENCE && lhs->type != AST_MEMBER_ACCESS) {
        printf("Parse error: invalid lvalue\n");
        free_ast(lhs);
        return NULL;
    }

    if (!match(TOK_ASSIGNMENT)) {
        printf("Parse error: expected '='\n");
        return NULL;
    }

    ASTNode* rhs = parse_expression();
    if (rhs == NULL) {
        printf("Parse error: expected expression on RHS\n");
        return NULL;
    }

    if (!match(TOK_SEMICOLON)) {
        printf("Parse error: expected ';'\n");
        free_ast(rhs);
        return NULL;
    }

    ASTNode* assign_node = new_node(AST_ASSIGN);
    add_child(assign_node, lhs);
    add_child(assign_node, rhs);

    return assign_node;
}

ASTNode* parse_variable_declaration() {
    TypeInfo type = parse_type();

    if (current_token()->type != TOK_IDENTIFIER) {
        printf("Parse error: expected variable name\n");
        return NULL;
    }

    ASTNode* node = new_node(AST_VAR_DECL);
    if(node == NULL)
        return NULL;

    node->name = strdup(current_token()->lexme);
    node->type_info = type;
    advance();

    if (match(TOK_ASSIGNMENT)) {
        ASTNode* expr = parse_expression();
        if(expr == NULL) {
            free_ast(node);
            return NULL;
        }
        add_child(node, expr);
    }
    
    if (!match(TOK_SEMICOLON)) {
        printf("Parse error: expected ';'\n");
        return NULL;
    }

    return node;
}

ASTNode* parse_struct_member() {
    TypeInfo type = parse_type();

    if (current_token()->type != TOK_IDENTIFIER) {
        return NULL;
    }

    ASTNode* member = new_node(AST_VAR_DECL);
    if(member == NULL)
        return NULL;

    member->name = strdup(current_token()->lexme);
    member->type_info = type;
    advance();

    if (!match(TOK_SEMICOLON)) {
        free_ast(member);
        return NULL;
    }

    return member;
}

ASTNode* parse_struct_declaration() {
    if (!match(TOK_STRUCT))
        return NULL;

    if (!check(TOK_IDENTIFIER))
        return NULL;

    ASTNode* struct_node = new_node(AST_STRUCT);
    if(struct_node == NULL)
        return NULL;
        
    struct_node->name = strdup(current_token()->lexme);
    advance();
    
    if (!match(TOK_LBRACE)) {
        free_ast(struct_node);
        return NULL;
    }

    while (!check(TOK_RBRACE) && !check(TOK_EOF)) {
        if (is_type(current_token()->type))
        {
            ASTNode* member = parse_struct_member();
            if(member != NULL)
                add_child(struct_node, member);
        }
        else
            advance();
    }

    if (!match(TOK_RBRACE))
        return NULL;

    if (!match(TOK_SEMICOLON))
        return NULL;

    return struct_node;
}

ASTNode* parse_return() {
    if (!match(TOK_RETURN)) {
        printf("Parse error: expected 'return'\n");
        return NULL;
    }

    ASTNode* ret_node = new_node(AST_RETURN);
    if(ret_node == NULL)
        return ret_node;

    if (!check(TOK_SEMICOLON)) {
        ASTNode* expr = parse_expression();
        if(expr == NULL) {
            free_ast(ret_node);
            return NULL;
        }
        add_child(ret_node, expr);
    }

    if (!match(TOK_SEMICOLON)) {
        printf("Parse error: expected ';' after return\n");
        return NULL;
    }

    return ret_node;
}

ASTNode* parse_statement() {
    if (check(TOK_RETURN))
        return parse_return();

    if (check(TOK_IF))
        return parse_condition();

    if (check(TOK_FOR))
        return parse_for_loop();

    if (check(TOK_WHILE))
        return parse_while_loop();

    if (check(TOK_LBRACE))
        return parse_block();

    if (check(TOK_IDENTIFIER) || check(TOK_MULTIPLICATION))
        return parse_assignment();

    if (is_type(current_token()->type))
        return parse_variable_declaration();

    if (check(TOK_STRUCT))
        return parse_struct_declaration();

    printf("Parse error: unknown statement\n");
    advance();
    return NULL; 
}

ASTNode* parse_block()
{
    if (!match(TOK_LBRACE)) {
        printf("Parse error: expected '{'\n");
        return NULL;
    }

    ASTNode* block = new_node(AST_BLOCK);
    if(block == NULL)
        return NULL;

    while (!check(TOK_RBRACE) && !check(TOK_EOF)) {
        ASTNode* stmt = parse_statement();
        if(stmt == NULL)
            continue;
        add_child(block, stmt);
    }

    if (!match(TOK_RBRACE)) {
        printf("Parse error: expected '}'\n");
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

ASTNode* parse_parameters() {
    if (!match(TOK_LPAREN)) {
        printf("Parse error: expected '('\n");
        return NULL;
    }

    ASTNode* param_list = new_node(AST_PARAM_LIST);
    if(param_list == NULL)
        return NULL;

    while (!check(TOK_RPAREN) && !check(TOK_EOF)) 
    {
        if (!is_type(current_token()->type)) {
            printf("Parse error: expected type in parameter list\n");
            free_ast(param_list);
            return NULL;
        }

        TypeInfo type = parse_type();

        if (!check(TOK_IDENTIFIER)) {
            printf("Parse error: expected parameter name\n");
            free_ast(param_list);
            return NULL;
        }
        char* param_name = strdup(current_token()->lexme);
        advance();

        ASTNode* param_node = new_node(AST_VAR_DECL);
        if(param_node == NULL)
        {
            free_ast(param_list);
        }

        param_node->name = param_name;
        param_node->type_info = type;

        add_child(param_list, param_node);

        if (!match(TOK_COMMA)) {
            break;
        }
    }

    if (!match(TOK_RPAREN)) {
        printf("Parse error: expected ')'\n");
        free_ast(param_list);
        return NULL;
    }

    return param_list;
}

ASTNode* parse_function()
{
    TypeInfo return_type = parse_type();

    if (!check(TOK_IDENTIFIER)) {
        printf("Parse error: expected function name\n");
        return NULL;
    }

    ASTNode* func = new_node(AST_FUNCTION);
    if(func == NULL)
        return NULL;

    func->type_info = return_type;
    func->name = strdup(current_token()->lexme);
    advance();

    ASTNode* params = parse_parameters();
    if (params == NULL) {
        free_ast(func);
        return NULL;
    }

    add_child(func, params);

    ASTNode* body = parse_block();
    if(body == NULL) {
        free_ast(func);
        return NULL;
    }

    add_child(func, body);
    return func;
}

ASTNode* parse_namespace(ASTNodeType type)
{
    if(!match(TOK_NAMESPACE))
    {
        printf("Parse error: expected 'namespace'\n");
        return NULL;
    }

    char* namespace_name = strdup(current_token()->lexme);
    if (!match(TOK_IDENTIFIER)) {
        free(namespace_name);
        printf("Parse error: expected namespace name\n");
        return NULL;
    }

    ASTNode* namespace_node = new_node(type);
    if (namespace_node != NULL)
        namespace_node->name = namespace_name;

    return namespace_node;
}

ASTNode* parse_program() {
    ASTNode* program = parse_namespace(AST_PROGRAM);
    if(program == NULL)
        return NULL;

    if (!match(TOK_LBRACE)) {
        printf("Parse error: expected '{'\n");
        return NULL;
    }

    while (!check(TOK_RBRACE) && !check(TOK_EOF)) {

        if (is_func_declaration())
        {
            ASTNode* func = parse_function();
            if(func != NULL)
                add_child(program, func);
        }
        else if (check(TOK_STRUCT)) {
            ASTNode* struct_node = parse_struct_declaration();
            if(struct_node != NULL)
                add_child(program, struct_node);
        }
        else if (is_type(current_token()->type))
        {
            ASTNode* var_decl = parse_variable_declaration();
            if(var_decl != NULL)
                add_child(program, var_decl);
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
    for (int i = 0; i < level; i++) printf("  ");

    switch (node->type) {
        case AST_PROGRAM:       printf("Program (namespace %s)\n", node->name ? node->name : "(unnamed)"); break;
        case AST_FUNCTION:      printf("Function (return type %s, name %s)\n", token_type_name(node->type_info.base_type), node->name); break;
        case AST_PARAM_LIST:    printf("Parameter List\n"); break;
        case AST_BLOCK:         printf("Block\n"); break;
        case AST_RETURN:        printf("Return\n"); break;
        case AST_ASSIGN:        printf("Assign\n"); break;
        case AST_VAR_DECL:      printf("VariableDecl(type %s, name %s, level %d)\n", token_type_name(node->type_info.base_type), node->name, node->type_info.pointer_level); break;
        case AST_DEREFERENCE:   printf("Dereference\n"); break;
        case AST_ADDRESS_OF:    printf("AddressOf\n"); break;
        case AST_FUNC_CALL:     printf("Function call(name: %s)\n", node->name); break;
        case AST_IDENTIFIER:    printf("Identifier(%s)\n", node->name); break;
        case AST_UNARY_MINUS:   printf("Unary minus\n"); break;
        case AST_ADDITION:      printf("Addition\n"); break;
        case AST_SUBTRACTION:   printf("Subtraction\n"); break;
        case AST_MULTIPLICATION:printf("Multiplication\n"); break;
        case AST_DIVISION:      printf("Division\n"); break;
        case AST_MODULO:        printf("Modulo\n"); break;
        case AST_EQUAL:         printf("Equal\n"); break;
        case AST_NOT_EQUAL:     printf("NotEqual\n"); break;
        case AST_IF:            printf("If\n"); break;
        case AST_FOR:           printf("For\n"); break;
        case AST_WHILE:         printf("While\n"); break;
        case AST_LESS:          printf("Less\n"); break;
        case AST_GREATER:       printf("Greater\n"); break;
        case AST_LESS_EQUAL:    printf("Less Equal\n"); break;
        case AST_GREATER_EQUAL: printf("Greater Equal\n"); break;
        case AST_STRUCT:        printf("Struct %s\n", node->name); break;
        case AST_MEMBER_ACCESS: printf("MemberAccess(.%s)\n", node->name); break;
        case AST_CAST:
            printf("Cast(to %s", token_type_name(node->type_info.base_type));
            if (node->type_info.pointer_level > 0) {
                for (int i = 0; i < node->type_info.pointer_level; i++)
                    printf("*");
            }
            printf(")\n");
            break;
        case AST_STRING_LITERAL:printf("StringLiteral(\"%s\")\n", node->name ? node->name : ""); break;            
        case AST_CHAR_LITERAL:  printf("CharLiteral('%s')\n", node->name ? node->name : ""); break;
        case AST_EXPRESSION:
            if (node->name != NULL) {
                switch (node->type_info.base_type) {
                    case TOK_NUMBER_INT:    printf("Number(int: %s)\n", node->name); break;
                    case TOK_NUMBER_FLOAT:  printf("Number(float: %s)\n", node->name); break;
                    case TOK_NUMBER_DOUBLE: printf("Number(double: %s)\n", node->name); break;
                    default:                printf("Number(%s)\n", node->name); break;
                }
            }
            break;
        default:                printf("UnknownNodeType(%d)\n", node->type); break;
    }

    for (int i = 0; i < node->child_count; i++) {
        print_ast(node->children[i], level + 1);
    }
}