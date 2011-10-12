/*
 * Copyright (c) The Piglit project 2007
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

#if defined(_WIN32)
#include <windows.h>
#endif

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "config.h"
#if defined(HAVE_SYS_TIME_H) && defined(HAVE_SYS_RESOURCE_H) && defined(HAVE_SETRLIMIT)
#include <sys/time.h>
#include <sys/resource.h>
#define USE_SETRLIMIT
#endif

#include "piglit-util.h"

void piglit_glutInit(int argc, char **argv)
{
	glutInit(&argc, argv);

#if defined USE_EGLUT && defined USE_OPENGL
	glutInitAPIMask(GLUT_OPENGL_BIT);
#elif defined USE_EGLUT && defined USE_OPENGL_ES1
	glutInitAPIMask(GLUT_OPENGL_ES1_BIT);
#elif defined USE_EGLUT && defined USE_OPENGL_ES2
	glutInitAPIMask(GLUT_OPENGL_ES2_BIT);
#elif defined USE_EGLUT
#	error
#endif
}

bool piglit_is_gles()
{
	const char *version_string = (const char *) glGetString(GL_VERSION);
	return strncmp("OpenGL ES ", version_string, 10) == 0;
}

int piglit_get_gl_version()
{
	const char *version_string = (const char *) glGetString(GL_VERSION);
	const char *version_number_string;
	int scanf_count;
	int major;
	int minor;

	/* skip to version number */
	if (strncmp("OpenGL ES ", version_string, 10) == 0)
		version_number_string = version_string + 10;
	else
		version_number_string = version_string;

	/* Interpret version number */
	scanf_count = sscanf(version_number_string, "%i.%i", &major, &minor);
	if (scanf_count != 2) {
		printf("Unable to interpret GL_VERSION string: %s\n",
		       version_string);
		piglit_report_result(PIGLIT_FAIL);
		exit(1);
	}
	return 10*major+minor;
}

bool piglit_is_extension_supported(const char *name)
{
	char *extensions;
	bool found = false;
	char *i;

	assert(name != NULL);
	extensions = strdup((const char*) glGetString(GL_EXTENSIONS));
	for (i = strtok(extensions, " "); i != NULL; i = strtok(NULL, " ")) {
		if (strcmp(name, i) == 0) {
			found = true;
			break;
		}
	}
	free(extensions);
	return found;
}

void piglit_require_gl_version(int required_version_times_10)
{
	if (piglit_is_gles() ||
	    piglit_get_gl_version() < required_version_times_10) {
		printf("Test requires GL version %g\n",
		       required_version_times_10 / 10.0);
		piglit_report_result(PIGLIT_SKIP);
		exit(1);
	}
}

void piglit_require_extension(const char *name)
{
	if (!piglit_is_extension_supported(name)) {
		printf("Test requires %s\n", name);
		piglit_report_result(PIGLIT_SKIP);
		exit(1);
	}
}

void piglit_require_not_extension(const char *name)
{
	if (piglit_is_extension_supported(name)) {
		piglit_report_result(PIGLIT_SKIP);
		exit(1);
	}
}

const char* piglit_get_gl_error_name(GLenum error)
{
#define CASE(x) case x: return #x; 
    switch (error) {
    CASE(GL_INVALID_ENUM)
    CASE(GL_INVALID_OPERATION)
    CASE(GL_INVALID_VALUE)
    CASE(GL_NO_ERROR)
    CASE(GL_OUT_OF_MEMORY)
    /* enums that are not available everywhere */
#if defined(GL_STACK_OVERFLOW)
    CASE(GL_STACK_OVERFLOW)
#endif
#if defined(GL_STACK_UNDERFLOW)
    CASE(GL_STACK_UNDERFLOW)
#endif
#if defined(GL_INVALID_FRAMEBUFFER_OPERATION)
    CASE(GL_INVALID_FRAMEBUFFER_OPERATION)
#endif
    default:
        return "(unrecognized error)";
    }
#undef CASE
}

void piglit_check_gl_error(GLenum expected_error, enum piglit_result result)
{
	GLenum actual_error;

	actual_error = glGetError();
	if (actual_error == expected_error) {
		return;
	}

	/*
	 * If the lookup of the error's name is successful, then print
	 *     Unexpected GL error: NAME 0xHEX
	 * Else, print
	 *     Unexpected GL error: 0xHEX
	 */
	printf("Unexpected GL error: %s 0x%x\n",
               piglit_get_gl_error_name(actual_error), actual_error);

	/* Print the expected error, but only if an error was really expected. */
	if (expected_error != GL_NO_ERROR) {
		printf("Expected GL error: %s 0x%x\n",
		piglit_get_gl_error_name(expected_error), expected_error);
        }

	piglit_report_result(result);
}

