/*
 * Copyright 2014 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEM, IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <assert.h>

#include "piglit-list.h"

static void
check_link(const struct pgl_list_link *link)
{
	if (link) {
		bool prev_attached = link->prev != link;
		bool next_attached = link->next != link;
		assert(prev_attached == next_attached);
	}
}

static void
check_list(const struct pgl_list *list)
{
	bool head_empty = list->head.next == &list->tail;
	bool tail_empty = list->tail.prev == &list->head;

	assert(head_empty == tail_empty);
}

static struct pgl_list_link*
get_prev_unchecked(struct pgl_list_link *link)
{
	if (link->prev->prev == link->prev) {
		return NULL;
	} else {
		return link->prev;
	}
}

static struct pgl_list_link*
get_next_unchecked(struct pgl_list_link *link)
{
	if (link->next->next == link->next) {
		return NULL;
	} else {
		return link->next;
	}
}

static void
insert_before_unchecked(struct pgl_list_link *resident_link,
			struct pgl_list_link *new_link)
{
	new_link->prev = resident_link->prev;
	new_link->next = resident_link;

	new_link->prev->next = new_link;
	new_link->next->prev = new_link;
}

static void
insert_after_unchecked(struct pgl_list_link *resident_link,
		       struct pgl_list_link *new_link)
{
	new_link->prev = resident_link;
	new_link->next = resident_link->next;

	new_link->prev->next = new_link;
	new_link->next->prev = new_link;
}

void
pgl_list_init(struct pgl_list *list)
{
	list->head.prev = &list->head;
	list->tail.next = &list->tail;

	list->head.next = &list->tail;
	list->tail.prev = &list->head;
}

void
pgl_list_init_link(struct pgl_list_link *link)
{
	link->prev = link;
	link->next = link;
}

bool
pgl_list_is_attached(const struct pgl_list_link *link)
{
	bool prev_attached = link->prev != link;
	bool next_attached = link->next != link;

	assert(prev_attached == next_attached);
	return prev_attached;
}

bool
pgl_list_is_empty(const struct pgl_list *list)
{
	bool head_empty = list->head.next == &list->tail;
	bool tail_empty = list->tail.prev == &list->head;

	assert(head_empty == tail_empty);
	return head_empty;
}

size_t
pgl_list_length(const struct pgl_list *list)
{
	struct pgl_list_link *link;
	size_t n = 0;

	for (link = pgl_list_get_first((struct pgl_list*) list);
	     link != NULL;
	     link = pgl_list_get_next(link)) {
		++n;
	}

	return n;
}

struct pgl_list_link*
pgl_list_get_first(struct pgl_list *list)
{
	struct pgl_list_link *link;

	link = get_next_unchecked(&list->head);
	check_link(link);
	return link;
}

struct pgl_list_link*
pgl_list_get_last(struct pgl_list *list)
{
	struct pgl_list_link *link;

	check_list(list);
	link = get_prev_unchecked(&list->tail);
	check_link(link);
	return link;
}

struct pgl_list_link*
pgl_list_get_prev(struct pgl_list_link *link)
{
	check_link(link);
	return get_prev_unchecked(link);
}

struct pgl_list_link*
pgl_list_get_next(struct pgl_list_link *link)
{
	check_link(link);
	return get_next_unchecked(link);
}

void
pgl_list_prepend(struct pgl_list *list, struct pgl_list_link *link)
{
	assert(!pgl_list_is_attached(link));
	insert_before_unchecked(&list->head, link);
}

void
pgl_list_append(struct pgl_list *list, struct pgl_list_link *link)
{
	assert(!pgl_list_is_attached(link));
	insert_after_unchecked(list->tail.prev, link);
}

void
pgl_list_insert_before(struct pgl_list_link *resident_link,
		              struct pgl_list_link *new_link)
{
	assert(pgl_list_is_attached(resident_link));
	assert(!pgl_list_is_attached(new_link));
	insert_before_unchecked(resident_link, new_link);
}

void
pgl_list_insert_after(struct pgl_list_link *resident_link,
		              struct pgl_list_link *new_link)
{
	assert(pgl_list_is_attached(resident_link));
	assert(!pgl_list_is_attached(new_link));
	insert_after_unchecked(resident_link, new_link);
}

void
pgl_list_remove(struct pgl_list_link *link)
{
	if (!link) {
		/* Behave like free(), which also accepts NULL. */
		return;
	}

	check_link(link);

	link->next->prev = link->prev;
	link->prev->next = link->next;

	pgl_list_init_link(link);
}
