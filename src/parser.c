#include "parser.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Token current_token;

ASTNode* new_node(ASTNodeType type) {
    ASTNode* node = malloc(sizeof(ASTNode));
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
    if(child == NULL)
        return;

    parent->children = realloc(parent->children, sizeof(ASTNode*) * (parent->child_count + 1));
    parent->children[parent->child_count++] = child;
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
    while (match(TOK_ASTERISK)) {
        level++;
    }
    return level;
}


ASTNode* parse_dereference()
{
    if(!match(TOK_ASTERISK))
        return NULL;

    ASTNode* node = new_node(AST_DEREFERENCE);
    ASTNode* dereference_exp = parse_expression();

    if(dereference_exp == NULL)
        return NULL;

    add_child(node, dereference_exp);
    return node;
}

ASTNode* parse_address_of()
{
    if(!match(TOK_AMPERSAND))
        return NULL;

    ASTNode* node = new_node(AST_ADDRESS_OF);
    ASTNode* address_of_exp = parse_expression();

    if(address_of_exp == NULL)
        return NULL;

    add_child(node, address_of_exp);
    return node;
}

ASTNode* parse_primary_expression() {
    ASTNode* node = new_node(AST_EXPRESSION);

    if (check(TOK_NUMBER_INT) || check(TOK_NUMBER_FLOAT) || check(TOK_NUMBER_DOUBLE)) {
        node->value = current_token.value;
        advance();
    } else {
        printf("Parse error: expected primary expression\n");
        return NULL;
    }

    return node;
}

ASTNode* parse_identifier_expression()
{
    ASTNode* node = new_node(AST_EXPRESSION);
    if (check(TOK_IDENTIFIER)) {
        node->name = strdup(current_token.text);
        advance();
    } 
    return node;
}

ASTNode* parse_dereference_expression() {
    if (!match(TOK_ASTERISK))
        return NULL;

    ASTNode* node = new_node(AST_DEREFERENCE);
    ASTNode* expr = parse_expression();
    if (!expr) {
        free(node);
        return NULL;
    }

    add_child(node, expr);
    return node;
}

ASTNode* parse_address_of_expression() {
    if (!match(TOK_AMPERSAND))
        return NULL;

    ASTNode* node = new_node(AST_ADDRESS_OF);
    ASTNode* expr = parse_expression();
    if (!expr) {
        free(node);
        return NULL;
    }

    add_child(node, expr);
    return node;
}

ASTNode* parse_expression() {
    if (check(TOK_ASTERISK))
        return parse_dereference_expression();

    if (check(TOK_IDENTIFIER))
        return parse_identifier_expression();

    if (check(TOK_AMPERSAND))
        return parse_address_of_expression();

    return parse_primary_expression();
}

ASTNode* parse_assignment() {

    ASTNode* lhs = parse_expression();
    if (lhs == NULL) {
        printf("Parse error: expected lvalue\n");
        return NULL;
    }

    if (lhs->type != AST_EXPRESSION && lhs->type != AST_DEREFERENCE) {
        printf("Parse error: invalid lvalue\n");
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

    ASTNode* node;
    if (type.pointer_level > 0) 
        node = new_node(AST_POINTER_DECL);
    else
        node = new_node(AST_VAR_DECL);

    node->name = strdup(current_token.text);
    node->type_info = type;
    advance();

    if (match(TOK_ASSIGNMENT)) {
        ASTNode* expr = parse_expression();
        if(expr != NULL)
        {
            add_child(node, expr);
            node->value.type = expr->value.type;
        }
    }

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

    if (!check(TOK_SEMICOLON)) {
        ASTNode* expr = parse_expression();
        if(expr != NULL)
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

    if (check(TOK_IDENTIFIER) || check(TOK_ASTERISK))
        return parse_assignment();

    if (is_type(current_token.type))
        return parse_variable_declaration();

    printf("Parse error: unknown statement\n");
    advance();
    return new_node(AST_EXPRESSION);
}

ASTNode* parse_block()
{
    if (!match(TOK_LBRACE)) {
        printf("Parse error: expected '{'\n");
        return NULL;
    }

    ASTNode* block = new_node(AST_BLOCK);
    while (!check(TOK_RBRACE) && !check(TOK_EOF)) {
        ASTNode* stmt = parse_statement();
        if(stmt != NULL)
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

int parse_parameters()
{
    if (!match(TOK_LPAREN) || !match(TOK_RPAREN)) {
        printf("Parse error: expected '()'\n");
        return -1;
    }
    return 0;
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

    parse_parameters();

    ASTNode* body = parse_block();
    if(body != NULL)
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
    for (int i = 0; i < level; i++) printf("  ");

    switch (node->type) {
        case AST_PROGRAM:       printf("Program (namespace %s)\n", node->name ? node->name : "(unnamed)"); break;
        case AST_FUNCTION:      printf("Function (return type %s, name %s)\n", token_type_name(node->type_info.base_type), node->name); break;
        case AST_BLOCK:         printf("Block\n"); break;
        case AST_RETURN:        printf("Return\n"); break;
        case AST_ASSIGN:        printf("Assign\n"); break;
        case AST_VAR_DECL:      printf("VarDecl(type %s, name %s)\n", token_type_name(node->type_info.base_type), node->name); break;
        case AST_POINTER_DECL:  printf("PointerDecl(type %s, name %s, level %d)\n", token_type_name(node->type_info.base_type), node->name, node->type_info.pointer_level); break;
        case AST_DEREFERENCE:   printf("Dereference\n"); break;
        case AST_ADDRESS_OF:    printf("AddressOf\n"); break;
        case AST_EXPRESSION:
            if (node->name) {
                printf("Identifier(%s)\n", node->name);
            } 
            else {
                switch (node->value.type) {
                    case VAL_INT:       printf("Number(int: %d)\n", node->value.int_val); break;
                    case VAL_FLOAT:     printf("Number(float: %f)\n", node->value.float_val); break;
                    case VAL_DOUBLE:    printf("Number(double: %lf)\n", node->value.double_val); break;
                    case VAL_NONE:      printf("Expression()\n");   break;
                }
            }
            break;
        default:                printf("UnknownNodeType(%d)\n", node->type); break;
    }

    for (int i = 0; i < node->child_count; i++) {
        print_ast(node->children[i], level + 1);
    }
}