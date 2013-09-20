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

#include <sys/stat.h>
#include <errno.h>

#include "piglit-util-gl-common.h"

void piglit_get_glsl_version(bool *es, int* major, int* minor)
{
	bool es_local;
	int major_local;
	int minor_local;

	const char *version_string;
	int c; /* scanf count */

	version_string = (const char*) glGetString(GL_SHADING_LANGUAGE_VERSION);
	es_local = strncmp("OpenGL ES", version_string, 9) == 0;
	if (es_local) {
		c = sscanf(version_string,
		           "OpenGL ES GLSL ES %i.%i",
		           &major_local,
		           &minor_local);
	} else {
		c = sscanf(version_string,
		           "%i.%i",
		           &major_local,
		           &minor_local);
	}
	assert(c == 2);

	/* Write outputs. */
	if (es != NULL)
		*es = es_local;
	if (major != NULL)
		*major = major_local;
	if (minor != NULL)
		*minor = minor_local;
}

/**
 * Convenience function to compile a GLSL shader from a file.
 */
GLuint
piglit_compile_shader(GLenum target, const char *filename)
{
	GLuint prog;
	struct stat st;
	int err;
	GLchar *prog_string;
	FILE *f;
	const char *source_dir;
	char filename_with_path[FILENAME_MAX];

	source_dir = getenv("PIGLIT_SOURCE_DIR");
	if (source_dir == NULL) {
		source_dir = SOURCE_DIR;
	}

	snprintf(filename_with_path, FILENAME_MAX - 1,
		 "%s/tests/%s", source_dir, filename);
	filename_with_path[FILENAME_MAX - 1] = 0;

	err = stat(filename_with_path, &st);
	if (err == -1) {
		fprintf(stderr, "Couldn't stat program %s: %s\n", filename_with_path, strerror(errno));
		fprintf(stderr, "You can override the source dir by setting the PIGLIT_SOURCE_DIR environment variable.\n");
		exit(1);
	}

	prog_string = malloc(st.st_size + 1);
	if (prog_string == NULL) {
		fprintf(stderr, "malloc\n");
		exit(1);
	}

	f = fopen(filename_with_path, "r");
	if (f == NULL) {
		fprintf(stderr, "Couldn't open program: %s\n", strerror(errno));
		exit(1);
	}
	fread(prog_string, 1, st.st_size, f);
	prog_string[st.st_size] = '\0';
	fclose(f);

	prog = piglit_compile_shader_text(target, prog_string);

	free(prog_string);

	return prog;
}

/** Return a string name for a shader target enum */
static const char *
shader_name(GLenum target)
{
   switch (target) {
   case GL_VERTEX_SHADER:
      return "vertex";
#if defined PIGLIT_USE_OPENGL
   case GL_GEOMETRY_SHADER:
      return "geometry";
#endif
   case GL_FRAGMENT_SHADER:
      return "fragment";
   default:
      assert(!"Unexpected shader target in shader_name()");
   }

   return "error";
}

/**
 * Convenience function to compile a GLSL shader.
 */
GLuint
piglit_compile_shader_text(GLenum target, const char *text)
{
	GLuint prog;
	GLint ok;

	piglit_require_GLSL();

	prog = glCreateShader(target);
	glShaderSource(prog, 1, (const GLchar **) &text, NULL);
	glCompileShader(prog);

	glGetShaderiv(prog, GL_COMPILE_STATUS, &ok);

	{
		GLchar *info;
		GLint size;

		glGetShaderiv(prog, GL_INFO_LOG_LENGTH, &size);
		info = malloc(size);

		glGetShaderInfoLog(prog, size, NULL, info);
		if (!ok) {
			fprintf(stderr, "Failed to compile %s shader: %s\n",
				shader_name(target),
				info);

			fprintf(stderr, "source:\n%s", text);
			piglit_report_result(PIGLIT_FAIL);
		}
		else if (0) {
			/* Enable this to get extra compilation info.
			 * Even if there's no compilation errors, the info
			 * log may have some remarks.
			 */
			fprintf(stderr, "Shader compiler warning: %s\n", info);
		}
		free(info);
	}

	return prog;
}

static GLboolean
link_check_status(GLint prog, FILE *output)
{
	GLchar *info = NULL;
	GLint size;
	GLint ok;

	piglit_require_GLSL();

	glGetProgramiv(prog, GL_LINK_STATUS, &ok);

	/* Some drivers return a size of 1 for an empty log.  This is the size
	 * of a log that contains only a terminating NUL character.
	 */
	glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &size);
	if (size > 1) {
		info = malloc(size);
		glGetProgramInfoLog(prog, size, NULL, info);
	}

	if (!ok) {
		fprintf(output, "Failed to link: %s\n",
			(info != NULL) ? info : "<empty log>");
	}
	else if (0 && info != NULL) {
		/* Enable this to get extra linking info.
		 * Even if there's no link errors, the info log may
		 * have some remarks.
		 */
		printf("Linker warning: %s\n", info);
	}

	free(info);

	return ok;
}

