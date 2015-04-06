/*
 * Copyright © 2014 Intel Corporation
 * Copyright © 2015 Red Hat
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
 * \file draw.c
 *
 * A basic drawing test for GL_ARB_texture_stencil8 which ensures that
 * sampling occurs from the right position in the texture.
 *
 * It creates two stencil textures.  The first has a horizontal
 * gradient (0 -> 255 for stencil), and the second a
 * vertical gradient.
 *
 * The expected output is two squares, separated by a blue border.
 * The left half of the window is generated by stencil texturing, and drawn
 * in red.
 *
 *   Stencil
 *    (red)
 *
 *   0 --> 1
 *  --------
 *      1
 *      ^
 *      |
 *      0
 */

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "piglit-util-gl.h"

PIGLIT_GL_TEST_CONFIG_BEGIN

	config.supports_gl_core_version = 32;
	config.window_visual = PIGLIT_GL_VISUAL_RGB | PIGLIT_GL_VISUAL_DOUBLE;
	config.window_width = 256 + 3;
	config.window_height = 256 * 2 + 3;

PIGLIT_GL_TEST_CONFIG_END

float stencil_horiz_expected[256 * 256 * 3];
float stencil_vert_expected[256 * 256 * 3];

GLuint horiz_tex;
GLuint vert_tex;

/** A program that samples from stencil using usampler2D. */
GLint stencil_prog;

GLuint vao;

/******************************************************************************/

enum piglit_result
piglit_display(void)
{
	bool pass = true;

	glClearColor(0.0, 0.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(stencil_prog);

	/* Upper left corner: Stencil (black to red, left to right). */
	glViewport(0, 259, 256, 256);
	glBindTexture(GL_TEXTURE_2D, horiz_tex);
	piglit_draw_rect(-1, -1, 2, 2);
	if (!piglit_probe_image_rgb(0, 259, 256, 256, stencil_horiz_expected)) {
		printf("  FAIL: stencil (horizontal).\n");
		pass = false;
	}


	/* Lower left corner: Stencil (black to red, upwards). */
	glViewport(0, 0, 256, 256);
	glBindTexture(GL_TEXTURE_2D, vert_tex);
	piglit_draw_rect(-1, -1, 2, 2);
	if (!piglit_probe_image_rgb(0, 0, 256, 256, stencil_vert_expected)) {
		printf("  FAIL: stencil (vertical).\n");
		pass = false;
	}

	piglit_present_results();

	return pass ? PIGLIT_PASS : PIGLIT_FAIL;
}

/******************************************************************************/

/*
 * - Stencil ranges from 0 to 255.
 *
 * horiz_tex is left to right (increasing in the +x direction);
 * vert_tex is bottom to top (increasing in the +y direction).
 *
 * Also set up expected values.
 */
static void
setup_textures(void)
{
	int i;
	uint8_t horiz_data[256 * 256];
	uint8_t vert_data[256 * 256];

	for (i = 0; i < ARRAY_SIZE(horiz_data); i++) {
		unsigned x = i % 256;
		unsigned y = i / 256;
		horiz_data[i] = x;
		vert_data[i] = y;
	}

	glGenTextures(1, &horiz_tex);
	glBindTexture(GL_TEXTURE_2D, horiz_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_STENCIL_INDEX8,
		     256, 256, 0, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE,
		     horiz_data);
	if (!piglit_check_gl_error(GL_NO_ERROR))
		piglit_report_result(PIGLIT_FAIL);

	glGenTextures(1, &vert_tex);
	glBindTexture(GL_TEXTURE_2D, vert_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_STENCIL_INDEX8,
		     256, 256, 0, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE,
		     vert_data);
	if (!piglit_check_gl_error(GL_NO_ERROR))
		piglit_report_result(PIGLIT_FAIL);

	/* Set up expected values. */
	memset(stencil_horiz_expected, 0, sizeof(stencil_horiz_expected));
	memset(stencil_vert_expected, 0, sizeof(stencil_vert_expected));

	for (i = 0; i < 256 * 256; i++) {
		unsigned x = i % 256;
		unsigned y = i / 256;
		stencil_horiz_expected[3*i] = x / 255.0f;
		stencil_vert_expected[3*i]  = y / 255.0f;
	}


}

/**
 * Compile the shaders used by this program.
 *
 */
static void
setup_shaders(void)
{
	int loc;

	const char *vs =
	"#version 130\n"
	"in vec4 piglit_vertex;\n"
	"out vec2 texcoords;\n"
	"void main()\n"
	"{\n"
	"    gl_Position = piglit_vertex;\n"
	"    texcoords = (piglit_vertex.xy + 1.0) / 2.0;\n"
	"}\n";

	const char *fs_stencil =
	"#version 130\n"
	"in vec2 texcoords;\n"
	"uniform usampler2D tex;\n"
	"void main()\n"
	"{\n"
	"    uint stencil = texture(tex, texcoords).x;\n"
	"    gl_FragColor = vec4(float(stencil) / 255.0, 0, 0, 1);\n"
	"}\n";

	stencil_prog = piglit_build_simple_program(vs, fs_stencil);
	loc = glGetUniformLocation(stencil_prog, "tex");
	glUseProgram(stencil_prog);
	glUniform1i(loc, 0);

}

void
piglit_init(int argc, char **argv)
{
	piglit_require_extension("GL_ARB_texture_stencil8");

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	setup_textures();
	setup_shaders();
}
