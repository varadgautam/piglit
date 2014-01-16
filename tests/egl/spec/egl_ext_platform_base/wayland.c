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

#include "common.h"

#ifndef PIGLIT_HAS_WAYLAND

void
pgl_wl_setup(enum piglit_result *out_result,
	     struct pgl_egl_resources **out_egl)
{
	*out_result = PIGLIT_SKIP;
	*out_egl = NULL;
}

void
pgl_wl_teardown(struct pgl_egl_resources *egl,
	        enum piglit_result *out_result)
{
	*out_result = PIGLIT_SKIP;
}

#else /* PIGLIT_HAS_WAYLAND */

#include <wayland-client.h>
#include <wayland-egl.h>

struct pgl_wl_resources {
	struct wl_display *dpy;
	struct wl_registry *registry;
	struct wl_compositor *compositor;
	struct wl_shell *shell;
	struct wl_surface *surface;
	struct wl_shell_surface *shell_surface;
	struct wl_egl_window *window;
};

static void
registry_global_add(void *user_data, struct wl_registry *registry,
		    uint32_t name, const char *interface, uint32_t version)
{
	struct pgl_wl_resources *wl = user_data;

	if (!strncmp(interface, "wl_compositor", 14)) {
		wl->compositor = wl_registry_bind(registry, name,
					          &wl_compositor_interface, 1);
	} else if (!strncmp(interface, "wl_shell", 9)) {
		wl->shell = wl_registry_bind(registry, name,
					    &wl_shell_interface, 1);
	}
}

static void
registry_global_remove(void *user_data, struct wl_registry *registry,
		       uint32_t name)
{
	return;
}

static const struct wl_registry_listener registry_listener = {
	registry_global_add,
	registry_global_remove,
};

static void
shell_surface_ping(void *user_data,
		   struct wl_shell_surface *shell_surface,
		   uint32_t serial)
{
    wl_shell_surface_pong(shell_surface, serial);
}

static void
shell_surface_configure(void *user_data,
		        struct wl_shell_surface *shell_surface,
		        uint32_t edges, int32_t width, int32_t height)
{
	return;
}

static void
shell_surface_popup_done(void *user_data,
			 struct wl_shell_surface *shell_surface)
{
	return;
}

static const struct wl_shell_surface_listener shell_surface_listener = {
	shell_surface_ping,
	shell_surface_configure,
	shell_surface_popup_done,
};

void
pgl_wl_setup(enum piglit_result *out_result,
	     struct pgl_egl_resources **out_egl)
{
	struct pgl_egl_resources *egl = NULL;
	struct pgl_wl_resources *wl = NULL;

	EGLDisplay egl_dpy_again = NULL;
	EGLint egl_major = 0, egl_minor = 0, num_configs = 0;

	int err = 0;
	bool ok = true;

	/* Initialize outputs. */
	*out_egl = NULL;
	*out_result = PIGLIT_FAIL;

	egl = calloc(1, sizeof(*egl));
	if (!egl) {
		error("out of memory");
		goto fail;
	}

	wl = calloc(1, sizeof(*wl));
	if (!wl) {
		error("out of memory");
		goto fail;
	}

	egl->platform_private = wl;
	
	wl->dpy = wl_display_connect(NULL);
	if (!wl->dpy) {
		error("wl_display_connect failed");
		goto skip;
	}

	wl->registry = wl_display_get_registry(wl->dpy);
	if (!wl->registry) {
		error("wl_display_get_registry failed");
		goto fail;
	}

	err = wl_registry_add_listener(wl->registry,
					 &registry_listener, wl);
	if (err < 0) {
		error("wl_registry_add_listener failed");
		goto fail;
	}

	/* Block until the Wayland server has processed all pending requests and
	 * has sent out pending events on all event queues. This should ensure
	 * that the registry listener has received announcement of the shell and
	 * compositor.
	 */
	err = wl_display_roundtrip(wl->dpy);
	if (err < 0) {
		error("wl_display_roundtrip failed");
		goto fail;
	}

	if (!wl->compositor) {
		error("failed to bind to the wayland compositor\n");
		goto fail;
	}

	if (!wl->shell) {
		error("failed to bind to the wayland shell\n");
		goto fail;
	}

