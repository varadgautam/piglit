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
 * @file
 * @brief Utilities for testing the DRI2 protocol.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <xcb/xcb.h>

#include "piglit-util.h"
#include "piglit-glx-util.h"

/**
 * Connect to the X server, create a GLX window, and call glXMakeCurrent.
 *
 * \param[out] conn
 * \param[out] window
 */
void
dri2_setup_window(xcb_connection_t **conn, xcb_window_t *window);

/**
 * \see dri2_check_error()
 */
void
dri2_check_error_(uint32_t expect_error_code,
		  xcb_generic_error_t *actual_error,
		  const char *file, unsigned line);

/**
 * \brief Check X errors.
 *
 * If \a expect_error_code differs from \c actual_error->error_code,
 * then print a diagnostic and fail the test. If \a actual_error is
 * null, then the check behaves as if \c actual_error->error_code is 0.
 *
 * \see dri2_check_error_()
 */
#define dri2_check_error(expect_error_code, actual_error) \
	dri2_check_error_(expect_error_code, actual_error, __FILE__, __LINE__)

/**
 * \brief Skip test if X server if DRI2 extension is not present.
 *
 * This is no need to call this if you call dri2_require_version().
 */
void
dri2_require_dri2(xcb_connection_t *conn);

/**
 * \brief Skip test if DRI2 version is less than required.
 */
void
dri2_require_version(xcb_connection_t *conn, int major, int minor);

/**
 * \param[out] is_param_recognized is ignored if null.
 * \param[out] value is ignored if null.
 */
bool
dri2_get_param(xcb_connection_t *conn,
	       xcb_drawable_t drawable,
	       uint32_t param,
	       bool *is_param_recognized,
	       uint64_t *value,
	       xcb_generic_error_t **error);
