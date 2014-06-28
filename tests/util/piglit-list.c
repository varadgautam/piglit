/*
 * Copyright © 2014 Intel Corporation
 * Copyright © 2008-2011 Kristian Høgsberg
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include "piglit-list.h"

void
pgl_list_init(struct pgl_list *list)
{
	list->root.prev = &list->root;
	list->root.next = &list->root;
}

bool
pgl_list_is_empty(const struct pgl_list *list)
{
	return list->root.next == &list->root;
}

size_t
pgl_list_length(const struct pgl_list *list)
{
	struct pgl_list_node *node;
	size_t count = 0;

	for (node = list->root.next;
	     node != &list->root;
	     node = node->next) {
		++count;
	}

	return count;
}

void
pgl_list_remove(struct pgl_list_node *node)
{
	node->prev->next = node->next;
	node->next->prev = node->prev;
	node->next = NULL;
	node->prev = NULL;
}

/**
 * Insert @a new_node after @a resident_node.
 */
void
pgl_list_insert(struct pgl_list_node *resident_node,
		struct pgl_list_node *new_node)
{
	new_node->prev = resident_node;
	new_node->next = resident_node->next;
	new_node->prev->next = new_node;
	new_node->next->prev = new_node;
}

void
pgl_list_prepend(struct pgl_list *list,
		 struct pgl_list_node *node)
{
	pgl_list_insert(&list->root, node);
}

void
pgl_list_append(struct pgl_list *list,
		struct pgl_list_node *node)
{
	pgl_list_insert(list->root.prev, node);
}
