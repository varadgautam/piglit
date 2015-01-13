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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Tapani Pälli <tapani.palli@intel.com>
 */

/** @file draw-buffers.c
 *
 * Tests GL_EXT_draw_buffers implementation
 *
 * Test iterates over valid and invalid arguments and checks that the
 * implementation returns correct error codes.
 *
 * GL_EXT_draw_buffers specification "Errors" section states:
 *
 *  "The INVALID_OPERATION error is generated if DrawBuffersEXT is called
 *   when the default framebuffer is bound and any of the following conditions
 *   hold:
 *    - <n> is zero,
 *    - <n> is greater than 1 and less than MAX_DRAW_BUFFERS_EXT,
 *    - <bufs> contains a value other than BACK or NONE.
 *
 *   The INVALID_OPERATION error is generated if DrawBuffersEXT is called
 *   when bound to a draw framebuffer object and any of the following
 *   conditions hold:
 *   - the <i>th value in <bufs> is not COLOR_ATTACHMENT<i>_EXT or NONE.
 *
 *   The INVALID_VALUE error is generated if DrawBuffersEXT is called
 *   with a value of <n> which is greater than MAX_DRAW_BUFFERS_EXT.
 *
 *   The INVALID_ENUM error is generated by FramebufferRenderbuffer if
 *   the <attachment> parameter is not one of the values listed in Table 4.x.
 *
 *   The INVALID_ENUM error is generated by FramebufferTexture2D if
 *   the <attachment> parameter is not one of the values listed in Table 4.x.
 *
 *   The INVALID_ENUM error is generated by GetFramebufferAttachmentParameteriv
 *   if the <attachment> parameter is not one of the values listed in Table 4.x."
 */

#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_es_version = 20;

PIGLIT_GL_TEST_CONFIG_END

#define TEXTURE_AMOUNT 3

static const GLenum valid_buffer_list[] = {
	GL_COLOR_ATTACHMENT0,
	GL_COLOR_ATTACHMENT1,
	GL_COLOR_ATTACHMENT2,
};

static const GLenum invalid_buffer_list[] = {
	GL_COLOR_ATTACHMENT0,
	GL_BACK,
	GL_COLOR_ATTACHMENT1,
};

static GLuint
create_fbo()
{
	GLuint fbo;
	GLuint depth;
	GLuint textures[TEXTURE_AMOUNT];
	GLint param;
	unsigned i;

	/* Generate fbo with TEXTURE_AMOUNT color attachments. */
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glGenTextures(TEXTURE_AMOUNT, textures);

	for (i = 0; i < TEXTURE_AMOUNT; i++) {
		glBindTexture(GL_TEXTURE_2D, textures[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, NULL);

		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
			GL_COLOR_ATTACHMENT0_EXT + i,
			GL_TEXTURE_2D,
			textures[i], 0);

	}

	/* Test adding invalid attachment. */
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
		GL_FALSE,
		GL_TEXTURE_2D,
		textures[0], 0);

	if (!piglit_check_gl_error(GL_INVALID_ENUM))
		return 0;

	/* Create a depth buffer. */
	glGenRenderbuffersEXT(1, &depth);
	glBindRenderbuffer(GL_RENDERBUFFER_EXT, depth);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT16, 4, 4);

	/* Test adding invalid renderbuffer. */
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
		GL_FALSE,
		GL_RENDERBUFFER_EXT,
		depth);

	if (!piglit_check_gl_error(GL_INVALID_ENUM))
		return 0;

	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
		GL_DEPTH_ATTACHMENT_EXT,
		GL_RENDERBUFFER_EXT,
		depth);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
		GL_FRAMEBUFFER_COMPLETE) {
		return 0;
	}

	/* Test invalid attachment with GetFramebufferAttachmentParameteriv. */
	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_FALSE,
		GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &param);

	if (!piglit_check_gl_error(GL_INVALID_ENUM))
		return 0;

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	return fbo;
}

static GLboolean
run_test(void)
{
	const GLenum back = GL_BACK;
	const GLenum att0 = GL_COLOR_ATTACHMENT0;
	GLuint fbo;
	GLint max_buffers;

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	/* Error cases when default framebuffer is bound. */
	glDrawBuffersEXT(0, &back);
	if (!piglit_check_gl_error(GL_INVALID_OPERATION))
		return false;

	glDrawBuffersEXT(2, &back);
	if (!piglit_check_gl_error(GL_INVALID_OPERATION))
		return false;

	glDrawBuffersEXT(3, &att0);
	if (!piglit_check_gl_error(GL_INVALID_OPERATION))
		return false;

	/* Positive case with default framebuffer. */
	glDrawBuffersEXT(1, &back);
	if (!piglit_check_gl_error(GL_NO_ERROR))
		return false;

	/* Create user fbo for rest of the tests. */
	fbo = create_fbo();
	if (!fbo || !piglit_check_gl_error(GL_NO_ERROR))
		return false;

	/* Error cases when user framebuffer is bound. */
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glDrawBuffersEXT(3, invalid_buffer_list);
	if (!piglit_check_gl_error(GL_INVALID_OPERATION))
		return false;

	glGetIntegerv(GL_MAX_DRAW_BUFFERS_EXT, &max_buffers);
	if (!piglit_check_gl_error(GL_NO_ERROR))
		return false;

	glDrawBuffersEXT(max_buffers + 1, valid_buffer_list);
	if (!piglit_check_gl_error(GL_INVALID_VALUE))
		return false;

	/* Positive case with user framebuffer. */
	glDrawBuffersEXT(TEXTURE_AMOUNT, valid_buffer_list);
	if (!piglit_check_gl_error(GL_NO_ERROR))
		return false;

	return true;
}


enum piglit_result
piglit_display(void)
{
	GLboolean pass = run_test();

	return pass ? PIGLIT_PASS : PIGLIT_FAIL;
}

void
piglit_init(int argc, char **argv)
{
	piglit_require_extension("GL_EXT_draw_buffers");
}