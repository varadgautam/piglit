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

import argparse
import mako
import os.path
import sys

from cStringIO import StringIO
from collections import namedtuple
from textwrap import dedent
from mako.template import Template

PIGLIT_TOP_DIR = os.path.join(os.path.dirname(__file__), '..', '..')
sys.path.append(PIGLIT_TOP_DIR)
import registry.gl
from registry.gl import OrderedKeyedSet

PROG_NAME = os.path.basename(sys.argv[0])
debug = False

def main():
    global debug

    argparser = argparse.ArgumentParser(prog=PROG_NAME)
    argparser.add_argument('-o', '--out-dir')
    argparser.add_argument('-d', '--debug', action='store_true', default=False)
    args = argparser.parse_args()

    debug = args.debug
    if args.out_dir is None:
        argparser.error('missing OUT_DIR')

    registry.gl.debug = debug
    gl_registry = registry.gl.parse()

    dispatch_h = open(os.path.join(args.out_dir, 'generated_dispatch.h'), 'w')
    dispatch_c = open(os.path.join(args.out_dir, 'generated_dispatch.c'), 'w')
    DispatchCode(gl_registry).emit(dispatch_h, dispatch_c)
    dispatch_h.close()
    dispatch_c.close()

def log_debug(msg):
    if debug:
        sys.stderr.write('debug: {0}: {1}\n'.format(PROG_NAME, msg))

copyright_block = dedent('''\
    /**
     * DO NOT EDIT!
     * This file was generated by the script {PROG_NAME!r}.
     *
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
     */'''
).format(PROG_NAME=PROG_NAME)

Api = namedtuple('Api', ('name', 'base_version_int', 'c_piglit_token'))
"""Api corresponds to the XML <feature> tag's 'api' attribute."""

apis = OrderedKeyedSet(
    key='name', elems=(
        Api(name='gl',      c_piglit_token='PIGLIT_DISPATCH_GL',  base_version_int=10),
        Api(name='glcore',  c_piglit_token='PIGLIT_DISPATCH_GL',  base_version_int=10),
        Api(name='gles1',   c_piglit_token='PIGLIT_DISPATCH_ES1', base_version_int=11),
        Api(name='gles2',   c_piglit_token='PIGLIT_DISPATCH_ES1', base_version_int=20),
    )
)

class CommandAliasMap(OrderedKeyedSet):

    unaliasable_commands = [
        'glGetObjectParameterivAPPLE',
        'glGetObjectParameterivARB',
    ]

    def __init__(self, commands=()):
        OrderedKeyedSet.__init__(self, key='name', elems=commands)

    def add(self, alias_set):
        assert(isinstance(alias_set, CommandAliasSet))
        OrderedKeyedSet.add(self, alias_set)

    def add_commands(self, commands):
        for command in commands:
            assert(isinstance(command, registry.gl.Command))

            if command.name in CommandAliasMap.unaliasable_commands:
                alias_name = command.name
            else:
                alias_name = command.basename

            alias_set = self.get(alias_name, None)

            if alias_set is None:
                self.add(CommandAliasSet(alias_name, [command]))
            else:
                alias_set.add(command)

class CommandAliasSet(OrderedKeyedSet):

    Resolution = namedtuple('Resolution', ('c_condition', 'c_get_proc'))

    def __init__(self, name, commands):
        OrderedKeyedSet.__init__(self, key='name')
        self.name = name
        for command in commands:
            self.add(command)

    def __cmp__(x, y):
        return cmp(x.name, y.name)

    @property
    def primary_command(self):
        return sorted(self)[0]

    @property
    def resolutions(self):
        """Iterable of Resolution actions to obtain the GL proc address."""
        Resolution = self.__class__.Resolution
        for command in self:
            for feature in command.features:
                api = apis[feature.api]
                format_map = dict(command=command, feature=feature, api=api)
                condition = 'dispatch_api == {api.c_piglit_token}'.format(**format_map)
                get_proc = 'get_core_proc("{command.name}", {feature.version_int})'.format(**format_map)
                if feature.version_int > api.base_version_int:
                    condition += ' && check_version({feature.version_int})'.format(**format_map)
                yield Resolution(condition, get_proc)

            for extension in command.extensions:
                format_map = dict(command=command, extension=extension)
                condition = 'check_extension("{extension.name}")'.format(**format_map)
                get_proc = 'get_ext_proc("{command.name}")'.format(**format_map)
                yield Resolution(condition, get_proc)

    def add(self, command):
        assert(isinstance(command, registry.gl.Command))
        OrderedKeyedSet.add(self, command)

