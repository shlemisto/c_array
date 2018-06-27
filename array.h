#ifndef __ARRAY_H__
#define __ARRAY_H__

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

// get index from iterator
#define __aind(name) __i_ ## name

#define __array_push(name)	__arr_## name ##_push
#define __array_pop(name)	__arr_## name ##_pop
#define __array_at(name)	__arr_## name ##_at
#define __array_find(name)	__arr_## name ##_find
#define __array_new(name)	__arr_## name ##_new

#define array_new(name, destructor, comparator) __array_new(name)(destructor, comparator)
#define array_len(arr) arr->len
#define array_data(arr) arr->data
#define array_is_empty(arr) arr->len == 0
#define array_set_item_destructor(arr, d) arr->item_destructor = d
#define array_set_comparator(arr, c) arr->comparator = c
#define array_at(arr, i) arr->at(arr, i)
#define array_at_val(arr, i) *arr->at(arr, i) // !!! be careful
#define array_pop(arr, pos) arr->pop(arr, pos)
#define array_push(arr, item) arr->push(arr, item)
#define array_push_val(arr, val) ({ \
		__typeof__(val) __item = val; \
		arr->push(arr, &__item); \
	})
#define array_push_array(arr, from, len) \
	int __aind(from) = 0; \
	for ( ; __aind(from) < (len); __aind(from)++) \
		array_push(arr, &from[__aind(from)]);
#define array_find_from(arr, what, pos) arr->find(arr, what, pos)
#define array_find_val_from(arr, val, pos) ({ \
		__typeof__(val) __item = (val); \
		arr->find(arr, &__item, pos); \
	})
#define array_find(arr, what) array_find_from(arr, what, 0)
#define array_find_val(arr, val) array_find_val_from(arr, val, 0)
#define array_purge(arr) \
	if (arr) { \
		if (arr->item_destructor) { \
			int i; \
			int len = array_len(arr); \
			for (i = 0 ; i < len; ++i) \
				arr->item_destructor(&arr->data[i]); \
		} \
		arr->len = 0; \
		arr->capacity = 10; \
	}
#define array_free(arr) \
	if (arr) { \
		if (arr->item_destructor) { \
			int i; \
			int len = array_len(arr); \
			for (i = 0 ; i < len; ++i) \
				arr->item_destructor(&arr->data[i]); \
		} \
		free(arr->data); \
		free(arr); \
	}

// note: pos >= 0
#define __array_for_each(arr, iter, pos) \
	__typeof(arr->data) iter = NULL; \
	int __aind(iter) = -ENOENT; \
	for (__aind(iter) = (pos); ((pos) >= 0) && (__aind(iter) < array_len(arr)) && (iter = array_at(arr, __aind(iter)), 1); ++__aind(iter))
#define array_for_each(arr, iter) \
	__array_for_each(arr, iter, 0)
#define array_for_each_val(arr, iter) \
	__typeof(array_at_val(arr, 0)) iter; \
	int __aind(iter) = 0; \
	for ( ; __aind(iter) < array_len(arr) && (iter = array_at_val(arr, __aind(iter)), 1); ++__aind(iter))