	egl->dpy = eglGetPlatformDisplayEXT(EGL_PLATFORM_WAYLAND_EXT,
					    wl->dpy, NULL);
	if (!egl->dpy) {
		error("eglGetPlatformDisplayEXT failed for Wayland");
		goto fail;
	}

	/* From the EGL_EXT_platform_base spec, version 9:
	 *
	 *   Multiple calls made to eglGetPlatformDisplayEXT with the same
	 *   <platform> and <native_display> will return the same EGLDisplay
	 *   handle.
	 */
	egl_dpy_again = eglGetPlatformDisplayEXT(EGL_PLATFORM_WAYLAND_EXT,
					         wl->dpy, NULL);
	if (egl->dpy != egl_dpy_again) {
		error("eglGetPlatformDisplayEXT returned different EGLDisplay "
		      "handles for same wl_display");
		goto fail;
	}

	ok = eglInitialize(egl->dpy, &egl_major, &egl_minor);
	if (!ok) {
		error("eglInitialize failed for Wayland");
		goto fail;
	}

	ok = eglChooseConfig(egl->dpy, pgl_egl_config_attrs,
			     &egl->config, 1, &num_configs);
	if (!ok || num_configs == 0 || !egl->config) {
		error("eglChooseConfig failed for Wayland");
		goto fail;
	}

	wl->surface = wl_compositor_create_surface(wl->compositor);
	if (!wl->surface) {
		error("wl_compositor_create_surface failed");
		goto fail;
	}

	wl->shell_surface = wl_shell_get_shell_surface(wl->shell,
							   wl->surface);
	if (!wl->shell_surface) {
		error("wl_shell_get_shell_surface failed");
		goto fail;
	}

	err = wl_shell_surface_add_listener(wl->shell_surface,
					      &shell_surface_listener, NULL);
	if (err < 0) {
		error("wl_shell_surface_add_listener failed");
		goto fail;
	}

	wl->window = wl_egl_window_create(wl->surface, window_width, window_height);
	if (!wl->window) {
		error("wl_egl_window_create failed");
		goto fail;
	}

	egl->window = eglCreatePlatformWindowSurfaceEXT(egl->dpy, egl->config,
						        wl->window, NULL);
	if (!egl->window) {
		error("eglCreatePlatformWindowSurfaceEXT failed for Wayland");
		goto fail;
	}

	ok = eglCreatePlatformPixmapSurfaceEXT(egl->dpy, egl->config, NULL, NULL);
	if (ok) {
		error("eglCreatePlatformPixmapSurfaceEXT succeeded for"
		      "Wayland, but should have failed");
		goto fail;
	}
	if (!piglit_check_egl_error(EGL_BAD_PARAMETER)) {
		error("eglCreatePlatformPixmapSurfaceEXT should emit "
		      "EGL_BAD_PARAMETER on Wayland");
		goto fail;
	}

	*out_result = PIGLIT_PASS;
	*out_egl = egl;
	return;

skip:
	pgl_wl_teardown(egl, out_result);
	*out_result = PIGLIT_SKIP;
	return;

fail:
	pgl_wl_teardown(egl, out_result);
	*out_result = PIGLIT_FAIL;
	return;
}

void
pgl_wl_teardown(struct pgl_egl_resources *egl,
	        enum piglit_result *out_result)
{
	struct pgl_wl_resources *wl = NULL;
	bool ok = true;

	*out_result = PIGLIT_PASS;

	if (egl) {
		wl = egl->platform_private;
	}

	if (egl && egl->window) {
		ok = eglDestroySurface(egl->dpy, egl->window);
		if (!ok) {
			error("eglDestroySurface failed for Wayland");
			*out_result = PIGLIT_FAIL;
		}
	}

	if (wl && wl->window) {
		wl_egl_window_destroy(wl->window);
	}

	if (wl && wl->shell_surface) {
		wl_shell_surface_destroy(wl->shell_surface);
	}

	if (wl && wl->surface) {
		wl_surface_destroy(wl->surface);
	}

	if (egl && egl->dpy) {
		ok = eglTerminate(egl->dpy);
		if (!ok) {
			error("eglTerminate failed for Wayland");
			*out_result = PIGLIT_FAIL;
		}
	}

	if (wl && wl->dpy) {
		wl_display_disconnect(wl->dpy);
	}

	free(wl);
	free(egl);
}
#endif /* PIGLIT_HAS_WAYLAND */
