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

#if !defined(PIGLIT_HAS_GBM) || !defined(PIGLIT_HAS_UDEV)

void
pgl_gbm_setup(enum piglit_result *out_result,
	      struct pgl_egl_resources **out_egl)
{
	*out_result = PIGLIT_SKIP;
	*out_egl = NULL;
}

void
pgl_gbm_teardown(struct pgl_egl_resources *egl,
		 enum piglit_result *out_result)
{
	*out_result = PIGLIT_SKIP;
}

#else /* !defined(PIGLIT_HAS_GBM) || !defined(PIGLIT_HAS_UDEV) */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libudev.h>
#include <gbm.h>

struct pgl_gbm_resources {
	struct gbm_device *dev;
	enum gbm_bo_format format;
	struct gbm_surface *surface;
};

static int
get_default_fd_for_pattern(const char *pattern)
{
    struct udev *ud;
    struct udev_enumerate *en;
    struct udev_list_entry *entry;
    const char *path, *filename;
    struct udev_device *device;
    int fd;

    ud = udev_new();
    en = udev_enumerate_new(ud);
    udev_enumerate_add_match_subsystem(en, "drm");
    udev_enumerate_add_match_sysname(en, pattern);
    udev_enumerate_scan_devices(en);

    udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(en)) {
        path = udev_list_entry_get_name(entry);
        device = udev_device_new_from_syspath(ud, path);
        filename = udev_device_get_devnode(device);
        fd = open(filename, O_RDWR | O_CLOEXEC);
        udev_device_unref(device);
        if (fd >= 0) {
            return fd;
        }
    }

    return -1;
}

static int
get_fd(void)
{
	int fd = -1;

	/* Prefer rendernodes to dri card. */
	fd = get_default_fd_for_pattern("renderD[0-9]*");
	if (fd >= 0) {
		return fd;
	}

	fd = get_default_fd_for_pattern("card[0-9]*");
	return fd;
}

void
pgl_gbm_setup(enum piglit_result *out_result,
	      struct pgl_egl_resources **out_egl)
{
	struct pgl_egl_resources *egl = NULL;
	struct pgl_gbm_resources *gbm = NULL;

	EGLDisplay egl_dpy_again = NULL;

	int fd = -1;
	bool ok = true;

	EGLint egl_major = 0, egl_minor = 0, num_configs = 0;

	/* Initialize outputs */
	*out_egl = NULL;
	*out_result = PIGLIT_FAIL;

	egl = calloc(1, sizeof(*egl));
	if (!egl) {
		error("out of memory");
		goto fail;
	}

	gbm = calloc(1, sizeof(*gbm));
	if (!gbm) {
		error("out of memory");
		goto fail;
	}

	egl->platform_private = gbm;

	fd = get_fd();
	if (fd < 0) {
		error("failed to open gbm fd");
		goto skip;
	}

	gbm->dev = gbm_create_device(fd);
	if (!gbm->dev) {
		error("gbm_create_device failed");
		goto skip;
	}

	egl->dpy = eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_MESA,
					    gbm->dev, NULL);
	if (!egl->dpy) {
		error("eglGetPlatformDisplay failed for GBM");
		goto fail;
	}

	/* From the EGL_EXT_platform_base spec, version 9:
	 *
	 *   Multiple calls made to eglGetPlatformDisplayEXT with the same
	 *   <platform> and <native_display> will return the same EGLDisplay
	 *   handle.
	 */
	egl_dpy_again = eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_MESA,
					         gbm->dev, NULL);
	if (egl->dpy != egl_dpy_again) {
		error("eglGetPlatformDisplayEXT returned different EGLDisplay "
		      "handles for same gbm_device");
		goto fail;
	}
	
	ok = eglInitialize(egl->dpy, &egl_major, &egl_minor);
	if (!ok) {
		error("eglInitialize failed for GBM");
		goto fail;
	}

	ok = eglChooseConfig(egl->dpy, pgl_egl_config_attrs,
			     &egl->config, 1, &num_configs);
	if (!ok || num_configs == 0 || !egl->config) {
		error("eglChooseConfig failed for GBM");
		goto fail;
	}

	ok = eglGetConfigAttrib(egl->dpy, egl->config, EGL_NATIVE_VISUAL_ID,
			        (EGLint*) &gbm->format);
	if (!ok) {
		error("eglGetConfigAttrib failed for X11");
		goto fail;
	}

	gbm->surface = gbm_surface_create(gbm->dev, window_width, window_height,
				          gbm->format, GBM_BO_USE_RENDERING);
	if (!gbm->surface) {
		error("gbm_surface_create failed");
		goto fail;
	}

	egl->window = eglCreatePlatformWindowSurfaceEXT(egl->dpy, egl->config,
						        gbm->surface, NULL);
	if (!egl->window) {
		error("eglCreatePlatformWindowSurface failed for GBM");
		goto fail;
	}

	ok = eglCreatePlatformPixmapSurfaceEXT(egl->dpy, egl->config, NULL, NULL);
	if (ok) {
		error("eglCreatePlatformPixmapSurfaceEXT succeeded for"
		      "GBM, but should have failed");
		goto fail;
	}
	if (!piglit_check_egl_error(EGL_BAD_PARAMETER)) {
		error("eglCreatePlatformPixmapSurfaceEXT should emit "
		      "EGL_BAD_PARAMETER on GBM");
		goto fail;
	}

	*out_result = PIGLIT_PASS;
	*out_egl = egl;
	return;

skip:
	pgl_gbm_teardown(egl, out_result);
	*out_result = PIGLIT_SKIP;
	return;

fail:
	pgl_gbm_teardown(egl, out_result);
	*out_result = PIGLIT_FAIL;
	return;
}


void
pgl_gbm_teardown(struct pgl_egl_resources *egl,
		 enum piglit_result *out_result)
{
	struct pgl_gbm_resources *gbm = NULL;
	bool ok = true;

	*out_result = PIGLIT_PASS;

	if (egl) {
		gbm = egl->platform_private;
	}

	if (egl && egl->window) {
		ok = eglDestroySurface(egl->dpy, egl->window);
		if (!ok) {
			error("eglDestroySurface failed for GBM window");
			*out_result = PIGLIT_FAIL;
		}
	}

	if (gbm && gbm->surface) {
		gbm_surface_destroy(gbm->surface);
	}

	if (egl && egl->dpy) {
		ok = eglTerminate(egl->dpy);
		if (!ok) {
			error("eglTerminate failed for GBM");
			*out_result = PIGLIT_FAIL;
		}
	}

	if (gbm && gbm->dev) {
		gbm_device_destroy(gbm->dev);
	}

	free(gbm);
	free(egl);
}
	
#endif /* !defined(PIGLIT_HAS_GBM) || !defined(PIGLIT_HAS_UDEV) */
