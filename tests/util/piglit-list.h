/*
 * Copyright © 2014 Intel Corporation
 * Copyright © 2008 Kristian Høgsberg
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


/**
 * \file
 * \brief pgl_list - a doubly-linked list
 *
 * The list must be initialized by pgl_list_init().  All entries in the list
 * must have the same type.  The item type must have contain
 * `struct pgl_list_node` as a member.
 *
 * Example:
 *
 * Consider a collection of timid squirrels, `s1`, `s2`, and `s3`.
 *
 * 	struct squirrel {
 * 		float timidity;
 * 		struct pgl_list_node link;
 * 		void *favorite_tree;
 * 	};
 *
 * 	struct squirrel s1, s2, s3;
 *
 * The code below will put the 3 timid squirrels into a list.
 *
 * 	struct pgl_list squirrel_list;
 *
 * 	pgl_list_init(&squirrel_list);
 * 	pgl_list_prepend(&squirrel_list, &s1.link);
 * 	pgl_list_prepend(&squirrel_list, &s2.link);
 * 	pgl_list_insert(&s2.link, &s3.link);  // Insert s3 after s2.
 *
 * The list order is now [s2, s3, s1]
 *
 * The code below traverses the list in ascending order:
 *
 * 	struct squirrel_list *s;
 * 	pgl_list_for_each(&squirrel_list, s, link) {
 * 		run_away_and_climb_tree(s);
 * 	}
 */

#pragma once

#ifndef __PIGLIT_LIST_H__
#define __PIGLIT_LIST_H__

#include <stdbool.h>
#include <stddef.h>

#ifdef  __cplusplus
extern "C" {
#endif

struct pgl_list_node {
	struct pgl_list_node *prev;
	struct pgl_list_node *next;
};

struct pgl_list {
	struct pgl_list_node root;
};

void pgl_list_init(struct pgl_list *list);
bool pgl_list_is_empty(const struct pgl_list *list);
size_t pgl_list_length(const struct pgl_list *list);
bool pgl_list_node_is_detached(const struct pgl_list_node *node);
void pgl_list_prepend(struct pgl_list *list, struct pgl_list_node *node);
void pgl_list_append(struct pgl_list *list, struct pgl_list_node *node);
void pgl_list_insert(struct pgl_list_node *resident_node,
		     struct pgl_list_node *new_node);
void pgl_list_remove(struct pgl_list_node *node);

#define pgl_container_of(member_ptr, container_type, member_name) \
	(container_type*)((void*)(member_ptr) - offsetof(container_type, member_name))

#define pgl_list_for_each(list_root, elem, link_name) \
	for (elem = pgl_container_of((list_root)->next, typeof(*elem), link_name); \
	     &elem->link_name != (list_root); \
	     elem = pgl_container_of(elem->link_name.next, typeof(*elem), link_name))

#ifdef  __cplusplus
}
#endif

#endif /*__PIGLIT_LIST_H__*/
