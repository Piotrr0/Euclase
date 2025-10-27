#ifndef STRING_VIEW_H
#define STRING_VIEW_H

#include <stdio.h>

typedef struct {
    const char* data;
    size_t length;
} StringView;

StringView sv_from_parts(const char* data, size_t len);
StringView sv_from_cstr(const char* cstr);
char* sv_to_owned_cstr(StringView sv);
void sv_print(StringView sv);

#endif