GLboolean
piglit_link_check_status(GLint prog)
{
	return link_check_status(prog, stderr);
}

/**
 * Check link status
 *
 * Similar to piglit_link_check_status except it logs error messages
 * to standard output instead of standard error.  This is useful for
 * tests that want to produce negative link results.
 *
 * \sa piglit_link_check_status
 */
GLboolean
piglit_link_check_status_quiet(GLint prog)
{
	return link_check_status(prog, stdout);
}


GLint piglit_link_simple_program(GLint vs, GLint fs)
{
	return piglit_link_simple_program_multiple_shaders(vs, fs, 0);
}


/**
 * Builds a program from optional VS and FS sources, but does not link
 * it.  If there is a compile failure, the test is terminated.
 */
GLuint
piglit_build_simple_program_unlinked(const char *vs_source,
				     const char *fs_source)
{
	return piglit_build_simple_program_unlinked_multiple_shaders(
			GL_VERTEX_SHADER, vs_source,
			GL_FRAGMENT_SHADER, fs_source,
			0);
}


/**
 * Builds and links a program from optional VS and FS sources,
 * throwing PIGLIT_FAIL on error.
 */
GLint
piglit_build_simple_program(const char *vs_source, const char *fs_source)
{
	return piglit_build_simple_program_multiple_shaders(
			GL_VERTEX_SHADER, vs_source,
			GL_FRAGMENT_SHADER, fs_source,
			0);
}

GLint piglit_link_simple_program_multiple_shaders(GLint shader1, ...)
{
	va_list ap;
	GLint prog, sh;

	piglit_require_GLSL();

	prog = glCreateProgram();

	va_start(ap, shader1);
	sh = shader1;

	while (sh != 0) {
		glAttachShader(prog, sh);
		sh = va_arg(ap, GLint);
	}

	va_end(ap);

	/* If the shaders reference piglit_vertex or piglit_tex, bind
	 * them to some fixed attribute locations so they can be used
	 * with piglit_draw_rect_tex() in GLES.
	 */
	glBindAttribLocation(prog, PIGLIT_ATTRIB_POS, "piglit_vertex");
	glBindAttribLocation(prog, PIGLIT_ATTRIB_TEX, "piglit_texcoord");

	glLinkProgram(prog);

	if (!piglit_link_check_status(prog)) {
		glDeleteProgram(prog);
		prog = 0;
	}

	return prog;
}

GLint
piglit_build_simple_program_unlinked_multiple_shaders_v(GLenum target1,
						        const char *source1,
						        va_list ap)
{
	GLuint prog;
	GLenum target;
	const char *source;

	piglit_require_GLSL();
	prog = glCreateProgram();

	source = source1;
	target = target1;

	while (target != 0) {
		GLuint shader = piglit_compile_shader_text(target, source);

		glAttachShader(prog, shader);
		glDeleteShader(shader);

		target  = va_arg(ap, GLenum);
		if (target != 0)
			source = va_arg(ap, char*);
	}

	return prog;
}

/**
 * Builds and links a program from optional sources,  but does not link
 * it. The last target must be 0. If there is a compile failure,
 * the test is terminated.
 *
 * example:
 * piglit_build_simple_program_unlinked_multiple_shaders(
 *				GL_VERTEX_SHADER,   vs,
 *				GL_GEOMETRY_SHADER, gs,
 *				GL_FRAGMENT_SHADER, fs,
 *				0);
 */
GLint
piglit_build_simple_program_unlinked_multiple_shaders(GLenum target1,
						      const char *source1,
						      ...)
{
	GLuint prog;
	va_list ap;

	va_start(ap, source1);

	prog = piglit_build_simple_program_unlinked_multiple_shaders_v(target1,
								       source1,
								       ap);
	va_end(ap);

	return prog;
}

/**
 * Builds and links a program from optional sources, throwing
 * PIGLIT_FAIL on error. The last target must be 0.
 */
GLint
piglit_build_simple_program_multiple_shaders(GLenum target1,
					    const char *source1,
					    ...)
{
	va_list ap;
	GLuint prog;

	va_start(ap, source1);

	prog = piglit_build_simple_program_unlinked_multiple_shaders_v(target1,
								       source1,
								       ap);

	va_end(ap);

	/* If the shaders reference piglit_vertex or piglit_tex, bind
	 * them to some fixed attribute locations so they can be used
	 * with piglit_draw_rect_tex() in GLES.
	 */
	glBindAttribLocation(prog, PIGLIT_ATTRIB_POS, "piglit_vertex");
	glBindAttribLocation(prog, PIGLIT_ATTRIB_TEX, "piglit_texcoord");

	glLinkProgram(prog);

	if (!piglit_link_check_status(prog)) {
		glDeleteProgram(prog);
		prog = 0;
		piglit_report_result(PIGLIT_FAIL);
	}

	return prog;
}
