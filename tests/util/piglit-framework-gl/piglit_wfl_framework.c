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

#include <stdio.h>

#include "piglit-util-gl-common.h"
#include "piglit-util-waffle.h"

#include "piglit_wfl_framework.h"

static bool
setup_gl(const struct piglit_gl_ctx_flavor *flavor,
	 const struct piglit_gl_test_config *test_config,
	 bool use_window_attribs,
	 struct waffle_display *display,
	 struct waffle_config **out_config,
	 struct waffle_context **out_context,
	 struct waffle_window **out_window);

struct piglit_wfl_framework*
piglit_wfl_framework(struct piglit_gl_framework *gl_fw)
{
	return (struct piglit_wfl_framework*) gl_fw;
}

int32_t
piglit_wfl_framework_choose_platform(const struct piglit_gl_ctx_flavor *flavor)
{
	const char *env = getenv("PIGLIT_PLATFORM");

	if (env == NULL) {
#if defined(PIGLIT_HAS_X11) && defined(PIGLIT_HAS_EGL)
		if (flavor->api == PIGLIT_GL_API_ES1 ||
		    flavor->api == PIGLIT_GL_API_ES2) {
			/* Some GLX implementations don't support creation of
			 * ES1 and ES2 contexts, so use XEGL instead.
			 */
			return WAFFLE_PLATFORM_X11_EGL;
		}
#endif
#ifdef PIGLIT_HAS_GLX
		return WAFFLE_PLATFORM_GLX;
#else
		fprintf(stderr, "environment var PIGLIT_PLATFORM must be set "
		        "when piglit is built without GLX support\n");
		piglit_report_result(PIGLIT_FAIL);
#endif
	}

	else if (strcmp(env, "gbm") == 0) {
#ifdef PIGLIT_HAS_GBM
		return WAFFLE_PLATFORM_GBM;
#else
		fprintf(stderr, "environment var PIGLIT_PLATFORM=gbm, but "
		        "piglit was built without GBM support\n");
		piglit_report_result(PIGLIT_FAIL);
#endif
	}

	else if (strcmp(env, "glx") == 0) {
#ifdef PIGLIT_HAS_GLX
		return WAFFLE_PLATFORM_GLX;
#else
		fprintf(stderr, "environment var PIGLIT_PLATFORM=glx, but "
		        "piglit was built without GLX support\n");
		piglit_report_result(PIGLIT_FAIL);
#endif
	}

	else if (strcmp(env, "x11_egl") == 0) {
#if defined(PIGLIT_HAS_X11) && defined(PIGLIT_HAS_EGL)
		return WAFFLE_PLATFORM_X11_EGL;
#else
		fprintf(stderr, "environment var PIGLIT_PLATFORM=x11_egl, "
		        "but piglit was built without X11/EGL support\n");
		piglit_report_result(PIGLIT_FAIL);
#endif
	}

	else if (strcmp(env, "wayland") == 0) {
#ifdef PIGLIT_HAS_WAYLAND
		return WAFFLE_PLATFORM_WAYLAND;
#else
		fprintf(stderr, "environment var PIGLIT_PLATFORM=wayland, "
		        "but piglit was built without Wayland support\n");
		piglit_report_result(PIGLIT_FAIL);
#endif
	}

	else {
		fprintf(stderr, "environment var PIGLIT_PLATFORM has bad "
			"value \"%s\"\n", env);
		piglit_report_result(PIGLIT_FAIL);
	}

	assert(false);
	return 0;
}

/**
 * \brief Handle requests for OpenGL 3.1 profiles.
 *
 * Strictly speaking, an OpenGL 3.1 context has no profile. (See the
 * EGL_KHR_create_context spec for the ugly details [1]). If the user does
 * request a specific OpenGL 3.1 profile, though, then let's do what the user
 * wants.
 *
 * If the user requests a OpenGL 3.1 Core Context, and the returned context is
 * exactly an OpenGL 3.1 context but it exposes GL_ARB_compatibility, then
 * fallback to requesting an OpenGL 3.2 Core Context because, if context
 * creation succeeds, then Waffle guarantees that an OpenGL 3.2 Context will
 * have the requested profile. Likewise for OpenGL 3.1 Compatibility Contexts.
 *
 * [1] http://www.khronos.org/registry/egl/extensions/KHR/EGL_KHR_create_context.txt
 */
static bool
special_case_gl31(const struct piglit_gl_ctx_flavor *flavor,
		  const char *ctx_flavor_name,
		  int actual_version,
		  const struct piglit_gl_test_config *test_config,
		  bool use_window_attribs,
		  struct waffle_display *display,
		  struct waffle_config **out_config,
		  struct waffle_context **out_context,
		  struct waffle_window **out_window)
{
	const int requested_version = flavor->version;
	const bool requested_core = (flavor->api == PIGLIT_GL_API_CORE);
	struct piglit_gl_ctx_flavor fallback_flavor;

