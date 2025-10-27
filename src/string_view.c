#include "string_view.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

StringView sv_from_cstr(const char* str) {
    if (str == NULL) {
        return (StringView){NULL, 0};
    }
    return (StringView){str, strlen(str)};
}

StringView sv_from_parts(const char* data, size_t length)
{
    return (StringView){data, length};
}

char* sv_to_owned_cstr(StringView sv) {
    if (sv.data == NULL) {
        return NULL;
    }
    
    char* str = malloc(sv.length + 1);
    if (str == NULL) {
        return NULL;
    }
    
    memcpy(str, sv.data, sv.length);
    str[sv.length] = '\0';
    return str;
}

void sv_print(StringView sv) {
    if (sv.data == NULL)
        return;

    printf("%.*s", (int)sv.length, sv.data);
}