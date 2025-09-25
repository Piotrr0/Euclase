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
    node->value = 0;
    node->children = NULL;
    node->child_count = 0;
    return node;
}

void add_child(ASTNode* parent, ASTNode* child) {
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

ASTNode* parse_expression() {
    ASTNode* node = new_node(AST_EXPRESSION);
    if (current_token.type == TOK_NUMBER) {
        node->value = current_token.value;
        advance();
    } else if (current_token.type == TOK_IDENTIFIER) {
        node->name = strdup(current_token.text);
        advance();
    } else {
        printf("Parse error: expected expression\n");
        exit(1);
    }
    return node;
}

ASTNode* parse_statement()
{
    if (current_token.type != TOK_RETURN) {
        printf("Parse error: unknown statement\n");
        exit(1);
    }

    advance();
    ASTNode* ret = new_node(AST_RETURN);
    ASTNode* expr = parse_expression();
    add_child(ret, expr);

    if (!match(TOK_SEMICOLON)) {
        printf("Parse error: expected ';'\n");
        exit(1);
    }
    return ret;
}

ASTNode* parse_block()
{
    if (!match(TOK_LBRACE)) {
        printf("Parse error: expected '{'\n");
        exit(1);
    }

    ASTNode* block = new_node(AST_BLOCK);
    while (current_token.type != TOK_RBRACE && current_token.type != TOK_EOF) {
        ASTNode* stmt = parse_statement();
        add_child(block, stmt);
    }

    if (!match(TOK_RBRACE)) {
        printf("Parse error: expected '}'\n");
        exit(1);
    }
    return block;
}

ASTNode* parse_function()
{
    if (current_token.type != TOK_INT && current_token.type != TOK_FLOAT &&
        current_token.type != TOK_DOUBLE && current_token.type != TOK_CHAR &&
        current_token.type != TOK_UINT && current_token.type != TOK_UFLOAT &&
        current_token.type != TOK_UDOUBLE && current_token.type != TOK_UCHAR &&
        current_token.type != TOK_VOID) {
        printf("Parse error: expected type\n");
        exit(1);
    }

    ASTNode* func = new_node(AST_FUNCTION);
    func->name = strdup(token_type_name(current_token.type));
    advance();

    if (current_token.type != TOK_IDENTIFIER) {
        printf("Parse error: expected function name\n");
        exit(1);
    }

    ASTNode* name_node = new_node(AST_EXPRESSION);
    if (current_token.text)
        name_node->name = strdup(current_token.text);
    add_child(func, name_node);
    advance();

    if (!match(TOK_LPAREN) || !match(TOK_RPAREN)) {
        printf("Parse error: expected '()'\n");
        exit(1);
    }

    ASTNode* body = parse_block();
    add_child(func, body);

    return func;
}

ASTNode* parse_program()
{
    if(!match(TOK_NAMESPACE))
    {
        printf("Parse error: expected 'namespace'\n");
        exit(1);
    }

    if (current_token.type != TOK_IDENTIFIER) {
        printf("Parse error: expected namespace name\n");
        exit(1);
    }

    ASTNode* program = new_node(AST_PROGRAM);
    program->name = strdup(current_token.text);
    advance();

    if (!match(TOK_LBRACE)) {
        printf("Parse error: expected '{'\n");
        exit(1);
    }

    while (current_token.type != TOK_RBRACE && current_token.type != TOK_EOF) {
        ASTNode* func = parse_function();
        add_child(program, func);
    }

    if (!match(TOK_RBRACE)) {
        printf("Parse error: expected '}'\n");
        exit(1);
    }

    return program;
}

void print_ast(ASTNode* node, int level) {
    for (int i = 0; i < level; i++) printf("  ");
    switch (node->type) {
        case AST_PROGRAM:   printf("Program (namespace %s)\n", node->name); break;
        case AST_FUNCTION:  printf("Function (return type %s)\n", node->name); break;
        case AST_BLOCK:     printf("Block\n"); break;
        case AST_RETURN:    printf("Return\n"); break;
        case AST_EXPRESSION:
            if (node->name) 
                printf("Identifier(%s)\n", node->name);
            else 
                printf("Number(%d)\n", node->value);
            break;
    }
    
    for (int i = 0; i < node->child_count; i++) {
        print_ast(node->children[i], level + 1);
    }
}