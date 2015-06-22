/*
 * Copyright © 2015 Intel Corporation
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/** @file deletebuffers.c
 *
 * Tests that glDeleteBuffers() also removes the
 * glBindBufferBase()/glBindBufferRange() bindings along with the
 * usual glBindBuffer() binding.
 */

#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_compat_version = 10;
	config.window_visual = PIGLIT_GL_VISUAL_RGBA | PIGLIT_GL_VISUAL_DOUBLE;

PIGLIT_GL_TEST_CONFIG_END

void
piglit_init(int argc, char **argv)
{
	bool pass = true;
	GLuint bo[2];
	GLint binding;

	piglit_require_extension("GL_ARB_shader_storage_buffer_object");

	glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, 0, &binding);
	if (binding != 0) {
		fprintf(stderr, "Default SSBO binding should be 0, was %d\n",
			binding);
		piglit_report_result(PIGLIT_FAIL);
	}

	glGenBuffers(2, bo);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, bo[0]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 4, NULL, GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, bo[0]);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, bo[1]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 4, NULL, GL_STATIC_DRAW);
	glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, bo[1], 0, 4);

	glDeleteBuffers(2, bo);

	if (glIsBuffer(bo[0]) || glIsBuffer(bo[1])) {
		fprintf(stderr, "Failed to delete buffers\n");
		pass = false;
	}

	glGetIntegerv(GL_SHADER_STORAGE_BUFFER_BINDING, &binding);
	if (binding != 0) {
		printf("111 Failed to unbind glBindBuffer() buffer %d:\n"
		       "  binding set to %d, should be 0\n",
		       bo[1], binding);
		pass = false;
	}

	glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, 0, &binding);
	if (binding != 0) {
		printf("Failed to unbind glBindBufferBase() buffer %d:\n"
		       "  binding set to %d, should be 0\n",
		       bo[0], binding);
		pass = false;
	}

	glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, 1, &binding);
	if (binding != 0) {
		printf("Failed to unbind glBindBufferRange() buffer %d:\n"
		       "  binding set to %d, should be 0\n",
		       bo[1], binding);
		pass = false;
	}

	piglit_report_result(pass ? PIGLIT_PASS : PIGLIT_FAIL);
}

enum piglit_result piglit_display(void)
{
	/* UNREACHED */
	return PIGLIT_FAIL;
}

