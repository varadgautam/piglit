/*
 * Copyright © 2009 Intel Corporation
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "piglit-util-gl-common.h"
#include "piglit-framework-gl.h"
#include "piglit-framework-gl/piglit_gl_framework.h"

struct piglit_gl_framework *gl_fw;

bool piglit_use_fbo = false;
int piglit_automatic = 0;
unsigned piglit_winsys_fbo = 0;

int piglit_width;
int piglit_height;

static void
process_args(int *argc, char *argv[], unsigned *force_samples,
	     struct piglit_gl_test_config *config);

int
piglit_gl_ctx_flavor_get_name(char name[], size_t size,
			      const struct piglit_gl_ctx_flavor *flavor)
{
	const char *api = "(Invalid API) ";
	const char *fwd_compat = "";
	const char *debug = "";

	switch (flavor->api) {
	case PIGLIT_GL_API_CORE:
		api = "Core ";
		break;
	case PIGLIT_GL_API_COMPAT:
		api = "Compatibility ";
		break;
	case PIGLIT_GL_API_ES1:
	case PIGLIT_GL_API_ES2:
		api = "ES ";
		break;
	}

	if (flavor->fwd_compat) {
		fwd_compat = "Forward-Compatible ";
	}
	if (flavor->debug) {
		debug = "Debug ";
	}

	return snprintf(name, size, "OpenGL %s%d.%d %s%sContext",
		        api, flavor->version / 10, flavor->version % 10,
		        fwd_compat, debug);
}
static bool
flavor_version_in_range(const char *api_name, int version, int min, int max)
{
	if (version >= min && version <= max) {
		return true;
	}

	piglit_logd("context flavor has invalid version (%d.%d) for the OpenGL "
		    "%s API; version must be in range [%d.%d, %d.%d]",
		    version / 10, version % 10, api_name,
		    min / 10, min % 10,
		    max / 10, max % 10);
	return false;
}

/**
 * Is the context flavor forward-compatible?
 */
static bool
flavor_is_not_fwd_compat(const char *api_name, bool fwd_compat)
{
	if (!fwd_compat) {
		return true;
	}

	piglit_logd("context attribute \"Forward-Compatible\" is illegal for "
		    "the OpenGL %s API", api_name);
	return false;
}

bool
piglit_gl_ctx_flavor_is_valid(const struct piglit_gl_ctx_flavor *flavor)
{
	bool ok = true;

	switch (flavor->api) {
	case PIGLIT_GL_API_CORE:
		ok &= flavor_version_in_range("Core", flavor->version, 31, 43);
		break;
	case PIGLIT_GL_API_COMPAT:
		ok &= flavor_version_in_range("Compatibility",
					      flavor->version, 10, 43);
		ok &= flavor->version >= 30
		      || flavor_is_not_fwd_compat("Compatibility",
					          flavor->fwd_compat);
		break;
	case PIGLIT_GL_API_ES1:
		ok &= flavor_version_in_range("ES1", flavor->version, 10, 11);
		ok &= flavor_is_not_fwd_compat("ES1", flavor->fwd_compat);
		break;
	case PIGLIT_GL_API_ES2:
		ok &= flavor_version_in_range("ES2", flavor->version, 20, 31);
		ok &= flavor_is_not_fwd_compat("ES2", flavor->fwd_compat);
		break;
	default:
		piglit_loge("context flavor has invalid api (0x%x)",
			    flavor->api);
		ok = false;
		break;
	}

	return ok;
}

static void
append_flavor(struct pgl_list *list,
	      enum piglit_gl_api api, int version, int attrib_mask)
{
	struct piglit_gl_ctx_flavor *flavor;
	static const int valid_attribs = PIGLIT_GL_CTX_DEBUG
				       | PIGLIT_GL_CTX_FORWARD_COMPATIBLE;

	if ((attrib_mask | valid_attribs) == valid_attribs) {
		piglit_loge("invalid attribute mask (0x0%x) for context "
			    "flavor; allowed bits are 0x%x",
			    attrib_mask, valid_attribs);
		piglit_report_result(PIGLIT_FAIL);
	}

	flavor = malloc(sizeof(*flavor));
	flavor->api = api;
	flavor->version = version;
	flavor->debug = attrib_mask & PIGLIT_GL_CTX_DEBUG;
	flavor->fwd_compat = attrib_mask & PIGLIT_GL_CTX_FORWARD_COMPATIBLE;

	if (!piglit_gl_ctx_flavor_is_valid(flavor)) {
		char name[1024];
		piglit_gl_ctx_flavor_get_name(name, sizeof(name), flavor);
		piglit_loge("invalid context flavor: %s", name);
		piglit_report_result(PIGLIT_FAIL);
	}

	pgl_list_init_link(&flavor->link);
	pgl_list_append(list, &flavor->link);
}

static void
extract_flavors(struct pgl_list *list,
		const struct piglit_gl_test_config *config)
{
	int attribs = 0;

	assert(pgl_list_is_empty(list));

	if (config->require_debug_context) {
		attribs |= PIGLIT_GL_CTX_DEBUG;
	}

	if (config->require_forward_compatible_context) {
		attribs |= PIGLIT_GL_CTX_FORWARD_COMPATIBLE;
	}

	if (config->supports_gl_core_version > 0) {
		append_flavor(list, PIGLIT_GL_API_CORE,
			      config->supports_gl_core_version, attribs);
	}

	if (config->supports_gl_compat_version > 0) {
		append_flavor(list, PIGLIT_GL_API_COMPAT,
			      config->supports_gl_compat_version, attribs);
	}

	if (config->supports_gl_es_version >= 20) {
		append_flavor(list, PIGLIT_GL_API_ES2,
			      config->supports_gl_es_version, attribs);
	} else if (config->supports_gl_es_version > 0) {
		append_flavor(list, PIGLIT_GL_API_ES1,
			      config->supports_gl_es_version, attribs);
	}
}

