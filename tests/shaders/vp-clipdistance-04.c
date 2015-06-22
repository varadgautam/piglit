/*
 * Copyright © 2009 Intel Corporation
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

/**
 * \file vp-clipdistance-04.c
 * Validate that result.clip[] is pre-initialized with 0.
 *
 * Each block enables one clip plane but writes a distance for a different
 * clip plane.  Since clipping only happens for clip distances < 0, there
 * should be no clipping.
 *
 * \author Ian Romanick <ian.d.romanick@intel.com>
 */

#include "piglit-util-gl.h"

#define TEST_ROWS  1
#define TEST_COLS  6
#define BOX_SIZE   32

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_compat_version = 10;

	config.window_width = (((BOX_SIZE+1)*TEST_COLS)+1);
	config.window_height = (((BOX_SIZE+1)*TEST_ROWS)+1);
	config.window_visual = PIGLIT_GL_VISUAL_RGB | PIGLIT_GL_VISUAL_DOUBLE;

PIGLIT_GL_TEST_CONFIG_END

static const char vertex_source_template[] =
	"!!ARBvp1.0\n"
	"OPTION	NV_vertex_program2;\n"
	"MOV	result.clip[%d], vertex.texcoord[0].x;\n"
	PIGLIT_VERTEX_PROGRAM_MVP_TRANSFORM
	"END\n"
	;

static const char fragment_source[] =
	"!!ARBfp1.0\n"
	"MOV	result.color, {0.0, 1.0, 0.0, 1.0};\n"
	"END"
	;

/**
 * \name Handles to programs.
 */
/*@{*/
static GLint progs[TEST_COLS];
static GLint frag_prog;
/*@}*/


static const GLfloat clear_color[4] = { 0.5, 0.5, 0.5, 1.0 };


enum piglit_result
piglit_display(void)
{
	static const GLfloat color[4] = { 0.0, 1.0, 0.0, 1.0 };
	enum piglit_result result = PIGLIT_PASS;
	unsigned i;

	glClear(GL_COLOR_BUFFER_BIT);

	for (i = 0; i < ARRAY_SIZE(progs); i++) {
		const int x = 1 + (i * (BOX_SIZE + 1));
		const GLenum p = GL_CLIP_PLANE0 + ((i + 1) % 6);

		glBindProgramARB(GL_VERTEX_PROGRAM_ARB, progs[i]);
		glEnable(p);
		piglit_draw_rect_tex(x, 1, BOX_SIZE, BOX_SIZE,
				     1.0, 1.0, -2.0, 0.0);
		glDisable(p);

		if (!piglit_probe_pixel_rgb(x + (BOX_SIZE / 2),
					    1 + (BOX_SIZE / 2),
					    color)) {
			result = PIGLIT_FAIL;
		}
	}

	piglit_present_results();
	return result;
}


void
piglit_init(int argc, char **argv)
{
	unsigned i;

	(void) argc;
	(void) argv;

	piglit_require_vertex_program();
	piglit_require_fragment_program();
	piglit_require_extension("GL_NV_vertex_program2_option");
	piglit_ortho_projection(piglit_width, piglit_height, GL_FALSE);

	for (i = 0; i < ARRAY_SIZE(progs); i++) {
		char shader_source[1024];

		snprintf(shader_source, sizeof(shader_source),
			 vertex_source_template, i);

		progs[i] = piglit_compile_program(GL_VERTEX_PROGRAM_ARB,
						  shader_source);
	}

	glEnable(GL_FRAGMENT_PROGRAM_ARB);
	glEnable(GL_VERTEX_PROGRAM_ARB);

	frag_prog = piglit_compile_program(GL_FRAGMENT_PROGRAM_ARB,
					   fragment_source);
	glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, frag_prog);

	glClearColor(clear_color[0],
		     clear_color[1],
		     clear_color[2],
		     clear_color[3]);
}
