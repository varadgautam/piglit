/* Copyright 2012 Intel Corporation
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

#include "piglit-util.h"

#if defined(_WIN32)

#elif defined(__APPLE__)

#include <stdlib.h>
#include <string.h>
#include <AvailabilityMacros.h>
#include <dlfcn.h>

#else /* Linux */

#include <GL/glx.h>

#endif

/**
 * Generated code calls this function if the test tries to use a GL
 * function that is not supported on the current implementation.
 *
 * This function terminates the test with a SKIP; this saves the
 * piglit test from the burden of having to pre-check whether the
 * implementation supports the functionality being tested.
 */
static void
default_unsupported(const char *name)
{
	printf("Function \"%s\" not supported on this implementation\n", name);
	piglit_report_result(PIGLIT_SKIP);
}

/**
 * Generated code calls this function if a call to GetProcAddress()
 * returns NULL.
 *
 * We don't expect this to ever happen, since we only call
 * GetProcAddress() for functions that the implementation claims to
 * support.  So if it does happen we terminate the test with a FAIL.
 */
static void
default_get_proc_address_failure(const char *function_name)
{
	printf("GetProcAddress failed for \"%s\"\n", function_name);
	piglit_report_result(PIGLIT_FAIL);
}

#if defined(_WIN32)

/**
 * This function is used to retrieve the address of GL extension
 * functions, and core GL functions for GL versions above 1.3, on
 * Windows.
 */
static piglit_dispatch_function_ptr
get_ext_proc_address(const char *function_name)
{
	return (piglit_dispatch_function_ptr) wglGetProcAddress(function_name);
}

/**
 * This function is used to retrieve the address of core GL functions
 * on windows.
 */
static piglit_dispatch_function_ptr
get_core_proc_address(const char *function_name, int gl_10x_version)
{
	if (gl_10x_version > 11) {
		return get_ext_proc_address(function_name);
	} else {
		return (piglit_dispatch_function_ptr)
			GetProcAddress(LoadLibraryA("OPENGL32"), function_name);
	}
}

#elif defined(__APPLE__)

/**
 * This function is used to retrieve the address of all GL functions
 * on Apple.
 */
static piglit_dispatch_function_ptr
get_ext_proc_address(const char *function_name)
{
	static const char *opengl_path =
		"/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL";
	static void *opengl_lib = NULL;

	/* Dynamically load lib */
	if (!opengl_lib)
	{
		opengl_lib = dlopen(opengl_path, RTLD_LAZY);
		if (!opengl_lib)
			return NULL;
	}

	return (piglit_dispatch_function_ptr) dlsym(opengl_lib, function_name);
}

/**
 * This function is used to retrieve the address of core GL functions
 * on Apple.
 */
static piglit_dispatch_function_ptr
get_core_proc_address(const char *function_name, int gl_10x_version)
{
	/* We don't need to worry about the GL version, since on Apple
	 * we retrieve all proc addresses in the same way.
	 */
	(void) gl_10x_version;

	return get_ext_proc_address(function_name);
}

#else /* Linux */

/**
 * This function is used to retrieve the address of all GL functions
 * on Linux.
 */
static piglit_dispatch_function_ptr
get_ext_proc_address(const char *function_name)
{
	return glXGetProcAddressARB((const GLubyte *) function_name);
}

/**
 * This function is used to retrieve the address of core GL functions
 * on Linux.
 */
static piglit_dispatch_function_ptr
get_core_proc_address(const char *function_name, int gl_10x_version)
{
	/* We don't need to worry about the GL version, since on Apple
	 * we retrieve all proc addresses in the same way.
	 */
	(void) gl_10x_version;

	return get_ext_proc_address(function_name);
}

#endif

/**
 * Initialize the GL dispatch mechanism to a default configuration.
 *
 * Eventually we will want to replace this with code that initializes
 * the GL dispatch mechanism based on run-time parameters (e.g. to
 * select X vs Wayland, or desktop GL vs GLES).
 *
 * This function is safe to call multiple times--it only has an effect
 * on the first call.
 */
void
piglit_dispatch_default_init()
{
	static bool already_initialized = false;

	if (already_initialized)
		return;

	piglit_dispatch_init(PIGLIT_DISPATCH_GL,
			     get_core_proc_address,
			     get_ext_proc_address,
			     default_unsupported,
			     default_get_proc_address_failure);

	already_initialized = true;
}
