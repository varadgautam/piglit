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

#ifndef PIGLIT_HAS_X11

void
pgl_x11_setup(enum piglit_result *out_result,
	      struct pgl_egl_resources **out_egl)
{
	*out_result = PIGLIT_SKIP;
	*out_egl = NULL;
}

void
pgl_x11_teardown(struct pgl_egl_resources *egl,
		 enum piglit_result *out_result)
{
	*out_result = PIGLIT_SKIP;
}

#else /* PIGLIT_HAS_X11 */

#include <X11/Xlib.h>

struct pgl_x11_resources {
	Display *dpy;
	XVisualInfo *vi;
	Window window;
	Pixmap pixmap;
};

void
pgl_x11_setup(enum piglit_result *out_result,
	      struct pgl_egl_resources **out_egl)
{
	struct pgl_egl_resources *egl = NULL;
	struct pgl_x11_resources *x11 = NULL;
	bool ok = true;

	int num_visuals = 0;
	Window x11_root = 0;
        XVisualInfo x11_vi_template = {0};

	EGLDisplay egl_dpy_again = NULL;
	EGLint egl_major = 0, egl_minor = 0, num_configs = 0;

	/* Initialize outputs. */
	*out_egl = NULL;
	*out_result = PIGLIT_FAIL;

	egl = calloc(1, sizeof(*egl));
	if (!egl) {
		error("out of memory");
		goto fail;
	}

	x11 = calloc(1, sizeof(*x11));
	if (!x11) {
		error("out of memory");
		goto fail;
	}

	egl->platform_private = x11;
	
	x11->dpy = XOpenDisplay(NULL);
	if (!x11->dpy) {
		error("XOpenDisplay failed");
		goto skip;
	}

	egl->dpy = eglGetPlatformDisplayEXT(EGL_PLATFORM_X11_EXT,
					    x11->dpy, NULL);
	if (!egl->dpy) {
		error("eglGetPlatformDisplay failed for X11");
		goto fail;
	}

	/* From the EGL_EXT_platform_base spec, version 9:
	 *
	 *   Multiple calls made to eglGetPlatformDisplayEXT with the same
	 *   <platform> and <native_display> will return the same EGLDisplay
	 *   handle.
	 */
	egl_dpy_again = eglGetPlatformDisplayEXT(EGL_PLATFORM_X11_EXT,
					         x11->dpy, NULL);
	if (egl->dpy != egl_dpy_again) {
		error("eglGetPlatformDisplayEXT returned different EGLDisplay "
		      "handles for same X11 Display");
		goto fail;
	}

	ok = eglInitialize(egl->dpy, &egl_major, &egl_minor);
	if (!ok) {
		error("eglInitialize failed for X11");
		goto fail;
	}

	ok = eglChooseConfig(egl->dpy, pgl_egl_config_attrs,
			     &egl->config, 1, &num_configs);
	if (!ok || num_configs == 0 || !egl->config) {
		error("eglChooseConfig failed for X11");
		goto fail;
	}

	memset(&x11_vi_template, 0, sizeof(x11_vi_template));
	ok = eglGetConfigAttrib(egl->dpy, egl->config, EGL_NATIVE_VISUAL_ID,
			        (EGLint*) &x11_vi_template.visualid);
	if (!ok) {
		error("eglGetConfigAttrib(EGL_NATIVE_VISUAL_ID) failed "
		      "for X11");
		goto fail;
	}

	x11->vi = XGetVisualInfo(x11->dpy, VisualIDMask, &x11_vi_template,
			         &num_visuals);
	if (!x11->vi || num_visuals == 0) {
		error("XGetVisualInfo failed");
		goto fail;
	}


	x11_root = RootWindow(x11->dpy, DefaultScreen(x11->dpy));
	if (!x11_root) {
		error("RootWindow() failed");
		goto fail;
	}

        x11->window = XCreateWindow(x11->dpy, x11_root,
				    0, 0, /*x, y*/
				    window_width, window_height,
				    0, /*border_width*/
				    x11->vi->depth,
				    InputOutput, /*class*/
				    x11->vi->visual,
				    0, /*attribute mask*/
				    NULL); /*attributes*/
	if (!x11->window) {
		error("XCreateWindow failed");
		goto fail;
	}

	egl->window = eglCreatePlatformWindowSurfaceEXT(egl->dpy, egl->config,
						        &x11->window, NULL);
	if (!egl->window) {
		error("eglCreatePlatformWindowSurfaceEXT failed for X11");
		goto fail;
	}

	x11->pixmap = XCreatePixmap(x11->dpy, x11_root,
				    window_width, window_height,
				    x11->vi->depth);
	if (!x11->pixmap) {
		error("XCreatePixmap failed");
		goto fail;
	}

	egl->pixmap = eglCreatePlatformPixmapSurfaceEXT(egl->dpy, egl->config,
							&x11->pixmap, NULL);
	if (!egl->pixmap) {
		error("eglCreatePlatformPixmapSurfaceEXT failed for X11");
		goto fail;
	}

	*out_result = PIGLIT_PASS;
	*out_egl = egl;
	return;

skip:
	pgl_x11_teardown(egl, out_result);
	*out_result = PIGLIT_SKIP;
	return;

fail:
	pgl_x11_teardown(egl, out_result);
	*out_result = PIGLIT_FAIL;
	return;
}

void
pgl_x11_teardown(struct pgl_egl_resources *egl,
		 enum piglit_result *out_result)
{
	struct pgl_x11_resources *x11 = NULL;
	bool ok = true;

	*out_result = PIGLIT_PASS;

	if (egl) {
		x11 = egl->platform_private;
	}

	if (egl && egl->window) {
		ok = eglDestroySurface(egl->dpy, egl->window);
		if (!ok) {
			error("eglDestroySurface failed for X11 window");
			*out_result = PIGLIT_FAIL;
		}
	}

	if (x11 && x11->window) {
		XDestroyWindow(x11->dpy, x11->window);
	}

	if (egl && egl->pixmap) {
		ok = eglDestroySurface(egl->dpy, egl->pixmap);
		if (!ok) {
			error("eglDestroySurface failed for X11 pixmap");
			*out_result = PIGLIT_FAIL;
		}
	}

	if (x11 && x11->pixmap) {
		XFreePixmap(x11->dpy, x11->pixmap);
	}

	if (x11 && x11->vi) {
		XFree(x11->vi);
	}

	if (egl && egl->dpy) {
		ok = eglTerminate(egl->dpy);
		if (!ok) {
			error("eglTerminate failed for X11");
			*out_result = PIGLIT_FAIL;
		}
	}

	if (x11 && x11->dpy) {
		XCloseDisplay(x11->dpy);
	}

	free(x11);
	free(egl);
}
#endif /* PIGLIT_HAS_X11 */
