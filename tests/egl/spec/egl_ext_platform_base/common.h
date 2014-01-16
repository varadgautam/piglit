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

#pragma once

#define EGL_EXT_platform_base 1 /* Inhibit definition of prototypes. */

#include "piglit-util.h"
#include "piglit-util-egl.h"

#define EGL_PLATFORM_X11_EXT              0x31D5
#define EGL_PLATFORM_X11_SCREEN_EXT       0x31D6
#define EGL_PLATFORM_WAYLAND_EXT          0x31D8
#define EGL_PLATFORM_GBM_MESA             0x31D7

extern EGLDisplay (*eglGetPlatformDisplayEXT)(EGLenum platform, void *native_display, const EGLint *attrib_list);
extern EGLSurface (*eglCreatePlatformWindowSurfaceEXT)(EGLDisplay dpy, EGLConfig config, void *native_window, const EGLint *attrib_list);
extern EGLSurface (*eglCreatePlatformPixmapSurfaceEXT)(EGLDisplay dpy, EGLConfig config, void *native_pixmap, const EGLint *attrib_list);

static const int window_width = 64;
static const int window_height = 64;

struct pgl_egl_resources {
	EGLDisplay dpy;
	EGLConfig config;
	EGLSurface window;
	EGLSurface pixmap;
	void *platform_private;
};

static const EGLint pgl_egl_config_attrs[] = {
	EGL_BUFFER_SIZE,        32,
	EGL_RED_SIZE,            8,
	EGL_GREEN_SIZE,          8,
	EGL_BLUE_SIZE,           8,
	EGL_ALPHA_SIZE,          8,

	EGL_DEPTH_SIZE,         EGL_DONT_CARE,
	EGL_STENCIL_SIZE,       EGL_DONT_CARE,

	EGL_RENDERABLE_TYPE,    EGL_OPENGL_ES2_BIT,
	EGL_SURFACE_TYPE,       EGL_WINDOW_BIT,
	EGL_NONE,
};

void
error(const char *fmt, ...);

void
pgl_x11_setup(enum piglit_result *out_result,
	      struct pgl_egl_resources **out_egl);

void
pgl_x11_teardown(struct pgl_egl_resources *egl,
		 enum piglit_result *out_result);

void
pgl_wl_setup(enum piglit_result *out_result,
	     struct pgl_egl_resources **out_egl);

void
pgl_wl_teardown(struct pgl_egl_resources *egl,
		enum piglit_result *out_result);

void
pgl_gbm_setup(enum piglit_result *out_result,
	      struct pgl_egl_resources **out_egl);

void
pgl_gbm_teardown(struct pgl_egl_resources *egl,
		 enum piglit_result *out_result);
