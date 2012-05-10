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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * @file dri2-getparam.c
 *
 * Test the DRI2GetParam protocol.
 *
 * For usage information, see usage_error();
 */

#include <stdio.h>

#include <X11/Xlib-xcb.h>
#include <xcb/dri2.h>
#include <xcb/xcbext.h>

#include "dri2-util.h"
#include "piglit-util.h"
#include "piglit-glx-util.h"

int piglit_width = 320;
int piglit_height = 240;

void
usage_error()
{
	const char *message =
		"usage:\n"
		"    dri2-getparam <testcase>\n"
		"\n"
		"testcases:\n"
		"   bad_drawable\n"
		"       Send DRI2GetParam with a bad drawable id.\n"
		"       Check that error BadDrawable is emitted.\n"
		"   param=unknown\n"
		"       Send DRI2GetParam(param=~0).\n"
		"       Check that no X error is emitted.\n"
		"       Check that reply has `is_param_recognized=false`.\n"
		;

	printf("%s", message);
	piglit_report_result(PIGLIT_FAIL);
}

static void
test_bad_drawable(xcb_connection_t *conn)
{
	xcb_generic_error_t *error = NULL;

	/* Obtain a unique id. Do not create the window. */
	xcb_window_t window = xcb_generate_id(conn);

	dri2_get_param(conn,
	               window,
	               0 /*param*/,
	               NULL,
	               NULL,
	               &error);
	dri2_check_error(BadDrawable, error);
}


static void
test_unknown_param(xcb_connection_t *conn, uint32_t drawable)
{
	xcb_generic_error_t *error = NULL;

	const uint32_t param = ~0;
	bool is_param_recognized;
	uint64_t value;

	dri2_get_param(conn,
	               drawable,
	               param,
	               &is_param_recognized,
	               &value,
	               &error);
	dri2_check_error(0, error);

	if (is_param_recognized) {
		printf("Expected (param=~0) to be unrecognized\n");
		piglit_report_result(PIGLIT_FAIL);
	}
}

int
main(int argc, char **argv) {
	Display *display;
	xcb_connection_t *conn;
	XVisualInfo *visual;
	Window window;
	GLXContext ctx;
	const char *testname;

	if (argc != 2)
		usage_error();

	display = piglit_get_glx_display();
	conn = XGetXCBConnection(display);
	visual = piglit_get_glx_visual(display);
	window = piglit_get_glx_window(display, visual);
	ctx = piglit_get_glx_context(display, visual);
	glXMakeCurrent(display, window, ctx);

	dri2_require_version(conn, 1, 4);

	testname = argv[1];
	if (!strcmp(testname, "bad_drawable"))
		test_bad_drawable(conn);
	else if (!strcmp(testname, "param=unknown"))
		test_unknown_param(conn, window);
	else
		usage_error();

	piglit_report_result(PIGLIT_PASS);
	return 0;
}
