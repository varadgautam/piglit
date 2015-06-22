# coding=utf-8
#
# Copyright © 2013, 2014 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

from __future__ import print_function, division, absolute_import
import os
import os.path

import six

from templates import template_file
from modules import utils

TEMPLATE = template_file(os.path.basename(os.path.splitext(__file__)[0]),
                         'template.glsl_parser_test.mako')

SAMPLER_TYPE_TO_COORD_TYPE = {
    'sampler1D':              'float',
    'isampler1D':             'float',
    'usampler1D':             'float',

    'sampler2D':              'vec2',
    'isampler2D':             'vec2',
    'usampler2D':             'vec2',

    'sampler3D':              'vec3',
    'isampler3D':             'vec3',
    'usampler3D':             'vec3',

    'samplerCube':            'vec3',
    'isamplerCube':           'vec3',
    'usamplerCube':           'vec3',

    'sampler1DArray':         'float',
    'isampler1DArray':        'float',
    'usampler1DArray':        'float',

    'sampler2DArray':         'vec2',
    'isampler2DArray':        'vec2',
    'usampler2DArray':        'vec2',

    'samplerCubeArray':       'vec3',
    'isamplerCubeArray':      'vec3',
    'usamplerCubeArray':      'vec3',

    'sampler1DShadow':        'float',
    'sampler2DShadow':        'vec2',
    'samplerCubeShadow':      'vec3',
    'sampler1DArrayShadow':   'float',
    'sampler2DArrayShadow':   'vec2',
    'samplerCubeArrayShadow': 'vec3',
}

REQUIREMENTS = {
    'ARB_texture_query_lod': {
        'version': '1.30',
        'extensions': 'GL_ARB_texture_query_lod'
    },
    'glsl-4.00': {
        'version': '4.00',
        'extensions': None
    }
}


def main():
    """Main function."""
    for api, requirement in six.iteritems(REQUIREMENTS):
        lod = 'Lod' if api == 'glsl-4.00' else 'LOD'
        dirname = os.path.join("spec", api.lower(), "compiler",
                               "built-in-functions")
        utils.safe_makedirs(dirname)

        for sampler_type, coord_type in six.iteritems(SAMPLER_TYPE_TO_COORD_TYPE):
            requirements = [requirement['extensions']] if requirement['extensions'] else []

            # samplerCubeArray types are part GLSL 4.00
            # or GL_ARB_texture_cube_map_array.
            if api == "ARB_texture_query_lod" and sampler_type in [
                    'samplerCubeArray', 'isamplerCubeArray',
                    'usamplerCubeArray', 'samplerCubeArrayShadow']:
                requirements.append('GL_ARB_texture_cube_map_array')

            for execution_stage in ("vs", "fs"):
                file_extension = 'frag' if execution_stage == 'fs' else 'vert'
                filename = os.path.join(
                    dirname,
                    "textureQuery{0}-{1}.{2}".format(lod, sampler_type,
                                                     file_extension))
                print(filename)

                with open(filename, "w") as f:
                    f.write(TEMPLATE.render_unicode(
                        version=requirement['version'],
                        extensions=requirements,
                        execution_stage=execution_stage,
                        sampler_type=sampler_type,
                        coord_type=coord_type,
                        lod=lod))


if __name__ == '__main__':
    main()
