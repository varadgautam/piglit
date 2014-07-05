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

#pragma once

#ifndef __PIGLIT_LIST_H__
#define __PIGLIT_LIST_H__

#include <stdbool.h>
#include <stddef.h>

struct pgl_list_link {
	struct pgl_list_link *prev;
	struct pgl_list_link *next;
};

struct pgl_list {
	struct pgl_list_link tail;
	struct pgl_list_link head;
};

void
pgl_list_init(struct pgl_list *list);

void
pgl_list_init_link(struct pgl_list_link *link);

bool
pgl_list_is_attached(const struct pgl_list_link *link);

bool
pgl_list_is_empty(const struct pgl_list *list);

size_t
pgl_list_length(const struct pgl_list *list);

struct pgl_list_link*
pgl_list_get_first(struct pgl_list *list);

struct pgl_list_link*
pgl_list_get_last(struct pgl_list *list);

struct pgl_list_link*
pgl_list_get_prev(struct pgl_list_link *link);

struct pgl_list_link*
pgl_list_get_next(struct pgl_list_link *link);

void
pgl_list_prepend(struct pgl_list *list, struct pgl_list_link *link);

void
pgl_list_append(struct pgl_list *list, struct pgl_list_link *link);

void
pgl_list_insert_before(struct pgl_list_link *resident_link,
			      struct pgl_list_link *new_link);

void
pgl_list_insert_after(struct pgl_list_link *resident_link,
			     struct pgl_list_link *new_link);

void
pgl_list_remove(struct pgl_list_link *link);

#endif /*__PIGLIT_LIST_H__*/
