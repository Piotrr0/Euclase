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
    node->value_type = VAL_INT;
    node->value.int_val = 0;
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

ASTNode* parse_expression() {
    ASTNode* node = new_node(AST_EXPRESSION);
    
    if (check(TOK_NUMBER_INT) || check(TOK_NUMBER_FLOAT) || check(TOK_NUMBER_DOUBLE)){
        node->value_type = current_token.value_type;
        switch (current_token.value_type) {
            case VAL_INT:
                node->value.int_val = current_token.value.int_val;
                break;
            case VAL_FLOAT:
                node->value.float_val = current_token.value.float_val;
                break;
            case VAL_DOUBLE:
                node->value.double_val = current_token.value.double_val;
                break;
            case VAL_NONE:
              break;
            }
        advance();        
    } else if (current_token.type == TOK_IDENTIFIER) {
        node->name = strdup(current_token.text);
        advance();
    } else {
        printf("Parse error: expected expression\n");
        return NULL;
    }
    return node;
}

ASTNode* parse_assignment() {
    if (!check(TOK_IDENTIFIER)) {
        printf("Parse error: expected variable name\n");
        return NULL;
    }

    ASTNode* node = new_node(AST_ASSIGN);
    node->name = strdup(current_token.text);
    advance();

    if (!match(TOK_ASSIGNMENT)) {
        printf("Parse error: expected '='\n");
        return NULL;
    }

    ASTNode* expr = parse_expression();
    if(expr)
        add_child(node, expr);

    if (!match(TOK_SEMICOLON)) {
        printf("Parse error: expected ';'\n");
        return NULL;
    }

    return node;
}

ASTNode* parse_variable_declaration() {
    if (!is_type(current_token.type)) {
        printf("Parse error: expected type\n");
        return NULL;
    }

    TokenType var_type = current_token.type;
    advance();

    if (current_token.type != TOK_IDENTIFIER) {
        printf("Parse error: expected variable name\n");
        return NULL;
    }

    ASTNode* node = new_node(AST_VAR_DECL);
    node->name = strdup(current_token.text);
    node->decl_type = var_type;
    node->value_type = VAL_NONE;
    advance();

    if (match(TOK_ASSIGNMENT)) {
        ASTNode* expr = parse_expression();
        if(expr != NULL)
        {
            add_child(node, expr);
            node->value_type = expr->value_type;
        }
    }

    if (!match(TOK_SEMICOLON)) {
        printf("Parse error: expected ';'\n");
        return NULL;
    }

    return node;
}

ASTNode* parse_return() {
    if (!check(TOK_RETURN)) {
        printf("Parse error: expected 'return'\n");
        return NULL;
    }

    advance();

    ASTNode* ret_node = new_node(AST_RETURN);

    if (current_token.type != TOK_SEMICOLON) {
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

    if (is_type(current_token.type))
        return parse_variable_declaration();

    if (check(TOK_IDENTIFIER))
        return parse_assignment();

    printf("Parse error: unknown statement\n");
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

ASTNode* parse_function()
{
    if (!is_type(current_token.type)) {
        printf("Parse error: expected return type\n");
        return NULL;
    }

    TokenType ret_type = current_token.type;
    advance();

    if (!check(TOK_IDENTIFIER)) {
        printf("Parse error: expected function name\n");
        return NULL;
    }

    ASTNode* func = new_node(AST_FUNCTION);
    func->decl_type = ret_type;
    func->name = strdup(current_token.text);
    advance();

    if (!match(TOK_LPAREN) || !match(TOK_RPAREN)) {
        printf("Parse error: expected '()'\n");
        return NULL;
    }

    ASTNode* body = parse_block();
    if(body != NULL)
        add_child(func, body);

    return func;
}

ASTNode* parse_program()
{
    current_token = get_token();
    if(!match(TOK_NAMESPACE))
    {
        printf("Parse error: expected 'namespace'\n");
        return NULL;
    }

    if (current_token.type != TOK_IDENTIFIER) {
        printf("Parse error: expected namespace name\n");
        return NULL;
    }

    ASTNode* program = new_node(AST_PROGRAM);
    program->name = strdup(current_token.text);
    advance();

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
            {
                add_child(program, var_decl);
            }
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

void print_ast(ASTNode* node, int level) {
    for (int i = 0; i < level; i++) printf("  ");
    switch (node->type) {
        case AST_PROGRAM:   printf("Program (namespace %s)\n", node->name); break;
        case AST_FUNCTION:  printf("Function (return type %s, name %s)\n", token_type_name(node->decl_type), node->name); break;
        case AST_BLOCK:     printf("Block\n"); break;
        case AST_RETURN:    printf("Return\n"); break;
        case AST_EXPRESSION:
            if (node->name) {
                printf("Identifier(%s)\n", node->name);
            } else {
                switch (node->value_type) {
                    case VAL_INT:
                        printf("Number(int: %d)\n", node->value.int_val);
                        break;
                    case VAL_FLOAT:
                        printf("Number(float: %f)\n", node->value.float_val);
                        break;
                    case VAL_DOUBLE:
                        printf("Number(double: %lf)\n", node->value.double_val);
                        break;
                    case VAL_NONE:
                      break;
                    }
            }
            break;
        case AST_VAR_DECL:
            printf("VarDecl(type %s, name %s)\n", token_type_name(node->decl_type), node->name);
            break;
        case AST_ASSIGN:
            printf("Assign(%s)\n", node->name);
            break;
        }

    for (int i = 0; i < node->child_count; i++) {
        print_ast(node->children[i], level + 1);
    }
}