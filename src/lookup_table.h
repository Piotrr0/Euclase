#ifndef LOOKUP_TABLE_H
#define LOOKUP_TABLE_H

#include "lexer.h"
#include "parser.h"
#include <llvm-c/Types.h>

#define HASH_TABLE_SIZE 32
#define MAX_SCOPE_DEPTH 64

typedef struct SymbolData {
    char* name;
    int is_global;
    TypeInfo info;
    LLVMValueRef alloc;
} SymbolData;

typedef struct SymbolEntry {
    SymbolData symbol_data;
    struct SymbolEntry* next;
} SymbolEntry;

typedef struct HashTable {
    SymbolEntry* buckets[HASH_TABLE_SIZE];
} HashTable;

typedef struct Scope {
    HashTable* table;
    struct Scope* parent;
    int depth;
} Scope;

typedef struct SymbolTable {
    Scope* scopes[MAX_SCOPE_DEPTH];
    Scope* current_scope;
    int scope_count;
} SymbolTable;

size_t hash_string(const char* str);

SymbolTable* init_symbol_table();
HashTable* create_hash_table();
Scope* create_scope(Scope* parent, int depth);
SymbolEntry* create_symbol_entry(SymbolData data);

void free_scope(Scope* scope);
void free_symbol_entry(SymbolEntry* entry);
void push_scope(SymbolTable* st);
void pop_scope(SymbolTable* st);

int add_symbol(SymbolTable* st, SymbolData symbol_data);
SymbolEntry* lookup_symbol_current_scope(SymbolTable* st, const char* name);
SymbolEntry* lookup_symbol(SymbolTable* st, const char* name);

size_t hash_string(const char* str);

#endif