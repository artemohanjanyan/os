#ifndef PTR_SET_H
#define PTR_SET_H

#ifdef __cplusplus
extern "C"
{
#endif

struct ptr_set_impl;

struct ptr_set
{
	struct ptr_set_impl *impl;
};

int ptr_set_init(struct ptr_set *set);
void ptr_set_free(struct ptr_set *set);

void ptr_set_insert(struct ptr_set *set, void *ptr);
void ptr_set_erase(struct ptr_set *set, void *ptr);

int ptr_set_is_empty(struct ptr_set *set);
int ptr_set_contains(struct ptr_set *set, void *ptr);

void *ptr_set_first(struct ptr_set *set);


struct ptr_set_iterator
{
	struct ptr_set_iterator_impl *impl;
};

struct ptr_set_iterator *ptr_set_get_iterator(struct ptr_set *set);

int ptr_set_iterator_not_end(struct ptr_set *set, struct ptr_set_iterator *it);
void ptr_set_iterator_next(struct ptr_set_iterator *it);
void *ptr_set_iterator_get(struct ptr_set_iterator *it);

void ptr_set_iterator_free(struct ptr_set_iterator *it);

#ifdef __cplusplus
}
#endif

#endif