/* These texture coordinates should have 1 or -1 in the major axis selecting
 * the face, and a nearly-1-or-negative-1 value in the other two coordinates
 * which will be used to produce the s,t values used to sample that face's
 * image.
 */
GLfloat cube_face_texcoords[6][4][3] = {
	{ /* GL_TEXTURE_CUBE_MAP_POSITIVE_X */
		{1.0,  0.99,  0.99},
		{1.0,  0.99, -0.99},
		{1.0, -0.99, -0.99},
		{1.0, -0.99,  0.99},
	},
	{ /* GL_TEXTURE_CUBE_MAP_POSITIVE_Y */
		{-0.99, 1.0, -0.99},
		{ 0.99, 1.0, -0.99},
		{ 0.99, 1.0,  0.99},
		{-0.99, 1.0,  0.99},
	},
	{ /* GL_TEXTURE_CUBE_MAP_POSITIVE_Z */
		{-0.99,  0.99, 1.0},
		{-0.99, -0.99, 1.0},
		{ 0.99, -0.99, 1.0},
		{ 0.99,  0.99, 1.0},
	},
	{ /* GL_TEXTURE_CUBE_MAP_NEGATIVE_X */
		{-1.0,  0.99, -0.99},
		{-1.0,  0.99,  0.99},
		{-1.0, -0.99,  0.99},
		{-1.0, -0.99, -0.99},
	},
	{ /* GL_TEXTURE_CUBE_MAP_NEGATIVE_Y */
		{-0.99, -1.0,  0.99},
		{-0.99, -1.0, -0.99},
		{ 0.99, -1.0, -0.99},
		{ 0.99, -1.0,  0.99},
	},
	{ /* GL_TEXTURE_CUBE_MAP_NEGATIVE_Z */
		{ 0.99,  0.99, -1.0},
		{-0.99,  0.99, -1.0},
		{-0.99, -0.99, -1.0},
		{ 0.99, -0.99, -1.0},
	},
};

const char *cube_face_names[6] = {
	"POSITIVE_X",
	"POSITIVE_Y",
	"POSITIVE_Z",
	"NEGATIVE_X",
	"NEGATIVE_Y",
	"NEGATIVE_Z",
};

const GLenum cube_face_targets[6] = {
	GL_TEXTURE_CUBE_MAP_POSITIVE_X,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
};

/** Returns the line in the program string given the character position. */
int FindLine(const char *program, int position)
{
	int i, line = 1;
	for (i = 0; i < position; i++) {
		if (program[i] == '0')
			return -1; /* unknown line */
		if (program[i] == '\n')
			line++;
	}
	return line;
}

void
piglit_report_result(enum piglit_result result)
{
	fflush(stderr);

	if (result == PIGLIT_PASS) {
		printf("PIGLIT: {'result': 'pass' }\n");
		fflush(stdout);
		exit(0);
	} else if (result == PIGLIT_SKIP) {
		printf("PIGLIT: {'result': 'skip' }\n");
		fflush(stdout);
		exit(0);
	} else if (result == PIGLIT_WARN) {
		printf("PIGLIT: {'result': 'warn' }\n");
		fflush(stdout);
		exit(0);
	} else {
		printf("PIGLIT: {'result': 'fail' }\n");
		fflush(stdout);
		exit(1);
	}
}

float piglit_tolerance[4] = { 0.01, 0.01, 0.01, 0.01 };

void
piglit_set_tolerance_for_bits(int rbits, int gbits, int bbits, int abits)
{
	int bits[4] = {rbits, gbits, bbits, abits};
	int i;

	for (i = 0; i < 4; i++) {
		if (bits[i] < 2) {
			/* Don't try to validate channels when there's only 1
			 * bit of precision (or none).
			 */
			piglit_tolerance[i] = 1.0;
		} else {
			piglit_tolerance[i] = 3.0 / (1 << bits[i]);
		}
	}
}


#ifndef HAVE_STRCHRNUL
char *strchrnul(const char *s, int c)
{
	char *t = strchr(s, c);

	return (t == NULL) ? ((char *) s + strlen(s)) : t;
}
#endif