	switch (flavor->api) {
	case PIGLIT_GL_API_CORE:
	case PIGLIT_GL_API_COMPAT:
		if (requested_version != 31 || actual_version != 31) {
			return true;
		}
		break;
	case PIGLIT_GL_API_ES1:
	case PIGLIT_GL_API_ES2:
	     return true;
	}

	if (requested_core == !piglit_is_extension_supported("GL_ARB_compatibility")) {
		return true;
	}

	piglit_logd("Requested an %s, and the returned context is exactly a 3.1 "
		    "context. But it has the wrong profile because it %s the "
		    "GL_ARB_compatibility extension. Fallback to requesting a "
		    "3.2 context, which is guaranteed to have the correct "
		    "profile if context creation succeeds.",
		    ctx_flavor_name, requested_core ? "exposes" : "lacks");

	fallback_flavor = *flavor;
	fallback_flavor.version = 32;

	waffle_config_destroy(*out_config);
	waffle_context_destroy(*out_context);
	waffle_window_destroy(*out_window);

	return setup_gl(&fallback_flavor, test_config, use_window_attribs,
			display, out_config, out_context, out_window);
}

static bool
setup_gl(const struct piglit_gl_ctx_flavor *flavor,
	 const struct piglit_gl_test_config *test_config,
	 bool use_window_attribs,
	 struct waffle_display *display,
	 struct waffle_config **out_config,
	 struct waffle_context **out_context,
	 struct waffle_window **out_window)
{
	char flavor_name[1024];
	int32_t config_attribs[128];
	int i = 0, window_visual = 0, window_samples = 0, actual_version = 0;

	*out_config = NULL;
	*out_context = NULL;
	*out_window = NULL;

	piglit_gl_ctx_flavor_get_name(flavor_name, sizeof(flavor_name),
				      flavor);

	#define APPEND(key, value) \
		do { \
			config_attribs[i++] = key; \
			config_attribs[i++] = value; \
		} while (0)

	if (use_window_attribs) {
		window_visual = test_config->window_visual;
		window_samples = test_config->window_samples;
	}

	switch (flavor->api) {
	case PIGLIT_GL_API_CORE:
		APPEND(WAFFLE_CONTEXT_API, WAFFLE_CONTEXT_OPENGL);
		assert(flavor->version >= 31);
		if (flavor->version >= 32) {
			APPEND(WAFFLE_CONTEXT_PROFILE,
			       WAFFLE_CONTEXT_CORE_PROFILE);
		}
		break;
	case PIGLIT_GL_API_COMPAT:
		APPEND(WAFFLE_CONTEXT_API, WAFFLE_CONTEXT_OPENGL);
		if (flavor->version >= 32) {
			APPEND(WAFFLE_CONTEXT_PROFILE,
			       WAFFLE_CONTEXT_COMPATIBILITY_PROFILE);
		}
		break;
	case PIGLIT_GL_API_ES1:
		APPEND(WAFFLE_CONTEXT_API, WAFFLE_CONTEXT_OPENGL_ES1);
		break;
	case PIGLIT_GL_API_ES2:
		if (flavor->version >= 30) {
			APPEND(WAFFLE_CONTEXT_API, WAFFLE_CONTEXT_OPENGL_ES3);
		} else {
			APPEND(WAFFLE_CONTEXT_API, WAFFLE_CONTEXT_OPENGL_ES2);
		}
		break;
	}

	APPEND(WAFFLE_CONTEXT_MAJOR_VERSION, flavor->version / 10);
	APPEND(WAFFLE_CONTEXT_MINOR_VERSION, flavor->version % 10);

	if (flavor->fwd_compat) {
		APPEND(WAFFLE_CONTEXT_FORWARD_COMPATIBLE, true);
	}
	if (flavor->debug) {
		APPEND(WAFFLE_CONTEXT_DEBUG, true);
	}
	if (window_visual & (PIGLIT_GL_VISUAL_RGB | PIGLIT_GL_VISUAL_RGBA)) {
		APPEND(WAFFLE_RED_SIZE, 1);
		APPEND(WAFFLE_GREEN_SIZE, 1);
		APPEND(WAFFLE_BLUE_SIZE, 1);
	}
	if (window_visual & PIGLIT_GL_VISUAL_RGBA) {
		APPEND(WAFFLE_ALPHA_SIZE, 1);
	}
	if (window_visual & PIGLIT_GL_VISUAL_DEPTH) {
		APPEND(WAFFLE_DEPTH_SIZE, 1);
	}
	if (window_visual & PIGLIT_GL_VISUAL_STENCIL) {
		APPEND(WAFFLE_STENCIL_SIZE, 1);
	}
	if (!(window_visual & PIGLIT_GL_VISUAL_DOUBLE)) {
		APPEND(WAFFLE_DOUBLE_BUFFERED, false);
	}
	if (window_visual & PIGLIT_GL_VISUAL_ACCUM) {
		APPEND(WAFFLE_ACCUM_BUFFER, true);
	}
	if (window_samples > 1) {
		APPEND(WAFFLE_SAMPLE_BUFFERS, 1);
		APPEND(WAFFLE_SAMPLES, window_samples);
	}

