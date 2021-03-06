/*
 * Copyright © 2009 Intel Corporation
 * Copyright © 2011 Marek Olšák <maraeo@gmail.com>
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
 *    Marek Olšák <maraeo@gmail.com>
 *
 * Based on fbo-clearmipmap by:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_compat_version = 10;

	config.window_width = 512;
	config.window_height = 256;
	config.window_visual = PIGLIT_GL_VISUAL_RGB | PIGLIT_GL_VISUAL_DOUBLE;

PIGLIT_GL_TEST_CONFIG_END

#define TEX_WIDTH 256
#define TEX_HEIGHT 256

static const float colors[][3] = {
	{1.0, 0.0, 0.0},
	{0.0, 1.0, 0.0},
	{0.0, 0.0, 1.0},
	{1.0, 1.0, 0.0},
	{0.0, 1.0, 1.0},
	{1.0, 0.0, 1.0},
	{0.5, 0.0, 0.5},
	{1.0, 1.0, 1.0},
};

static const char *sh_tex =
	"uniform sampler2D tex;"
	"void main()"
	"{"
	"   gl_FragColor = texture2D(tex, gl_TexCoord[0].xy);"
	"}";

static const char *sh_texgrad =
	"#extension GL_ARB_shader_texture_lod : enable\n"
	"uniform sampler2D tex;"
	"void main()"
	"{"
	"   gl_FragColor = texture2DGradARB(tex, gl_TexCoord[0].xy,"
	"                                   dFdx(gl_TexCoord[0].xy),"
	"                                   dFdy(gl_TexCoord[0].xy));"
	"}";

static GLint prog_tex, prog_texgrad;

void piglit_init(int argc, char **argv)
{
	GLuint tex, fb;
	GLenum status;
	int i, dim;

	piglit_require_GLSL();
	piglit_require_extension("GL_EXT_framebuffer_object");
	piglit_require_extension("GL_ARB_shader_texture_lod");

	prog_tex = piglit_build_simple_program(NULL, sh_tex);
	prog_texgrad = piglit_build_simple_program(NULL, sh_texgrad);

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	for (i = 0, dim = TEX_WIDTH; dim >0; i++, dim /= 2) {
		glTexImage2D(GL_TEXTURE_2D, i, GL_RGBA,
			     dim, dim,
			     0,
			     GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}
	if (!piglit_check_gl_error(GL_NO_ERROR))
		piglit_report_result(PIGLIT_FAIL);

	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glGenFramebuffersEXT(1, &fb);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);

	for (i = 0, dim = TEX_WIDTH; dim >0; i++, dim /= 2) {
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
					  GL_COLOR_ATTACHMENT0_EXT,
					  GL_TEXTURE_2D,
					  tex,
					  i);

		status = glCheckFramebufferStatusEXT (GL_FRAMEBUFFER_EXT);
		if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
			fprintf(stderr, "FBO incomplete\n");
			piglit_report_result(PIGLIT_SKIP);
		}

		glClearColor(colors[i][0],
			     colors[i][1],
			     colors[i][2],
			     0.0);
		glClear(GL_COLOR_BUFFER_BIT);

		if (!piglit_check_gl_error(GL_NO_ERROR))
		        piglit_report_result(PIGLIT_FAIL);
	}
	glDeleteFramebuffersEXT(1, &fb);
	glBindTexture(GL_TEXTURE_2D, tex);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-0.1, 0.1, -0.1, 0.1, 0.1, 1000.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(-0.5, -0.5, -1.2);
	glRotatef(68, 0, 1, 0);
	glScalef(2000, 1, 1);

	glEnable(GL_TEXTURE_2D);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	piglit_set_tolerance_for_bits(7, 7, 7, 7);

	printf("Left: texture2D, Right: texture2DGradARB\n");
}

static void draw_quad()
{
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(0, 0);
	glTexCoord2f(1, 0);
	glVertex2f(1, 0);
	glTexCoord2f(1, 1);
	glVertex2f(1, 1);
	glTexCoord2f(0, 1);
	glVertex2f(0, 1);
	glEnd();
}

enum piglit_result piglit_display(void)
{
	GLboolean pass = GL_TRUE;

	glViewport(0, 0, piglit_width, piglit_height);
	glClearColor(0.5, 0.5, 0.5, 0.5);
	glClear(GL_COLOR_BUFFER_BIT);

	glViewport(0, 0, piglit_width/2, piglit_height);
	glUseProgram(prog_tex);
	draw_quad();

	glViewport(piglit_width/2, 0, piglit_width/2, piglit_height);
	glUseProgram(prog_texgrad);
	draw_quad();

	if (!piglit_probe_rect_halves_equal_rgba(0, 0, piglit_width, piglit_height))
		pass = GL_FALSE;

	piglit_present_results();

	return pass ? PIGLIT_PASS : PIGLIT_FAIL;
}
