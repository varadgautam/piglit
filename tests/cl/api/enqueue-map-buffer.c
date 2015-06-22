/*
 * Copyright 2014 Advanced Micro Devices, Inc.
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
 *
 * Authors: Tom Stellard <thomas.stellard@amd.com>
 *          Aaron Watry <awatry@gmail.com>
 *
 */

#include "piglit-framework-cl-api.h"
#include "piglit-util-cl.h"


PIGLIT_CL_API_TEST_CONFIG_BEGIN

	config.name = "clEnqueueMapBuffer";
	config.version_min = 10;

	config.run_per_platform = true;
	config.create_context = true;

PIGLIT_CL_API_TEST_CONFIG_END

enum piglit_result
piglit_cl_test(const int argc,
	const char **argv,
	const struct piglit_cl_api_test_config* config,
	const struct piglit_cl_api_test_env* env)
{
	int host_src_buffer[4] = {1, 2, 3, 4};
	int host_dst_buffer[4] = {0, 0, 0, 0};
	cl_mem device_src_buffer, device_dst_buffer;
	cl_command_queue queue = env->context->command_queues[0];
	cl_int err;
	cl_int *mapped_buffer;
	int i;

	device_src_buffer = piglit_cl_create_buffer(
		env->context, CL_MEM_READ_WRITE, sizeof(host_src_buffer));
	device_dst_buffer = piglit_cl_create_buffer(
		env->context, CL_MEM_READ_WRITE, sizeof(host_dst_buffer));

	//Map source buffer
	mapped_buffer = clEnqueueMapBuffer(queue, device_src_buffer, CL_TRUE,
		CL_MAP_WRITE, 0, sizeof(host_src_buffer), 0, NULL, NULL, &err);
	if (!piglit_cl_check_error(err, CL_SUCCESS)) {
		return PIGLIT_FAIL;
	}

	//memcpy to it
	memcpy(mapped_buffer, host_src_buffer, sizeof(host_src_buffer));

	//Unmap source buffer
	err = clEnqueueUnmapMemObject(queue, device_src_buffer, mapped_buffer,
			0, NULL, NULL);
	if (!piglit_cl_check_error(err, CL_SUCCESS)) {
		return PIGLIT_FAIL;
	}

	//enqueueCopyBuffer from device src to device dst
	err = clEnqueueCopyBuffer(queue, device_src_buffer, device_dst_buffer,
			0, 0, sizeof(host_src_buffer), 0, NULL, NULL);
	if (!piglit_cl_check_error(err, CL_SUCCESS)) {
		return PIGLIT_FAIL;
	}

	//Map dest buffer
	mapped_buffer = clEnqueueMapBuffer(queue, device_dst_buffer, CL_TRUE,
			CL_MAP_READ, 0, sizeof(host_src_buffer), 0, NULL, NULL, &err);
	if (!piglit_cl_check_error(err, CL_SUCCESS)) {
		return PIGLIT_FAIL;
	}

	//Memcpy back
	memcpy(host_dst_buffer, mapped_buffer, sizeof(host_dst_buffer));

	//Unmap
	err = clEnqueueUnmapMemObject(queue, device_dst_buffer, mapped_buffer,
			0, NULL, NULL);
	if (!piglit_cl_check_error(err, CL_SUCCESS)) {
		return PIGLIT_FAIL;
	}

	//Do comparison of host src/dest
	for (i = 0; i < sizeof(host_src_buffer) / sizeof(host_src_buffer[0]);
									i++) {
		if (!piglit_cl_probe_integer(host_dst_buffer[i],
						host_src_buffer[i], 0)) {
			fprintf(stderr, "Error at %d\n", i);
			return PIGLIT_FAIL;
		}
	}

	err = clReleaseMemObject(device_src_buffer);
	if (!piglit_cl_check_error(err, CL_SUCCESS)) {
		return PIGLIT_FAIL;
	}
	err = clReleaseMemObject(device_dst_buffer);
	if (!piglit_cl_check_error(err, CL_SUCCESS)) {
		return PIGLIT_FAIL;
	}

	return PIGLIT_PASS;
}
