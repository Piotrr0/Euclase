#ifndef LEXER_H
#define LEXER_H

typedef enum {
    TOK_LBRACE, 
    TOK_RBRACE, 
    TOK_LPAREN, 
    TOK_RPAREN,
    TOK_SEMICOLON,

    TOK_VOID,
    TOK_INT,
    TOK_UINT,
    TOK_FLOAT,
    TOK_UFLOAT,
    TOK_DOUBLE,
    TOK_UDOUBLE,
    TOK_CHAR,
    TOK_UCHAR,

    TOK_IDENTIFIER,
    TOK_NUMBER,

    TOK_NAMESPACE,
    TOK_RETURN,

    TOK_EOF,
    TOK_ERROR

} TokenType;

typedef struct Token {
    TokenType type;
    char* text;
    int value;
} Token;


typedef struct Lexer {
    const char* source;
    int position;
} Lexer;

extern Lexer lexer;

void init_lexer(const char* source);
char peek();
char get();
void skip_whitespaces();
Token get_token();
const char* token_type_name(TokenType type);

#endif