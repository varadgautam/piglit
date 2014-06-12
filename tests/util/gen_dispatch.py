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
Generate C source code from Khronos XML.
"""

from __future__ import print_function

import argparse
import mako.runtime
import mako.template
import os.path
import re
import sys

from collections import namedtuple
from textwrap import dedent

PIGLIT_TOP_DIR = os.path.join(os.path.dirname(__file__), '..', '..')
sys.path.append(PIGLIT_TOP_DIR)

import registry.gl
from registry.gl import OrderedKeyedSet, ImmutableOrderedKeyedSet


debug = False


def log_debug(msg):
    if debug:
        prog_name = os.path.basename(sys.argv[0])
        print('debug: {0}: {1}'.format(prog_name, msg), file=sys.stderr)


def main():
    global debug

    argparser = argparse.ArgumentParser()
    argparser.add_argument('-o', '--out-dir', required=True)
    argparser.add_argument('-d', '--debug', action='store_true', default=False)
    args = argparser.parse_args()

    debug = args.debug
    registry.gl.debug = debug

    gl_registry = registry.gl.parse()
    DispatchCode(gl_registry).emit(args.out_dir)
    EnumCode(gl_registry).emit(args.out_dir)


warning = 'DO NOT EDIT! A script generated this file.'


def render_template(template_name, template_vars, out_dir):
    fake_alignment = re.compile(r'\.*\n\.+', flags=re.MULTILINE)
    fake_tab = re.compile(r'>-------')

    def fake_whitespace(proto_text):
        if debug:
            print('fake whitespace: before: {0!r}'.format(proto_text))
        text = unicode(proto_text)
        text = fake_alignment.sub('', text)
        text = fake_tab.sub('\t', text)
        if debug:
            print('fake whitespace:  after: {0!r}'.format(text))
        return text

    template_filename = os.path.join(os.path.dirname(__file__), template_name)
    out_filename = os.path.join(out_dir, template_name[:-len('.template')])
    with open(out_filename, 'w') as out_file:
        template = mako.template.Template(
            filename=template_filename,
            strict_undefined=True)
        ctx = mako.runtime.Context(
            buffer=out_file,
            fake_whitespace=fake_whitespace,
            **template_vars)
        template.render_context(ctx)


DispatchApi = namedtuple('DispatchApi', ('name', 'base_version_int', 'c_piglit_token'))
DISPATCH_APIS = {
    'gl':    DispatchApi(name='gl',      c_piglit_token='PIGLIT_DISPATCH_GL',  base_version_int=10),
    'gles1': DispatchApi(name='gles1',   c_piglit_token='PIGLIT_DISPATCH_ES1', base_version_int=11),
    'gles2': DispatchApi(name='gles2',   c_piglit_token='PIGLIT_DISPATCH_ES2', base_version_int=20),
}
DISPATCH_APIS['glcore'] = DISPATCH_APIS['gl']
assert(set(DISPATCH_APIS.keys()) == set(registry.gl.VALID_APIS))


class DispatchCode(object):

    H_TEMPLATE = 'piglit-dispatch-gen.h.template'
    C_TEMPLATE = 'piglit-dispatch-gen.c.template'

    def __init__(self, gl_registry):
        assert(isinstance(gl_registry, registry.gl.Registry))
        self.gl_registry = gl_registry

    def emit(self, out_dir):
        template_vars = dict(
            warning=warning,
            gl_registry=self.gl_registry,
            command_alias_map=sorted(set(self.gl_registry.command_alias_map)),
            DISPATCH_APIS=DISPATCH_APIS,
        )

        render_template(self.H_TEMPLATE, template_vars, out_dir)
        render_template(self.C_TEMPLATE, template_vars, out_dir)


class EnumCode(object):

    C_TEMPLATE = 'piglit-util-gl-enum-gen.c.template'

    def __init__(self, gl_registry):
        assert(isinstance(gl_registry, registry.gl.Registry))
        self.gl_registry = gl_registry
        self.unique_default_namespace_enums = None
        self.__gather_unique_default_namespace_enums()

    def emit(self, out_dir):
        template_vars = dict(
            warning=warning,
            gl_registry=self.gl_registry,
            unique_default_namespace_enums=self.unique_default_namespace_enums,
        )
        render_template(self.C_TEMPLATE, template_vars, out_dir)


    def __gather_unique_default_namespace_enums(self):
        duplicate_enums = [
            enum
            for enum_group in self.gl_registry.enum_groups
            if enum_group.type == 'default_namespace'
            for enum in enum_group.enums
        ]

        # Sort enums by numerical value then by name. This ensures that
        # non-suffixed variants of an enum name precede the suffixed variant.
        # For example, GL_RED will precede GL_RED_EXT.
        def enum_cmp(x, y):
            c = cmp(x.num_value, y.num_value)
            if c != 0:
                return c
            c = cmp(x.name, y.name)
            if c != 0:
                return c
            return 0

        duplicate_enums.sort(enum_cmp)

        # Copy duplicate_enums into unique_enums, filtering out duplicate
        # values. The algorithm requires that dupliate_enums be sorted by
        # value.
        unique_enums = [duplicate_enums[0]]
        for enum in duplicate_enums[1:]:
            if enum.num_value > unique_enums[-1].num_value:
                unique_enums.append(enum)

        self.unique_default_namespace_enums = unique_enums


if __name__ == '__main__':
    main()
