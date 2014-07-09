/*
 * Copyright © 2012 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#pragma once
#ifndef PIGLIT_GL_FRAMEWORK_H
#define PIGLIT_GL_FRAMEWORK_H

#include <stdbool.h>

#include "piglit-framework-gl.h"

/**
 * Abstract class. Use piglit_gl_framework_create to create a concrete
 * instance.
 */
struct piglit_gl_framework {
	const struct piglit_gl_test_config *test_config;

	/**
	 * Does not return.
	 */
	void
	(*run_test)(struct piglit_gl_framework *gl_fw,
		    int argc, char *argv[]);

	/**
	 * Analogous to glutSwapBuffers(). May be null.
	 */
	void
	(*swap_buffers)(struct piglit_gl_framework *gl_fw);

	/**
	 * Analogous to glutKeyboardFunc(). May be null.
	 */
	void
	(*set_keyboard_func)(struct piglit_gl_framework *gl_fw,
	                     void (*func)(unsigned char key, int x, int y));

	/**
	 * Analogous to glutReshapeFunc(). May be null.
	 */
	void
	(*set_reshape_func)(struct piglit_gl_framework *gl_fw,
	                    void (*func)(int w, int h));

	/**
	 * Analogous to glutPostRedisplay(). May be null.
	 */
	void
	(*post_redisplay)(struct piglit_gl_framework *gl_fw);

	void
	(*destroy)(struct piglit_gl_framework *gl_fw);

	enum piglit_result
	(*create_dma_buf)(unsigned w, unsigned h, unsigned cpp,
			  const void *src_data, unsigned src_stride,
			  struct piglit_dma_buf **buf, int *fd,
			  unsigned *stride, unsigned *offset);

	void
	(*destroy_dma_buf)(struct piglit_dma_buf *buf);
};

struct piglit_gl_framework*
piglit_gl_framework_create(const struct piglit_gl_ctx_flavor *flavor,
			   const struct piglit_gl_test_config *test_config);

bool
piglit_gl_framework_init(struct piglit_gl_framework *gl_fw,
			 const struct piglit_gl_ctx_flavor *flavor,
                         const struct piglit_gl_test_config *test_config);

void
piglit_gl_framework_teardown(struct piglit_gl_framework *gl_fw);

#endif /* PIGLIT_GL_FRAMEWORK_H */
