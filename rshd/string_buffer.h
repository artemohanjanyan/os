#ifndef STRING_BUFFER_H
#define STRING_BUFFER_H

#include "stddef.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct string_buffer_impl;

struct string_buffer
{
	struct string_buffer_impl *impl;
};

int string_buffer_init(struct string_buffer *buffer);
void string_buffer_free(struct string_buffer *buffer);

void string_buffer_append(struct string_buffer *buffer, char const *str, size_t length);

char const *string_buffer_get_str(struct string_buffer *buffer);
size_t string_buffer_get_length(struct string_buffer *buffer);
void string_buffer_drop(struct string_buffer *buffer, size_t length);

int string_buffer_is_empty(struct string_buffer *buffer);

#ifdef __cplusplus
}
#endif

#endif