class DispatchCode(object):

    h_template = Template(dedent('''\
        ${copyright}

        #ifndef __PIGLIT_DISPATCH_GEN_H__
        #define __PIGLIT_DISPATCH_GEN_H__

        % for f in sorted(gl_registry.commands):
        typedef ${f.c_return_type} (APIENTRY *PFN${f.name.upper()}PROC)(${f.c_named_param_list});
        % endfor
        % for alias_set in alias_map:
        <% f0 = alias_set.primary_command %>
        %   for f in alias_set:
        <%      cat_list = ' '.join(['(' + cat.name + ')' for cat in f.requirements]) %>\\
        /* ${f.name} ${cat_list} */
        %   endfor
        extern PFN${f0.name.upper()}PROC piglit_dispatch_${f0.name};
        %   for f in alias_set:
        #define ${f.name} piglit_dispatch_${f0.name}
        %   endfor
        % endfor
        % for enum_group in gl_registry.enum_groups:
        %   if len(enum_group.enums) > 0:

        /* Enum Group ${enum_group.name} */
        %     for enum in enum_group.enums:
        %         if not enum.is_collider:
        #define ${enum.name} ${enum.c_num_literal}
        %         endif
        %      endfor
        %   endif
        % endfor

        % for extension in gl_registry.extensions:
        #define ${extension.name} 1
        % endfor

        % for feature in gl_registry.features:
        #define ${feature.name} 1
        % endfor

        #endif /*__PIGLIT_DISPATCH_GEN_H__*/'''
    ))

    c_template = Template(dedent('''\
        ${copyright}
        % for alias_set in alias_map:
        <% f0 = alias_set.primary_command %>
        %   for f in alias_set:
        <%      cat_list = ' '.join(['(' + cat.name + ')' for cat in f.requirements]) %>\\
        /* ${f.name} ${cat_list} */
        %   endfor
        static void* resolve_${f0.name}(void)
        {
        %   for resolution in alias_set.resolutions:
        %       if loop.first:
            if (${resolution.c_condition}) {
        %       else:
            } else if (${resolution.c_condition}) {
        %       endif
                return ${resolution.c_get_proc};
        %   endfor
            } else {
                unsupported("${f0.name}");
                return piglit_dispatch_${f0.name};
            }
        }

        static ${f0.c_return_type} APIENTRY stub_${f0.name}(${f0.c_named_param_list})
        {
        <%
            if f0.c_return_type == 'void': 
                maybe_return = ''
            else:
                maybe_return = 'return '
        %>\\
            check_initialized();
            piglit_dispatch_${f0.name} = resolve_${f0.name}();
            ${maybe_return}piglit_dispatch_${f0.name}(${f0.c_untyped_param_list});
        }

        PFN${f0.name.upper()}PROC piglit_dispatch_${f0.name} = stub_${f0.name};
        % endfor

        static void reset_dispatch_pointers(void)
        {
        % for alias_set in alias_map:
        <% f0 = alias_set.primary_command %>\\
            piglit_dispatch_${f0.name} = stub_${f0.name};
        % endfor
        }

        static const char * function_names[] = {
        % for f in gl_registry.commands:
            "${f.name}",
        % endfor
        };

        static void* (*const function_resolvers[])(void) = {
        % for alias_set in alias_map:
        <% f0 = alias_set.primary_command %>\\
            resolve_${f0.name},
        % endfor
        };
        '''
    ))

    def __init__(self, gl_registry):
        assert(isinstance(gl_registry, registry.gl.Registry))
        self.gl_registry = gl_registry
        self.alias_map = CommandAliasMap()
        self.alias_map.add_commands(gl_registry.commands)

    def emit(self, h_buf, c_buf):
        for (buf, template) in (
                (h_buf, DispatchCode.h_template),
                (c_buf, DispatchCode.c_template)):
            ctx = mako.runtime.Context(
                    buffer=buf,
                    gl_registry=self.gl_registry,
                    alias_map=self.alias_map,
                    copyright=copyright_block,
            )
            template.render_context(ctx)

if __name__ == '__main__':
    main()
