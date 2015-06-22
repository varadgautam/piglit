/*
 * Copyright © 2014 Advanced Micro Devices, Inc.
 * All rights reserved.
 * Copyright 2015 Intel Corporation
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
 *
 * Authors:
 *    Marek Olšák <marek.olsak@amd.com>
 *    Laura Ekstrand <laura@jlekstrand.net>
 *
 * Adapted to demonstrate a bug in ClientWaitSync by Laura Ekstrand
 * <laura@jlekstrand.net>, January 2015
 */

#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_compat_version = 10;
	config.window_visual = PIGLIT_GL_VISUAL_RGB | PIGLIT_GL_VISUAL_DOUBLE;

PIGLIT_GL_TEST_CONFIG_END

#define BUF_SIZE (12 * 4 * sizeof(float))

void
piglit_init(int argc, char **argv)
{
	piglit_require_gl_version(15);
	piglit_require_extension("GL_ARB_buffer_storage");
	piglit_require_extension("GL_ARB_map_buffer_range");
	piglit_require_extension("GL_ARB_copy_buffer");
	piglit_require_extension("GL_ARB_sync");

	piglit_ortho_projection(piglit_width, piglit_height, GL_FALSE);
}

bool
create_mapped_buffer(GLuint *buffer, GLfloat **map, GLboolean coherent,
		     GLboolean client_storage)
{
	glGenBuffers(1, buffer);
	glBindBuffer(GL_ARRAY_BUFFER, *buffer);
	glBufferStorage(GL_ARRAY_BUFFER, BUF_SIZE, NULL,
			GL_MAP_WRITE_BIT |
			GL_MAP_PERSISTENT_BIT |
			(coherent ? GL_MAP_COHERENT_BIT : 0) |
			GL_DYNAMIC_STORAGE_BIT |
			(client_storage ? GL_CLIENT_STORAGE_BIT : 0));

	piglit_check_gl_error(GL_NO_ERROR);

	*map = glMapBufferRange(GL_ARRAY_BUFFER, 0, BUF_SIZE,
			        GL_MAP_WRITE_BIT |
			        GL_MAP_PERSISTENT_BIT |
			        (coherent ? GL_MAP_COHERENT_BIT : 0));

	piglit_check_gl_error(GL_NO_ERROR);

	if (!*map)
		return false;

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return true;
}

static enum piglit_result result = PIGLIT_PASS;
static float array[] = {
	17, 13, 0,
	17, 18, 0,
	12, 13, 0,
	12, 18, 0,
	27, 13, 0,
	27, 18, 0,
	22, 13, 0,
	22, 18, 0,
	37, 13, 0,
	37, 18, 0,
	32, 13, 0,
	32, 18, 0,
	47, 13, 0,
	47, 18, 0,
	42, 13, 0,
	42, 18, 0
};

void
read_subtest(GLboolean coherent, GLboolean client_storage)
{
	GLuint buffer;
	GLfloat *map;
	bool pass = true;

	int i;
	GLuint srcbuf;
	GLsync fence;
	GLenum wait_cond = GL_TIMEOUT_EXPIRED;
	int try_counter = 0;

	if (!create_mapped_buffer(&buffer, &map, coherent, client_storage)) {
		piglit_report_subtest_result(PIGLIT_FAIL,
			"read%s%s", coherent ? " coherent" : "",
			client_storage ? " client-storage" : "");
		result = PIGLIT_FAIL;
		return;
	}

	glClear(GL_COLOR_BUFFER_BIT);
	glGenBuffers(1, &srcbuf);
	glBindBuffer(GL_COPY_READ_BUFFER, srcbuf);
	glBufferData(GL_COPY_READ_BUFFER, BUF_SIZE, array, GL_STATIC_DRAW);

	/* Copy some data to the mapped buffer and check if the CPU can see it. */
	glBindBuffer(GL_COPY_WRITE_BUFFER, buffer);
	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER,
			    0, 0, BUF_SIZE);

	glBindBuffer(GL_COPY_READ_BUFFER, 0);
	glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
	glDeleteBuffers(1, &srcbuf);

	if (!coherent)
		glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);

	/*
	 * Wait for the GPU to flush.
	 * This should only take one try because ClientWaitSync with
	 * GL_TIMEOUT_IGNORED should wait until the signal happens and never
	 * return GL_TIMEOUT_EXPIRED.
	 */
	fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	while ((wait_cond == GL_TIMEOUT_EXPIRED) && (try_counter < 100)){
		wait_cond = glClientWaitSync(fence,
					     GL_SYNC_FLUSH_COMMANDS_BIT,
					     GL_TIMEOUT_IGNORED);
		printf("glClientWaitSync returned %s.\n",
		       piglit_get_gl_enum_name(wait_cond));
		try_counter++;

		if (wait_cond == GL_WAIT_FAILED) {
			/* Give up */
			pass = false;
			break;
		}
	}

	for (i = 0; i < ARRAY_SIZE(array); i++) {
		if (map[i] != array[i]) {
			printf("Probe [%i] failed. Expected: %f  Observed: %f\n",
			       i, array[i], map[i]);
			pass = false;
		}
	}

	if (try_counter > 1) {
		printf("glClientWaitSync called more than once"
		       " (%d total times).\n", try_counter);
		pass = false;
	}

	piglit_report_subtest_result(pass ? PIGLIT_PASS : PIGLIT_FAIL,
		"read%s%s", coherent ? " coherent" : "",
		client_storage ? " client-storage" : "");

	if (!pass)
		result = PIGLIT_FAIL;
}

enum piglit_result
piglit_display(void)
{
	/* !Coherent read subtests: require MemoryBarrier */
	if (piglit_is_extension_supported(
	       "GL_ARB_shader_image_load_store")) {
		read_subtest(false, false);
		read_subtest(false, true);
	}

	/* Coherent read subtests */
	read_subtest(true, false);
	read_subtest(true, true);

	return result;
}
