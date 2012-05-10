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

#include "dri2-util.h"

#include <stdio.h>

#include <X11/Xlib-xcb.h>
#include <xcb/dri2.h>
#include <xcb/xcbext.h>

#include "piglit-util.h"

void
dri2_check_error_(uint32_t expect_error_code,
		  xcb_generic_error_t *actual_error,
		  const char *file, unsigned line)
{
	uint32_t actual_error_code = 0;

	if (actual_error)
		actual_error_code = actual_error->error_code;

	if (actual_error_code == expect_error_code) {
		return;
	} else if (expect_error_code == 0) {
		printf("Unexpected X error %d\n", actual_error_code);
	} else if (expect_error_code != 0 && actual_error_code == 0) {
		printf("Expected X error %d, but no error found\n",
		       expect_error_code);
	} else if (expect_error_code != 0 && actual_error_code != 0) {
		printf("Expected X error %d. Actual error is %d\n",
		       expect_error_code, actual_error_code);
	} else {
		abort();
	}

	printf("Test failed at %s:%d\n", file, line);
	piglit_report_result(PIGLIT_FAIL);
}

void
dri2_require_dri2(xcb_connection_t *conn)
{
	xcb_extension_t ext;
	const xcb_query_extension_reply_t *reply;

	ext.name = "DRI2";
	ext.global_id = xcb_generate_id(conn);

	reply = xcb_get_extension_data(conn, &ext);
	if (!reply->present) {
		printf("Test requires DRI2\n");
		piglit_report_result(PIGLIT_SKIP);
	}
}

void
dri2_require_version(xcb_connection_t *conn, int major, int minor)
{
	xcb_dri2_query_version_cookie_t cookie;
	xcb_dri2_query_version_reply_t *reply;

	xcb_generic_error_t *error = NULL;

	dri2_require_dri2(conn);

	/* The version supplied in the request is the client version. */
	cookie = xcb_dri2_query_version(conn, major, minor);
	reply = xcb_dri2_query_version_reply(conn, cookie, &error);
	dri2_check_error(0, error);

	if (reply->major_version < major || reply->minor_version < minor) {
		printf("Test requires DRI2 version %d.%d. "
		       "X server has version %d.%d.\n",
		       major, minor,
		       reply->major_version, reply->minor_version);
		piglit_report_result(PIGLIT_SKIP);
	}
}

bool
dri2_get_param(xcb_connection_t *conn,
	       xcb_drawable_t drawable,
	       uint32_t param,
	       bool *is_param_recognized,
	       uint64_t *value,
	       xcb_generic_error_t **error)
{
	xcb_dri2_get_param_cookie_t cookie;
	xcb_dri2_get_param_reply_t *reply;

	union {
		uint64_t u64;

		struct {
			uint32_t lo;
			uint32_t hi;
		} u32;
	} v;

	cookie = xcb_dri2_get_param(conn, drawable, param);
	reply = xcb_dri2_get_param_reply(conn, cookie, error);

	if (!reply)
		return false;

	v.u32.hi = reply->value_hi;
	v.u32.lo = reply->value_lo;

	if (is_param_recognized)
		*is_param_recognized = reply->is_param_recognized;
	if (value)
		*value = v.u64;

	return true;
}
