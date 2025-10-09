#include "lookup_table.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

SymbolTable* init_symbol_table() {
    SymbolTable* st = (SymbolTable*)malloc(sizeof(SymbolTable));
    st->scope_count = 0;

    Scope* global_scope = create_scope(NULL, 0);
    st->scopes[st->scope_count++] = global_scope;
    st->current_scope = global_scope;

    return st;
}

HashTable* create_hash_table() {
    HashTable* table = (HashTable*)malloc(sizeof(HashTable));
    return table;
}

Scope* create_scope(Scope* parent, int depth)
{
    if (parent == NULL)
        return NULL;

    Scope* scope = (Scope*)malloc(sizeof(Scope));
    scope->table = create_hash_table();
    scope->parent = parent;
    scope->depth = depth;
    return scope;
}

SymbolEntry* create_symbol_entry(SymbolData data) {
    SymbolEntry* new_entry = (SymbolEntry*)malloc(sizeof(SymbolEntry));
    new_entry->symbol_data = data;

    return new_entry;
}

void free_symbol_entry(SymbolEntry* entry) {
    if (entry == NULL)
        return;

    free(entry->symbol_data.name);
    free(entry);
}

void free_scope(Scope* scope) {
    if (scope == NULL)
        return;

    free(scope->table);
    free(scope);
}

size_t hash_string(const char* str) {
    size_t hash = 2166136261u;
    while (*str) {
        hash ^= (size_t)(*str++);
        hash *= 16777619u;
    }
    return hash % HASH_TABLE_SIZE;
}

void push_scope(SymbolTable* st)
{
    if(st == NULL)
        return;

    if (st->scope_count >= MAX_SCOPE_DEPTH)
        return;

    Scope* new_scope = create_scope(st->current_scope, st->scope_count);
    if(new_scope == NULL)
        return;

    st->scopes[st->scope_count++] = new_scope;
    st->current_scope = new_scope;
}

void pop_scope(SymbolTable* st)
{
    if(st == NULL)
        return;

    if (st->scope_count < 2) {
        return;
    }
    
    Scope* current = st->current_scope;
    for (int i = 0; i < HASH_TABLE_SIZE; i++)
    {
        SymbolEntry* entry = current->table->buckets[i];
        while (entry != NULL)
        {
            SymbolEntry* next = entry->next;
            free_symbol_entry(entry);
            entry = next;
        }
    }

    free_scope(current);
    st->current_scope = st->scopes[st->scope_count--];
}

int add_symbol(SymbolTable* st, SymbolData symbol_data) 
{
    if(st == NULL)
        return 0;

    size_t index = hash_string(symbol_data.name);
    HashTable* table = st->current_scope->table;
    
    SymbolEntry* entry = table->buckets[index];
    while (entry != NULL) {
        if (strcmp(entry->symbol_data.name, symbol_data.name) == 0) {
            return 0;
        }
        entry = entry->next;
    }
    
    SymbolEntry* new_entry = create_symbol_entry(symbol_data);
    table->buckets[index] = new_entry;
    return 1;
}