#include "ptr_set.h"

#include <set>
#include <new>

struct ptr_set_impl
{
	std::set<void*> set;
};

int ptr_set_init(struct ptr_set *set)
{
	set->impl = new(std::nothrow) ptr_set_impl;
	return set->impl != nullptr;
}

void ptr_set_free(struct ptr_set *set)
{
	delete set->impl;
}

void ptr_set_insert(struct ptr_set *set, void *ptr)
{
	set->impl->set.insert(ptr);
}

void ptr_set_erase(struct ptr_set *set, void *ptr)
{
	set->impl->set.erase(ptr);
}

int ptr_set_is_empty(struct ptr_set *set)
{
	return set->impl->set.empty();
}

int ptr_set_contains(struct ptr_set *set, void *ptr)
{
	return set->impl->set.find(ptr) != set->impl->set.end();
}

void *ptr_set_first(struct ptr_set *set)
{
	return *set->impl->set.begin();
}

struct ptr_set_iterator_impl
{
	std::set<void*>::iterator it;
};

ptr_set_iterator *ptr_set_get_iterator(struct ptr_set *set)
{
	auto impl = new(std::nothrow) ptr_set_iterator_impl{set->impl->set.begin()};
	if (impl == nullptr)
		return nullptr;
	return new(std::nothrow) ptr_set_iterator{impl};
}

int ptr_set_iterator_not_end(struct ptr_set *set, struct ptr_set_iterator *it)
{
	return it->impl->it != set->impl->set.end();
}

void ptr_set_iterator_next(struct ptr_set_iterator *it)
{
	++it->impl->it;
}

void *ptr_set_iterator_get(struct ptr_set_iterator *it)
{
	return *it->impl->it;
}

void ptr_set_iterator_free(struct ptr_set_iterator *it)
{
	delete it->impl;
	delete it;
}
