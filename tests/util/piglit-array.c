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

#include <stdlib.h>
#include <string.h>

#include "piglit-array.h"

void
pgl_array_init(struct pgl_array *array)
{
	memset(array, 0, sizeof(*array));
}

bool
pgl_array_resize(struct pgl_array *array, size_t size)
{
	static const size_t initial_capacity = 16;
	size_t capacity;
	void *data;

	if (size == 0) {
		free(array->data);
		pgl_array_init(array);
		return true;
	}

	if (size <= array->capacity) {
		array->size = size;
		return true;
	}

	capacity = array->capacity;

	if (capacity == 0) {
		capacity = initial_capacity;
	}

	while (capacity < size) {
		capacity *= 2;
	}

	data = realloc(array->data, capacity);
	if (!data) {
		/* From the realloc(3) manpage:
		 *   If  realloc() fails the original block is left
		 *   untouched; it is not freed or moved.
		 */
		return false;
	}

	array->size = size;
	array->capacity = capacity;
	array->data = data;

	return true;
}
