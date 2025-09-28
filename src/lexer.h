#ifndef LEXER_H
#define LEXER_H

typedef enum {
    TOK_LBRACE, 
    TOK_RBRACE, 
    TOK_LPAREN, 
    TOK_RPAREN,
    TOK_SEMICOLON,
    TOK_ASSIGNMENT,
    TOK_ASTERISK,
    TOK_AMPERSAND,

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
    TOK_NUMBER_INT,
    TOK_NUMBER_FLOAT,
    TOK_NUMBER_DOUBLE,

    TOK_NAMESPACE,
    TOK_RETURN,

    TOK_EOF,
    TOK_ERROR

} TokenType;

typedef enum {
    VAL_INT,
    VAL_FLOAT,
    VAL_DOUBLE,
    VAL_NONE
} ValueType;

typedef struct Value
{
    ValueType type;
    union {
        int int_val;
        float float_val;
        double double_val;
    };

} Value;

typedef struct Token {
    TokenType type;
    char* text;
    Value value;
} Token;


typedef struct Lexer {
    const char* source;
    int position;
} Lexer;

typedef struct Keyword {
    const char* name;
    TokenType type;
} Keyword;

typedef struct Symbol {
    char ch;
    TokenType type;
} Symbol;


extern Lexer lexer;
extern Symbol symbols[];
extern Keyword keywords[];


Token make_token(TokenType type, const char* text, ValueType vtype);
void free_token(Token* token);

Token make_int_token(int value);
Token make_float_token(float value);
Token make_double_token(double value);
TokenType lookup_for_keyword(const char* str);
TokenType lookup_for_symbol(char c);

void init_lexer(Lexer* lexer, const char* source);

void skip_whitespaces_from_pos(int* pos);
void skip_whitespaces();

Token lex_symbol_form_pos(int* pos);
Token lex_symbol();

Token lex_number_form_pos(int* pos);
Token lex_number();

Token lex_keyword_from_pos(int* pos);
Token lex_keyword();

Token get_token_from_pos(int* pos);
Token get_token();

Token peek_token(int steps);

char peek(int* pos);
char get(int* pos);

const char* token_type_name(TokenType type);

#endif