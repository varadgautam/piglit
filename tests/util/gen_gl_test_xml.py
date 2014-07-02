# Copyright 2014 Intel Corporation
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
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

"""
TODO(chadv): comment
"""

from __future__ import print_function

import argparse
import os
import re

# Prefer the external module 'lxml.etree' (it uses libxml2) over Python's
# builtin 'xml.etree.ElementTree'.  It's faster.
#try:
#    import lxml.etree as etree
#except ImportError:
#    import xml.etree.cElementTree as etree

import xml.etree.cElementTree as etree


class ConfigBlock(object):

    REGEX = '|'.join([
        r'(?P<begin>PIGLIT_GL_TEST_CONFIG_BEGIN)',
        r'(config.supports_gl_core_version = (?P<core_version>\d+);)',
        r'(config.supports_gl_compat_version = (?P<compat_version>\d+);)',
        r'(config.supports_gl_es_version = (?P<es1_version>(10|11));)',
        r'(config.supports_gl_es_version = (?P<es2_version>(20|30|31));)',
        r'(config.require_forward_compatible_context = (?P<require_fwd_comat>true);)',
        r'(config.require_debug_context = (?P<require_debug>true);)',
        r'(config.requires_displayed_window = (?P<requires_displayed_window>true);)',
        r'(config.window_width = (?P<window_width>\d+);)',
        r'(config.window_height = (?P<window_height>\d+);)',
        r'(config.window_samples = (?P<window_samples>\d+);)',
        r'(?P<window_rgb>\bPIGLIT_GL_VISUAL_RGB\b)',
        r'(?P<window_rgba>\bPIGLIT_GL_VISUAL_RGBA\b)',
        r'(?P<window_double>\bPIGLIT_GL_VISUAL_DOUBLE\b)',
        r'(?P<window_accum>\bPIGLIT_GL_VISUAL_ACCUM\b)',
        r'(?P<window_depth>\bPIGLIT_GL_VISUAL_DEPTH\b)',
        r'(?P<window_stencil>\bPIGLIT_GL_VISUAL_STENCIL\b)',
        r'(?P<end>PIGLIT_GL_TEST_CONFIG_END)',
    ])
    REGEX = '(' + REGEX + ')+'
    REGEX = re.compile(REGEX, re.UNICODE)

    @staticmethod
    def from_file(c_file):
        self = ConfigBlock()

        found_begin = False
        found_end = False
        found_keys = set()

        for line in c_file:
            match = ConfigBlock.REGEX.search(line)
            if match is None:
                continue

            for (key, value) in match.groupdict().iteritems():
                if value is None:
                    continue

                if key in found_keys:
                    raise Exception('config block has multiple occurences of {0!r}'.format(key))

                if not found_begin and key != 'begin':
                    raise Exception('found config data {0!r} PIGLIT_GL_TEST_CONFIG_BEGIN'.format(key))

                found_keys.add(key)

                if key == 'begin':
                    found_begin = True
                elif key == 'end':
                    found_end = True
                elif key.endswith('version'):
                    setattr(self, key, value)
                elif key in ('window_width', 'window_height'):
                    setattr(self, key, value)
                else:
                    setattr(self, key, True)

        if not found_begin:
            raise Exception('failed to find PIGLIT_GL_TEST_CONFIG_BEGIN')

        if not found_end:
            raise Exception('failed to find PIGLIT_GL_TEST_CONFIG_END')

        return self

    def __init__(self):
        self.core_version = 0
        self.compat_version = 0
        self.es1_version = 0
        self.es2_version = 0

        self.require_fwd_compat = False
        self.require_debug = False
        self.require_displayed_window = False

        self.window_width = 0
        self.window_height = 0

        self.window_accum = False
        self.window_depth = False
        self.window_double = False
        self.window_rgb = False
        self.window_rgbs = False
        self.window_stencil = False

    def get_xml(self):
        b = etree.TreeBuilder()

        piglit = b.start('piglit')
        gl_test = b.start('gl-test')
        if self.core_version:
            ctx_flavor = b.start('context-flavor')
            ctx_flavor.attrs['api'] = 'core'
            ctx_flavor.attrs['version'] = self.core_version
            ctx_flavor.end()
        if self.compat_version:
            ctx_flavor = b.start('context-flavor')
            ctx_flavor.set('api', 'compat')
            ctx_flavor.set('version', self.compat_version)
            b.end(ctx_flavor)
        b.end(gl_test)
        b.end(piglit)

        return b.close()

def main():
    argparser = argparse.ArgumentParser()
    argparser.add_argument('c_file')
    args = argparser.parse_args()

    print(args)

    c_filename = args.c_file
    (basename, ext) = os.path.splitext(os.path.basename(c_filename))
    xml_filename = basename + '.xml'

    c_file = open(c_filename, 'r')
    xml_file = open(xml_filename, 'w')

    config = ConfigBlock.from_file(c_file)
    xml = config.get_xml()
    print(etree.tostring(xml))

    c_file.close()
    xml_file.close()

if __name__ == '__main__':
    main()