	config_attribs[i++] = 0;
	#undef APPEND

	*out_config = waffle_config_choose(display, config_attribs);
	if (!*out_config) {
		wfl_log_error("waffle_config_choose");
		piglit_loge("failed to create waffle_config for %s",
			    flavor_name);
		goto fail;
	}

	*out_context = waffle_context_create(*out_config, NULL);
	if (!*out_context) {
		wfl_log_error("waffle_context_create");
		piglit_loge("failed to create waffle_context for %s",
			    flavor_name);
		goto fail;
	}

	*out_window = wfl_checked_window_create(*out_config,
	                                        test_config->window_width,
	                                        test_config->window_height);
	wfl_checked_make_current(display, *out_window, *out_context);

#ifdef PIGLIT_USE_OPENGL
	piglit_dispatch_default_init(PIGLIT_DISPATCH_GL);
#elif defined(PIGLIT_USE_OPENGL_ES1)
	piglit_dispatch_default_init(PIGLIT_DISPATCH_ES1);
#elif defined(PIGLIT_USE_OPENGL_ES2) || defined(PIGLIT_USE_OPENGL_ES3)
	piglit_dispatch_default_init(PIGLIT_DISPATCH_ES2);
#else
#	error
#endif

	actual_version = piglit_get_gl_version();
	if (actual_version < flavor->version) {
		piglit_loge("requested an %s, but actual context version is "
			    "%d.%d", flavor_name,
			    actual_version / 10, actual_version % 10);
		goto fail;
	}

	if (!special_case_gl31(flavor, flavor_name, actual_version,
			       test_config, use_window_attribs, display,
			       out_config, out_context, out_window)) {
		goto fail;
	}

	piglit_is_core_profile = (flavor->api == PIGLIT_GL_API_CORE);
	return true;

fail:
	waffle_window_destroy(*out_window);
	waffle_context_destroy(*out_context);
	waffle_config_destroy(*out_config);
	piglit_gl_reinitialize_extensions();

	return false;
}

/**
 * Choose waffle platform from context flavor, initialize waffle. Return chosen
 * platform.
 */
static enum waffle_enum
init_platform(const struct piglit_gl_ctx_flavor *flavor)
{
	static enum waffle_enum old_platform = 0;
	enum waffle_enum platform = 0;

	platform = piglit_wfl_framework_choose_platform(flavor);
	if (!platform) {
		goto done;
	}

	if (old_platform) {
		assert(platform == old_platform);
		goto done;
	}

	wfl_checked_init((int32_t[]){WAFFLE_PLATFORM, platform, 0});
	old_platform = platform;

done:
	return platform;
}


bool
piglit_wfl_framework_init(struct piglit_wfl_framework *wfl_fw,
			  const struct piglit_gl_ctx_flavor *flavor,
                          const struct piglit_gl_test_config *test_config,
			  bool use_window_attribs)
{
	bool ok = true;

	ok = piglit_gl_framework_init(&wfl_fw->gl_fw, flavor, test_config);
	if (!ok)
		goto fail;

	wfl_fw->platform = init_platform(flavor);
	if (!wfl_fw->platform)
		goto fail;

	wfl_fw->display = wfl_checked_display_connect(NULL);
	ok = setup_gl(flavor, test_config, use_window_attribs,
		      wfl_fw->display, &wfl_fw->config, &wfl_fw->context,
		      &wfl_fw->window);
	if (!ok)
		goto fail;

	return true;

fail:
	piglit_wfl_framework_teardown(wfl_fw);
	return false;
}

void
piglit_wfl_framework_teardown(struct piglit_wfl_framework *wfl_fw)
{
	waffle_window_destroy(wfl_fw->window);
	waffle_context_destroy(wfl_fw->context);
	waffle_config_destroy(wfl_fw->config);
	waffle_display_disconnect(wfl_fw->display);

	piglit_gl_framework_teardown(&wfl_fw->gl_fw);
}