#define __array_new_gen(T, name) \
	typedef T (*name##_item_constructor_p)(); \
	typedef void (*name##_item_destructor)(T *item); \
	typedef void (*name##_item_destructor_p)(T item); \
	typedef int (*name##_comparator)(const void *v1, const void *v2); \
	\
	int __array_push(name)(struct name *arr, T *item) \
	{ \
		if (!item) \
			return EPERM; \
		if (arr->capacity == arr->len) { \
			arr->capacity *= 2; \
			arr->data = (T *) realloc(arr->data, arr->capacity * sizeof(T)); \
			if (!arr->data) \
				return ENOMEM; \
		} \
		\
		memcpy((void *) &arr->data[arr->len++], (void *) item, sizeof(T)); \
		\
		return 0; \
	} \
	\
	int __array_pop(name)(struct name *arr, int pos) \
	{ \
		T *tmp = array_at(arr, pos); \
		\
		if (array_is_empty(arr)) \
			return ENODATA; \
		if ((pos >= array_len(arr)) || (pos < -array_len(arr))) \
			return EPERM; \
		/* case for len == 1, or for the last elem */ \
		else if ((pos == array_len(arr) - 1) || (pos == -1)) \
			arr->len -= 1; \
		else if ((pos >= 0) && (pos < array_len(arr) - 1)) { \
			int i; \
			for (i = pos; i < array_len(arr) - 1; ++i) \
				memcpy((void *) array_at(arr, i), (void *) array_at(arr, i+1), sizeof(T)); \
			arr->len -= 1; \
		} else if ((pos < 0) && (pos >= -array_len(arr))) { \
			int i = array_len(arr) + pos; \
			for (; i < array_len(arr) - 1; ++i) \
				memcpy((void *) array_at(arr, i), (void *) array_at(arr, i+1), sizeof(T)); \
			arr->len -= 1; \
		} \
		\
		if (arr->item_destructor) \
			arr->item_destructor(tmp); \
		\
		return 0; \
	} \
	\
	T *__array_at(name)(struct name *arr, int pos) \
	{ \
		if (pos >= 0) \
			return pos < array_len(arr) ? &arr->data[pos] : NULL; \
		else \
			return pos >= -array_len(arr) ? &arr->data[array_len(arr)+pos] : NULL; \
	} \
	\
	T *__array_find(name)(struct name *arr, T *what, int pos) \
	{ \
		if (!arr->comparator || !what) \
			return NULL; \
		\
		__array_for_each(arr, iter, pos) { \
			if (0 == arr->comparator((const void *) iter, (const void *) what)) \
				return iter; \
		} \
		\
		return NULL; \
	} \
	\
	struct name *__array_new(name)(name##_item_destructor destructor, name##_comparator comparator) \
	{ \
		struct name *arr = (struct name *) calloc(1, sizeof(struct name)); \
		if (!arr) \
			return NULL; \
		arr->capacity = 10; \
		arr->data = (T *) calloc(arr->capacity, sizeof(T)); \
		if (!arr->data) { \
			free(arr); \
			return NULL; \
		} \
		arr->len = 0; \
		\
		arr->push = __array_push(name); \
		arr->pop  = __array_pop(name); \
		arr->at   = __array_at(name); \
		arr->find = __array_find(name); \
		arr->comparator = comparator; \
		arr->item_destructor = destructor; \
		\
		arr->find_p = NULL; \
		arr->item_destructor_p = NULL; \
		arr->item_constructor_p = NULL; \
		\
		return arr; \
	}

/* --> specialization for array of pointers */
#define __parray_find(name)	__parr_## name ##_find
#define __parray_new(name)	__parr_## name ##_new

#define parray_new(name, constructor, destructor, comparator) __parray_new(name)(constructor, destructor, comparator)
#define parray_len(arr) arr->len
#define parray_data(arr) arr->data
#define parray_is_empty(arr) (arr->len == 0)
#define parray_set_item_destructor(arr, d) arr->item_destructor_p = d
#define parray_set_comparator(arr, c) arr->comparator = c
#define parray_push(arr, item) arr->push(arr, &item)
#define parray_pop(arr, pos) ({ \
		void *addr = parray_at(arr, pos); \
		if (addr) { \
			array_pop(arr, pos); \
			if (arr->item_destructor_p) \
				arr->item_destructor_p(addr); \
		} \
	})
#define parray_at(arr, i) ({ \
		__typeof(arr->data) __item = array_at(arr, i); \
		__item ? *__item : NULL; \
	})
#define __parray_for_each(arr, iter, pos) \
	__typeof(arr->data[0]) iter = NULL; \
	int __aind(iter) = -ENOENT; \
	for (__aind(iter) = (pos); ((pos) >= 0) && (__aind(iter) < array_len(arr)) && (iter = parray_at(arr, __aind(iter)), 1); ++__aind(iter))
#define parray_for_each(arr, iter) \
	__parray_for_each(arr, iter, 0)
#define parray_find_from(arr, what, pos) arr->find_p(arr, what, pos)
#define parray_find(arr, what) parray_find_from(arr, what, 0)
#define parray_new_item(arr) arr->item_constructor_p ? arr->item_constructor_p() : NULL

#define parray_purge(arr) \
	if (arr) { \
		if (arr->item_destructor_p) { \
			int i; \
			int len = array_len(arr); \
			for (i = 0 ; i < len; ++i) \
				arr->item_destructor_p(arr->data[i]); \
		} \
		arr->len = 0; \
		arr->capacity = 10; \
	}
#define parray_free(arr) \
	if (arr) { \
		if (arr->item_destructor_p) { \
			int i; \
			int len = array_len(arr); \
			for (i = 0 ; i < len; ++i) \
				arr->item_destructor_p(arr->data[i]); \
		} \
		\
		free(arr->data); \
		free(arr); \
	}

#define parray_generator(T, name) \
	array_generator(T, name) \
	\
	T __parray_find(name)(struct name *arr, T what, int pos) \
	{ \
		if (!arr->comparator || !what) \
			return NULL; \
		\
		__parray_for_each(arr, iter, pos) { \
			if (0 == arr->comparator((const void *) &iter, (const void *) &what)) \
				return iter; \
		} \
		\
		return NULL; \
	} \
	\
	struct name *__parray_new(name)(name##_item_constructor_p constructor, name##_item_destructor_p destructor, name##_comparator comparator) \
	{ \
		struct name *arr = (struct name *) calloc(1, sizeof(struct name)); \
		if (!arr) \
			return NULL; \
		arr->capacity = 10; \
		arr->data = (T *) calloc(arr->capacity, sizeof(T)); \
		if (!arr->data) { \
			free(arr); \
			return NULL; \
		} \
		arr->len = 0; \
		\
		arr->push = __array_push(name); \
		arr->pop  = __array_pop(name); \
		arr->at   = __array_at(name); \
		arr->find = NULL; \
		arr->comparator = comparator; \
		arr->item_destructor = NULL; \
		\
		arr->find_p = __parray_find(name); \
		arr->item_destructor_p = destructor; \
		arr->item_constructor_p = constructor; \
		\
		return arr; \
	}
/* <-- specialization for array of pointers */

#define array_generator(T, name) \
	typedef struct name { \
		T *data; \
		int len; \
		int capacity; \
		\
		int (*push)(struct name *arr, T *item); \
		int (*pop)(struct name *arr, int pos); \
		T *(*at)(struct name *arr, int pos); \
		T *(*find)(struct name *arr, T *item, int pos); \
		int (*comparator)(const void *item1, const void *item2); \
		void (*item_destructor)(T *item); \
		\
		/* special functions for array of pointers */ \
		T (*item_constructor_p)(); \
		void (*item_destructor_p)(T item); \
		T (*find_p)(struct name *arr, T item, int pos); \
	} name##_t; \
	\
	__array_new_gen(T, name)

#endif // __ARRAY_H__