void
piglit_set_rlimit(unsigned long lim)
{
#if defined(USE_SETRLIMIT)
	struct rlimit rl;
	if (getrlimit(RLIMIT_AS, &rl) != -1) {
		printf("Address space limit = %lu, max = %lu\n",
		       (unsigned long) rl.rlim_cur,
		       (unsigned long) rl.rlim_max);

		if (rl.rlim_max > lim) {
			printf("Resetting limit to %lu.\n", lim);

			rl.rlim_cur = lim;
			rl.rlim_max = lim;
			if (setrlimit(RLIMIT_AS, &rl) == -1) {
				printf("Could not set rlimit "
				       "due to: %s (%d)\n",
				       strerror(errno), errno);
			}
		}
	}

	printf("\n");
#else
	printf("Cannot reset rlimit on this platform.\n\n");
#endif
}

void piglit_print_buffer(int width, int height, GLenum format, GLenum type) {
	int x;
	int y;

	uint8_t *pixels = 0;
	int components = 0;
	int component_size = 0;
	int pixel_size = 0;

	int x_tick_width = 0;
	int y_tick_width = 0;
	int component_width = 0;
	int pixel_width = 0;
	int cell_width = 0;

	switch (type) {
	case GL_UNSIGNED_BYTE:
		component_size = 1;
		component_width = 2;
		break;
	case GL_UNSIGNED_INT:
		component_size = 4;
		component_width = 8;
		break;
#ifdef USE_OPENGL
	case GL_UNSIGNED_INT_24_8:
		/* print format: dddddd|ss */
		component_size = 4;
		component_width = 9;
		break;
#endif
	case GL_FLOAT:
		/* print format: 1234.123456 */
		component_size = 4;
		component_width = 11;
		break;
	default:
		assert(!"not implemented");
		break;
	}

	switch (format) {
	case GL_DEPTH_COMPONENT:
	case GL_DEPTH_STENCIL:
		components = 1;
		break;
	case GL_RGB:
		components = 3;
		break;
	default:
		assert(!"not implemented");
		break;
	}

	x_tick_width = (int) ceil(log(width) / log(10));;
	y_tick_width = (int) ceil((log(height) / log(10)));
	pixel_width = components * (component_width + 1) - 1;
	cell_width = 1 + (int) fmax(x_tick_width, pixel_width);

	pixel_size = components * component_size;
	pixels = calloc(width * height, pixel_size);
	glReadPixels(0, 0, width, height, format, type, pixels);
	assert(glGetError() == 0);

	for (y = height - 1; y >= 0; --y) {
		/* Print y tick. */
		printf("%*d|", y_tick_width, y);

		/* Print row of pixels. */
		for (x = 0; x < width; ++x) {
			const uint8_t *pixel = &pixels[pixel_size * (y * width + x)];
			int c; // component iterator

			/* Print leading space. */
			const int leading_space = cell_width - pixel_width;
			int j;
			for (j = 0; j < leading_space; ++j) {
				printf(" ");
			}

			/* Print one pixel. */
			for (c = 0; c < components; ++c) {
			   switch (type) {
			   case GL_UNSIGNED_BYTE:
				   printf("%2.2x", ((GLubyte*)pixel)[c]);
				   break;
			   case GL_UNSIGNED_INT:
				   printf("%8x", ((GLuint*)pixel)[c]);
				   break;
   #ifdef USE_OPENGL
			   case GL_UNSIGNED_INT_24_8: {
				   GLuint depth = (((GLuint*)pixel)[c]) >> 8;
				   GLubyte stencil = (((GLuint*)pixel)[c]) & 0xff;
				   printf("%6x|%2x", depth, stencil);
				   break;
			   }
   #endif
			   case GL_FLOAT:
				   printf("%11.6f", ((GLfloat*)pixel)[c]);
				   break;
			   default:
				   assert(0);
				   break;
			   }

			   if (c != components - 1) {
			      printf(",");
			   }
			}
		}
		printf("\n");
	}

	/* Print x axis. */
	for (x = 0; x < y_tick_width; ++x) {
		printf("-");
	}
	printf("+");
	for (x = 0; x < cell_width * width; ++x) {
		printf("-");
	}
	printf("\n");

	/* Print x ticks. */
	for (x = 0; x < y_tick_width; ++x) {
		printf(" ");
	}
	printf("|");
	for (x = 0; x < width; ++x) {
		printf("%*d", cell_width, x);
	}
	printf("\n");
}