void
piglit_gl_test_config_init(struct piglit_gl_test_config *config)
{
	memset(config, 0, sizeof(*config));
}

static void
delete_arg(char *argv[], int argc, int arg)
{
	int i;

	for (i = arg + 1; i < argc; i++) {
		argv[i - 1] = argv[i];
	}
}

/**
 * Recognized arguments are removed from @a argv. The updated array
 * length is returned in @a argc.
 */
static void
process_args(int *argc, char *argv[], unsigned *force_samples,
	     struct piglit_gl_test_config *config)
{
	int j;

	piglit_parse_subtest_args(argc, argv, config->subtests,
				  &config->selected_subtests,
				  &config->num_selected_subtests);

	/* Find/remove "-auto" and "-fbo" from the argument vector.
	 */
	for (j = 1; j < *argc; j++) {
		if (!strcmp(argv[j], "-auto")) {
			piglit_automatic = 1;
			delete_arg(argv, *argc, j--);
			*argc -= 1;
		} else if (!strcmp(argv[j], "-fbo")) {
			piglit_use_fbo = true;
			delete_arg(argv, *argc, j--);
			*argc -= 1;
		} else if (!strcmp(argv[j], "-rlimit")) {
			char *ptr;
			unsigned long lim;
			int i;

			j++;
			if (j >= *argc) {
				fprintf(stderr,
					"-rlimit requires an argument\n");
				piglit_report_result(PIGLIT_FAIL);
			}

			lim = strtoul(argv[j], &ptr, 0);
			if (ptr == argv[j]) {
				fprintf(stderr,
					"-rlimit requires an argument\n");
				piglit_report_result(PIGLIT_FAIL);
			}

			piglit_set_rlimit(lim);

			/* Remove 2 arguments (hence the 'i - 2') from the
			 * command line.
			 */
			for (i = j + 1; i < *argc; i++) {
				argv[i - 2] = argv[i];
			}
			*argc -= 2;
			j -= 2;
		} else if (!strncmp(argv[j], "-samples=", 9)) {
			*force_samples = atoi(argv[j]+9);
			delete_arg(argv, *argc, j--);
			*argc -= 1;
		}
	}
}

void
piglit_gl_process_args(int *argc, char *argv[],
		       struct piglit_gl_test_config *config)
{
	unsigned force_samples = 0;

	process_args(argc, argv, &force_samples, config);

	if (force_samples > 1)
		config->window_samples = force_samples;

}

void
piglit_gl_test_run(int argc, char *argv[],
		   const struct piglit_gl_test_config *config)
{
	struct pgl_list flavors;

	piglit_width = config->window_width;
	piglit_height = config->window_height;

	pgl_list_init(&flavors);
	extract_flavors(&flavors, config);

	if (pgl_list_is_empty(&flavors)) {
		piglit_loge("test declares support for no context flavor");
		piglit_report_result(PIGLIT_FAIL);
	}

	gl_fw = piglit_gl_framework_create(config);
	if (gl_fw == NULL) {
		printf("piglit: error: failed to create "
		       "piglit_gl_framework\n");
		piglit_report_result(PIGLIT_FAIL);
	}

	gl_fw->run_test(gl_fw, argc, argv);
	assert(false);
}

void
piglit_post_redisplay(void)
{
	if (gl_fw->post_redisplay)
		gl_fw->post_redisplay(gl_fw);
}

void
piglit_set_keyboard_func(void (*func)(unsigned char key, int x, int y))
{
	if (gl_fw->set_keyboard_func)
		gl_fw->set_keyboard_func(gl_fw, func);
}

void
piglit_swap_buffers(void)
{
	if (gl_fw->swap_buffers)
		gl_fw->swap_buffers(gl_fw);
}

void
piglit_present_results(void)
{
	if (!piglit_automatic)
		piglit_swap_buffers();
}

void
piglit_set_reshape_func(void (*func)(int w, int h))
{
	if (!gl_fw->set_reshape_func)
		gl_fw->set_reshape_func(gl_fw, func);
}


enum piglit_result
piglit_create_dma_buf(unsigned w, unsigned h, unsigned cpp,
		      const void *src_data, unsigned src_stride,
		      struct piglit_dma_buf **buf, int *fd,
		      unsigned *stride, unsigned *offset)
{
	*fd = 0;
	*stride = 0;
	*offset = 0;

	if (!gl_fw->create_dma_buf)
		return PIGLIT_SKIP;

	return gl_fw->create_dma_buf(w, h, cpp, src_data, src_stride, buf, fd,
				stride, offset);
}

void
piglit_destroy_dma_buf(struct piglit_dma_buf *buf)
{
	if (gl_fw->destroy_dma_buf)
		gl_fw->destroy_dma_buf(buf);
}

size_t
piglit_get_selected_tests(const char ***selected_subtests)
{
	*selected_subtests = gl_fw->test_config->selected_subtests;
	return gl_fw->test_config->num_selected_subtests;
}
