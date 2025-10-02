#include "parser.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Token current_token;

ASTNode* new_node(ASTNodeType type) {
    ASTNode* node = malloc(sizeof(ASTNode));
    if (node == NULL)
            return NULL;

    node->type = type;
    node->name = NULL;
    node->value.type = VAL_NONE;
    node->value.int_val = 0;
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
    if (current_token.type != TOK_EOF)
        current_token = get_token();
}

int match(TokenType type) {
    if (current_token.type == type) {
        advance();
        return 1;
    }
    return 0;
}

int check(TokenType type) {
    return current_token.type == type;
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

ASTNode* parse_expression() 
{
    return parse_additive();
}

ASTNode* parse_additive()
{
    ASTNode* left = parse_multiplicative();
    if (left == NULL) 
        return NULL;
    
    while (check(TOK_ADDITION) || check(TOK_SUBTRACTION))
    {
        TokenType op = current_token.type;
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
        TokenType op = current_token.type;
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

ASTNode* parse_unary()
{
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
    if (is_func_call())
        return parse_function_call();
    
    return parse_primary();
}

ASTNode* parse_primary()
{
    if (check(TOK_IDENTIFIER))
        return parse_identifier_expression();
    
    return parse_primary_expression();
}

ASTNode* parse_primary_expression() {
    if (check(TOK_LPAREN)) 
    {
        advance();
        ASTNode* node = parse_additive();
        if (node == NULL)
            return NULL;

        if (!match(TOK_RPAREN)) {
            printf("Parse error: expected ')'\n");
            free_ast(node);
            return NULL;
        }
        return node;
    }
    
    if (check(TOK_NUMBER_INT) || check(TOK_NUMBER_FLOAT) || check(TOK_NUMBER_DOUBLE))
    {
        ASTNode* node = new_node(AST_EXPRESSION);
        node->value = current_token.value;
        advance();
        return node;
    }
    
    printf("Parse error: expected primary expression\n");
    return NULL;
}

ASTNode* parse_identifier_expression()
{
    ASTNode* node = new_node(AST_IDENTIFIER);
    if (!check(TOK_IDENTIFIER)) {
        return NULL;
    }

    node->name = strdup(current_token.text);
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

    char* name = strdup(current_token.text);
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
        ASTNode* arg = parse_additive();
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

    if(!is_type(current_token.type)) {
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
    if (!is_type(current_token.type)) {
        printf("Parse error: expected return type\n");
        exit(1);
    }

    TokenType ret_type = current_token.type;
    advance();

    int pointer_level = parse_pointer_level();

    TypeInfo info = { .base_type = ret_type, .pointer_level = pointer_level };
    return info;
}

int is_func_call()
{
    if(!check(TOK_IDENTIFIER))
        return 0;

    Token lparen_token = peek_token(1);
    if(lparen_token.type != TOK_LPAREN) {
        free_token(&lparen_token);
        return 0;
    }

    free_token(&lparen_token);
    return 1;
}

int is_casting()
{
    if(!check(TOK_LPAREN))
        return 0;

    Token type_token = peek_token(1);
    if(!is_type(type_token.type)) {
        free_token(&type_token);
        return 0;
    }
    free_token(&type_token);

    int pointer_level = check_pointer_level(2);
    Token rparen_token = peek_token(2 + pointer_level);

    int is_cast = (rparen_token.type == TOK_RPAREN);
    free_token(&rparen_token);

    return is_cast;
}

int check_pointer_level(int offset)
{
    int starting_offset = offset; 
    Token next_token = peek_token(offset);

    while(next_token.type == TOK_MULTIPLICATION) {
        free_token(&next_token);
        next_token = peek_token(++offset);
    }
    
    free_token(&next_token);
    return offset - starting_offset;
}

ASTNode* parse_assignment() {
    ASTNode* lhs = parse_expression();
    if (lhs == NULL) {
        printf("Parse error: expected lvalue\n");
        return NULL;
    }

    if (lhs->type != AST_IDENTIFIER && lhs->type != AST_DEREFERENCE) {
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

    if (current_token.type != TOK_IDENTIFIER) {
        printf("Parse error: expected variable name\n");
        return NULL;
    }

    ASTNode* node = new_node(AST_VAR_DECL);
    if(node == NULL)
        return NULL;

    node->name = strdup(current_token.text);
    node->type_info = type;
    advance();

    if (!match(TOK_ASSIGNMENT)) {
        printf("Parse error: expected initialization");
        return NULL;        
    }

    ASTNode* expr = parse_expression();
    if(expr == NULL) {
        free_ast(node);
        return NULL;
    }
    
    add_child(node, expr);
    node->value.type = expr->value.type;

    if (!match(TOK_SEMICOLON)) {
        printf("Parse error: expected ';'\n");
        return NULL;
    }

    return node;
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

    if (check(TOK_IDENTIFIER) || check(TOK_MULTIPLICATION))
        return parse_assignment();

    if (is_type(current_token.type))
        return parse_variable_declaration();

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

int is_func_declaration()
{
    if(!is_type(current_token.type))
        return 0;

    Token next_token = peek_token(1);
    if (next_token.type != TOK_IDENTIFIER) {
        free_token(&next_token);
        return 0;
    }

    Token third_token = peek_token(2);
    int is_func = (third_token.type == TOK_LPAREN);
    
    free_token(&next_token);
    free_token(&third_token);
    
    return is_func;
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
        if (!is_type(current_token.type)) {
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
        char* param_name = strdup(current_token.text);
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
    func->name = strdup(current_token.text);
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

    char* namespace_name = strdup(current_token.text);
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

ASTNode* parse_program()
{
    current_token = get_token();
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

        else if (is_type(current_token.type))
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
        case AST_ADDITION:      printf("Addition\n"); break;
        case AST_SUBTRACTION:   printf("Subtraction\n"); break;
        case AST_MULTIPLICATION: printf("Multiplication\n"); break;
        case AST_DIVISION:      printf("Division\n"); break;
        case AST_MODULO:        printf("Modulo\n"); break;
        case AST_CAST:
            printf("Cast(to %s", token_type_name(node->type_info.base_type));
            if (node->type_info.pointer_level > 0) {
                for (int i = 0; i < node->type_info.pointer_level; i++)
                    printf("*");
            }
            printf(")\n");
            break;
        case AST_EXPRESSION:
            switch (node->value.type) {
                case VAL_INT:       printf("Number(int: %d)\n", node->value.int_val); break;
                case VAL_FLOAT:     printf("Number(float: %f)\n", node->value.float_val); break;
                case VAL_DOUBLE:    printf("Number(double: %lf)\n", node->value.double_val); break;
                case VAL_NONE:      printf("Expression()\n");   break;
            }
            break;
        default:                printf("UnknownNodeType(%d)\n", node->type); break;
    }

    for (int i = 0; i < node->child_count; i++) {
        print_ast(node->children[i], level + 1);
    }
}