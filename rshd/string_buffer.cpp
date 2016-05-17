#include "string_buffer.h"

#include <queue>
#include <string>
#include <new>
#include <iostream>

struct string_buffer_impl
{
	size_t dropped = 0;
	std::queue<std::string> queue;
};

int string_buffer_init(struct string_buffer *buffer)
{
	buffer->impl = new(std::nothrow) string_buffer_impl{};
	return buffer->impl == nullptr;
}

void string_buffer_free(struct string_buffer *buffer)
{
	delete buffer->impl;
}

void string_buffer_append(struct string_buffer *buffer, char const *str, size_t length)
{
	buffer->impl->queue.emplace(str, length);
}

char const *string_buffer_get_str(struct string_buffer *buffer)
{
	return buffer->impl->queue.front().c_str() + buffer->impl->dropped;
}

size_t string_buffer_get_length(struct string_buffer *buffer)
{
	return buffer->impl->queue.front().size() - buffer->impl->dropped;
}

void string_buffer_drop(struct string_buffer *buffer, size_t length)
{
	buffer->impl->dropped += length;
	if (buffer->impl->dropped == buffer->impl->queue.front().size())
	{
		buffer->impl->queue.pop();
		buffer->impl->dropped = 0;
	}
}

int string_buffer_is_empty(struct string_buffer *buffer)
{
	return buffer->impl->queue.empty();
}
