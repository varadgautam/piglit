/* Copyright © 2013 Intel Corporation
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

/**
 * \file piglit-fbo.h
 * This file declares classes to initialize a framebuffer object as per piglit
 * test's requirements.
 */

#include "piglit-util-gl.h"
#include "math.h"

namespace piglit_util_fbo {
/* I think 16 is the sufficient number of color attachments which tests would
 * want to use in near future.
 */
#define PIGLIT_MAX_COLOR_ATTACHMENTS 16

	/**
	 * Information needed to configure a framebuffer object for MSAA
	 * testing.
	 */
	class FboConfig
	{
	public:
		FboConfig(int num_samples, int width, int height);

		int num_samples;
		int num_rb_attachments; /* Default value is 1 */
		int num_tex_attachments; /* Default value is 0 */
		int width;
		int height;

		/**
		 * True if a single renderbuffer should be used as the backing
		 * store for both the depth and stencil attachment points.
		 * Defaults to true.
		 */
		bool combine_depth_stencil;

		/**
		 * Set color attachment points for color_tex[i] or color_rb[i].
		 * Default value for color_tex[0] and color_rb[0] is
		 * GL_COLOR_ATTACHMENT0.
		 */
		GLuint rb_attachment[PIGLIT_MAX_COLOR_ATTACHMENTS];
		GLuint tex_attachment[PIGLIT_MAX_COLOR_ATTACHMENTS];

		/**
		 * Useful if num_tex_attachments > 0 and color buffer is
		 * non-multisample. Specifies the format that should be used
		 * for the color buffer, or GL_NONE if no color buffer should
		 * be used. Defaults to GL_RGBA.
		 */
		GLenum color_format;

		/**
		 * Internalformat that should be used for the color buffer, or
		 * GL_NONE if no color buffer should be used.  Defaults to
		 * GL_RGBA.
		 */
		GLenum color_internalformat;

		/**
		 * Internalformat that should be used for the depth buffer, or
		 * GL_NONE if no depth buffer should be used.  Ignored if
		 * combine_depth_stencil is true.  Defaults to
		 * GL_DEPTH_COMPONENT24.
		 */
		GLenum depth_internalformat;

		/**
		 * Internalformat that should be used for the stencil buffer,
		 * or GL_NONE if no stencil buffer should be used.  Ignored if
		 * combine_depth_stencil is true.  Defaults to
		 * GL_STENCIL_INDEX8.
		 */
		GLenum stencil_internalformat;
	};

	/**
	 * Data structure representing one of the framebuffer objects used in
	 * the test.
	 *
	 * For the supersampled framebuffer object we use a texture as the
	 * backing store for the color buffer so that we can use a fragment
	 * shader to blend down to the reference image.
	 */
	class Fbo
	{
	public:
		Fbo();

		void set_samples(int num_samples);
		void setup(const FboConfig &new_config);
		bool try_setup(const FboConfig &new_config);

		void set_viewport();

		FboConfig config;
		GLuint handle;

		/**
		 * If config.num_tex_attachments > 0, the backing store for the
		 * color buffers.
		 */
		GLuint color_tex[PIGLIT_MAX_COLOR_ATTACHMENTS];

		/**
		 * If config.num_rb_attachments > 0, the backing store for the
		 * color buffers.
		 */
		GLuint color_rb[PIGLIT_MAX_COLOR_ATTACHMENTS];

		/**
		 * If config.combine_depth_stencil is true, the backing store
		 * for the depth/stencil buffer.  If
		 * config.combine_depth_stencil is false, the backing store
		 * for the depth buffer.
		 */
		GLuint depth_rb;

		/**
		 * If config.combine_depth_stencil is false, the backing store
		 * for the stencil buffer.
		 */
		GLuint stencil_rb;

	private:
		void generate_gl_objects();
		void attach_color_renderbuffer(const FboConfig &config,
					       int index);
		void attach_color_texture(const FboConfig &config, int index);
		void attach_multisample_color_texture(const FboConfig &config,
						      int index);

		/**
		 * True if generate_gl_objects has been called and color_tex,
		 * color_rb, depth_rb, and stencil_rb have been initialized.
		 */
		bool gl_objects_generated;
	};
}
