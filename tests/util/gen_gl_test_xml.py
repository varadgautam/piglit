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
import re

# Prefer the external module 'lxml.etree' (it uses libxml2) over Python's
# builtin 'xml.etree.ElementTree'.  It's faster.
try:
    import lxml.etree as etree
except ImportError:
    import xml.etree.cElementTree as etree


def main():
    argparser = argparse.ArgumentParser()
    argparser.add_argument('c_file')
    args = argparser.parse_args()

    with open(args.c_file, 'r') as c_file:
        gen_xml(c_file)

def gen_xml(c_file):
    config_re = make_config_regex()

    config_data = dict()
    has_begun = False
    has_ended = False

    for line in c_file:
        match = config_re.search(line)
        if match is None:
            continue
        print(match.groups())

def make_config_regex():
    config_regex = '|'.join([
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
    config_regex = '(' + config_regex + ')+'
    config_regex = re.compile(config_regex, re.UNICODE)
    return config_regex

if __name__ == '__main__':
    main()
