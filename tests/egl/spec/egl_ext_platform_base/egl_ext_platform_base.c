/* 
 * Copyright 2014 Intel Corporation
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
 * \file
 * \brief Tests for EGL_EXT_platform_base and layered extensions.
 *
 * For each platform requested on the command line, the test will call the
 * functions added by EGL_EXT_platform_base, validating the functions'
 * behavior, then destroy all EGL resources for that platform. If for any
 * platform the test fails to connect to a display, then the test skips.
 *
 * To catch errors in EGL's internal dispatch tables, the test creates all EGL
 * resources for all requested platforms before proceeding to destroy the EGL
 * resources.
 */

#include "common.h"

#include <stdio.h>
#include <unistd.h>

#include "piglit-util.h"
#include "piglit-util-egl.h"

EGLDisplay (*eglGetPlatformDisplayEXT)(EGLenum platform, void *native_display, const EGLint *attrib_list);
EGLSurface (*eglCreatePlatformWindowSurfaceEXT)(EGLDisplay dpy, EGLConfig config, void *native_window, const EGLint *attrib_list);
EGLSurface (*eglCreatePlatformPixmapSurfaceEXT)(EGLDisplay dpy, EGLConfig config, void *native_pixmap, const EGLint *attrib_list);

static const char *prog_name;

void
error(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    printf("%s: error: ", prog_name);
    vprintf(fmt, ap);
    printf("\n");
    va_end(ap);
}

static void
print_usage(void)
{
	static const char *usage =
		"usage: %1$s PLATFORM[,PLATFORM[,PLATFORM[...]]]\n"
		"\n"
		"PLATFORM must be one of 'x11', 'wayland', or 'gbm'.\n"
		"\n"
		"At least one platform must be given. The same platform may\n"
		"be given multiple times.\n"
		"\n"
		"Examples:\n"
		"    %1$s x11\n"
		"    %1$s wayland\n"
		"    %1$s gbm,x11,wayland\n"
		"    %1$s x11,wayland,x11,gbm\n"
		;
	printf(usage, prog_name);
}

static void
usage_error(void)
{
	printf("usage_error: %s\n", prog_name);
	printf("\n");
	print_usage();
	piglit_report_result(PIGLIT_FAIL);
}

static bool
streq(const char *a, const char *b)
{
	return strcmp(a, b) == 0;
}

static void
parse_args(int argc, char **argv,
	   const char ***out_platform_list)
{
	int i = 0;
	const char **platform_list = NULL;

	/* Discard common Piglit arguments. */
	piglit_strip_arg(&argc, argv, "-auto");
	piglit_strip_arg(&argc, argv, "-fbo");

	/* Discard argv[0]. */
	--argc;
	++argv;

	if (argc < 1) {
		usage_error();
	}

	platform_list = piglit_split_string_to_array(argv[0], ",");
	--argc;
	++argv;

	if (platform_list == NULL || platform_list[0] == NULL) {
		usage_error();
	}

	/* Validate platform list. */
	for (i = 0; platform_list[i] != NULL; ++i) {
		if (!streq(platform_list[i], "x11") &&
		    !streq(platform_list[i], "wayland") &&
		    !streq(platform_list[i], "gbm")) {
			usage_error();
		}
	}

	*out_platform_list = platform_list;
}

static void
init_egl_funcs(void)
{
	eglGetPlatformDisplayEXT = (void*) eglGetProcAddress("eglGetPlatformDisplayEXT");
	eglCreatePlatformWindowSurfaceEXT = (void*) eglGetProcAddress("eglCreatePlatformWindowSurfaceEXT");
	eglCreatePlatformPixmapSurfaceEXT = (void*) eglGetProcAddress("eglCreatePlatformPixmapSurfaceEXT");

	if (!eglGetPlatformDisplayEXT ||
	    !eglCreatePlatformWindowSurfaceEXT ||
	    !eglCreatePlatformPixmapSurfaceEXT) {
		printf("%s: error: failed to get all EGL_EXT_platform_base proc "
		       "addresess\n", prog_name);
		piglit_report_result(PIGLIT_FAIL);
	}
}

static enum piglit_result
test_platforms(const char **platforms)
{
	struct pgl_egl_resources **egl_resources = NULL;
	int num_platforms = 0;
	int i = 0;

	/* Count number of platforms. */
	for (i = 0; platforms[i] != NULL; ++i) {
		++num_platforms;
	}

	egl_resources = calloc(num_platforms, sizeof(*egl_resources));
	if (!egl_resources) {
		printf("%s: error: out of memory\n", prog_name);
		piglit_report_result(PIGLIT_FAIL);
	}

	/* Setup each platform. */
	for (i = 0; i < num_platforms; ++i) {
		enum piglit_result result = PIGLIT_SKIP;

		if (streq(platforms[i], "x11")) {
			pgl_x11_setup(&result, &egl_resources[i]);
		} else if (streq(platforms[i], "wayland")) {
			pgl_wl_setup(&result, &egl_resources[i]);
		} else if (streq(platforms[i], "gbm")) {
			pgl_gbm_setup(&result, &egl_resources[i]);
		} else {
			printf("%s: internal error: unexpected platform '%s'\n",
			       prog_name, platforms[i]);
			result = PIGLIT_FAIL;
		}

		if (result != PIGLIT_PASS) {
			return result;
		}
	}

	/* We intentionally setup all platforms before tearing any down. This
	 * catches possible errors EGL's internal dispatch table.
	 */

	/* Teardown each platform. */
	for (i = 0; i < num_platforms; ++i) {
		enum piglit_result result = PIGLIT_SKIP;

		if (egl_resources[i] == NULL) {
			continue;
		} else if (streq(platforms[i], "x11")) {
			pgl_x11_teardown(egl_resources[i], &result);
		} else if (streq(platforms[i], "wayland")) {
			pgl_wl_teardown(egl_resources[i], &result);
		} else if (streq(platforms[i], "gbm")) {
			pgl_gbm_teardown(egl_resources[i], &result);
		} else {
			printf("%s: internal error: unexpected platform '%s'\n",
			       prog_name, platforms[i]);
			result = PIGLIT_FAIL;
		}

		if (result != PIGLIT_PASS) {
			return result;
		}
	}

	free(egl_resources);
	return PIGLIT_PASS;
}

int
main(int argc, char **argv)
{
	enum piglit_result result = PIGLIT_SKIP;
	const char **platform_list = NULL;

	prog_name = basename(argv[0]);

	parse_args(argc, argv, &platform_list);
	piglit_require_egl_extension(EGL_NO_DISPLAY, "EGL_EXT_platform_base");
	init_egl_funcs();
	result = test_platforms(platform_list);
	piglit_report_result(result);

	assert(!"unreachable");
        return EXIT_FAILURE;
}
