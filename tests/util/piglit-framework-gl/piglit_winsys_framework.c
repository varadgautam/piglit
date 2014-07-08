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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <assert.h>
#include <unistd.h>

#include "piglit-util-gl-common.h"
#include "piglit-util-waffle.h"

#include "piglit_gbm_framework.h"
#include "piglit_gl_framework.h"
#include "piglit_winsys_framework.h"
#include "piglit_wl_framework.h"
#include "piglit_x11_framework.h"

struct piglit_winsys_framework*
piglit_winsys_framework(struct piglit_gl_framework *gl_fw)
{
	return (struct piglit_winsys_framework*) gl_fw;
}

static void
swap_buffers(struct piglit_gl_framework *gl_fw)
{
	waffle_window_swap_buffers(piglit_wfl_framework(gl_fw)->window);
}

static void
run_test(struct piglit_gl_framework *gl_fw,
         int argc, char *argv[])
{
	struct piglit_winsys_framework *winsys_fw = piglit_winsys_framework(gl_fw);
	bool force_window = false;
	const char *env_force_window = getenv("PIGLIT_FORCE_WINDOW");


	if (env_force_window != NULL) {
		if (strcmp(env_force_window, "0") == 0) {
			force_window = false;
		} else if (strcmp(env_force_window, "1") == 0) {
			force_window = true;
		} else {
			fprintf(stderr, "PIGLIT_FORCE_WINDOW has invalid"
				" value: %s\n", env_force_window);
			abort();
		}
	}

	if (gl_fw->test_config->init)
		gl_fw->test_config->init(argc, argv);

	if (!gl_fw->test_config->requires_displayed_window &&
	    piglit_automatic && !force_window) {
		enum piglit_result result = PIGLIT_PASS;
		if (gl_fw->test_config->display)
			result = gl_fw->test_config->display();

		piglit_report_result(result);
	}

	/* In non-auto mode, the user wishes to see the window regardless
	 * of the value of piglit_gl_test_config::require_displayed_window.
	 */
	winsys_fw->show_window(winsys_fw);
	winsys_fw->enter_event_loop(winsys_fw);
	abort();
}

static void
set_keyboard_func(struct piglit_gl_framework *gl_fw,
                  void (*func)(unsigned char key, int x, int y))
{
	piglit_winsys_framework(gl_fw)->user_keyboard_func = func;
}

static void
post_redisplay(struct piglit_gl_framework *gl_fw)
{
	piglit_winsys_framework(gl_fw)->need_redisplay = true;
}

struct piglit_gl_framework*
piglit_winsys_framework_create(const struct piglit_gl_ctx_flavor *flavor,
			       const struct piglit_gl_test_config *test_config)
{
	int32_t platform = piglit_wfl_framework_choose_platform(flavor);

	switch (platform) {
#ifdef PIGLIT_HAS_X11
	case WAFFLE_PLATFORM_GLX:
	case WAFFLE_PLATFORM_X11_EGL:
		return piglit_x11_framework_create(flavor, test_config);
#endif

#ifdef PIGLIT_HAS_GBM
	case WAFFLE_PLATFORM_GBM:
		return piglit_gbm_framework_create(flavor, test_config);
#endif

/* There is no need to #ifdef out Piglit support for Wayland yet
 * because Piglit calls no Wayland functions.
 */
	case WAFFLE_PLATFORM_WAYLAND:
		return piglit_wl_framework_create(flavor, test_config);
	default:
		assert(0);
		return NULL;
	}
}

bool
piglit_winsys_framework_init(struct piglit_winsys_framework *winsys_fw,
                             const struct piglit_gl_ctx_flavor *flavor,
                             const struct piglit_gl_test_config *test_config)
{
	struct piglit_wfl_framework *wfl_fw = &winsys_fw->wfl_fw;
	struct piglit_gl_framework *gl_fw = &wfl_fw->gl_fw;
	bool ok = true;

	ok = piglit_wfl_framework_init(wfl_fw, flavor, test_config,
	                               true /*use_window_attribs*/);
	if (!ok)
		goto fail;

	winsys_fw->user_keyboard_func = piglit_escape_exit_key;

	wfl_fw->gl_fw.post_redisplay = post_redisplay;
	wfl_fw->gl_fw.set_keyboard_func = set_keyboard_func;

	gl_fw->run_test = run_test;
	gl_fw->swap_buffers = swap_buffers;

	return true;

fail:
	piglit_winsys_framework_teardown(winsys_fw);
	return false;
}

void
piglit_winsys_framework_teardown(struct piglit_winsys_framework *winsys_fw)
{
	piglit_wfl_framework_teardown(&winsys_fw->wfl_fw);
}
