#ifndef TOKEN_H
#define TOKEN_H

#include "string_view.h"

typedef enum {
    TOK_LBRACE, 
    TOK_RBRACE, 
    TOK_LPAREN, 
    TOK_RPAREN,
    TOK_COMMA,
    TOK_DOT,
    TOK_SEMICOLON,
    TOK_ASSIGNMENT,
    TOK_AMPERSAND,
    TOK_LESS,
    TOK_GREATER,
    TOK_LESS_EQUALS,
    TOK_GREATER_EQUALS,
    TOK_INCREMENT,
    TOK_DECREMENT,

    TOK_VOID,
    TOK_INT,
    TOK_UINT,
    TOK_FLOAT,
    TOK_UFLOAT,
    TOK_DOUBLE,
    TOK_UDOUBLE,
    TOK_CHAR,
    TOK_UCHAR,
    TOK_IF,
    TOK_ELSE,
    TOK_FOR,
    TOK_WHILE,
    TOK_STRUCT,

    TOK_STRING_LITERAL,
    TOK_CHAR_LITERAL,

    TOK_ADDITION,
    TOK_SUBTRACTION,
    TOK_MULTIPLICATION,
    TOK_DIVISION,
    TOK_MODULO,
    
    TOK_ASSIGNMENT_ADDITION,
    TOK_ASSIGNMENT_SUBTRACTION,
    TOK_ASSIGNMENT_MULTIPLICATION,
    TOK_ASSIGNMENT_DIVISION,
    TOK_ASSIGNMENT_MODULO,

    TOK_EQUAL,
    TOK_NOT_EQUAL,

    TOK_IDENTIFIER,
    TOK_NUMBER_INT,
    TOK_NUMBER_FLOAT,
    TOK_NUMBER_DOUBLE,

    TOK_NAMESPACE,
    TOK_RETURN,

    TOK_EOF,
    TOK_NONE,
    TOK_ERROR

} TokenType;

typedef struct Token {
    TokenType type;
    StringView lexeme;

    int line;
    int column;
} Token;

typedef struct Tokens {
    Token* tokens;
    int token_count;
    int capacity;
} Tokens;

#endif