# Copyright (c) 2014 Intel Corporation

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

""" Provides tests for the shader_test module """

from __future__ import print_function, absolute_import
import os

import nose.tools as nt

from framework import exceptions
import framework.test as testm
import framework.tests.utils as utils


def test_initialize_shader_test():
    """test.shader_test.ShaderTest: class initializes"""
    testm.ShaderTest('tests/spec/glsl-es-1.00/execution/sanity.shader_test')


def test_parse_gl_test_no_decimal():
    """test.shader_test.ShaderTest: raises if version lacks decminal"""
    data = ('[require]\n'
            'GL = 2\n')
    with utils.tempfile(data) as temp:
        with nt.assert_raises(exceptions.PiglitFatalError) as exc:
            testm.ShaderTest(temp)
            nt.assert_equal(exc.exception, "No GL version set",
                            msg="A GL version was passed without a decimal, "
                                "which should have raised an exception, but "
                                "did not")


def test_parse_gles2_test():
    """test.shader_test.ShaderTest: Identifies GLES2 tests successfully"""
    data = ('[require]\n'
            'GL ES >= 2.0\n'
            'GLSL ES >= 1.00\n')
    with utils.tempfile(data) as temp:
        test = testm.ShaderTest(temp)

    nt.assert_equal(
        os.path.basename(test.command[0]), "shader_runner_gles2",
        msg="This test should have run with shader_runner_gles2, "
            "but instead ran with " + os.path.basename(test.command[0]))


def test_parse_gles3_test():
    """test.shader_test.ShaderTest: Identifies GLES3 tests successfully"""
    data = ('[require]\n'
            'GL ES >= 3.0\n'
            'GLSL ES >= 3.00\n')
    with utils.tempfile(data) as temp:
        test = testm.ShaderTest(temp)

    nt.assert_equal(
        os.path.basename(test.command[0]), "shader_runner_gles3",
        msg="This test should have run with shader_runner_gles3, "
            "but instead ran with " + os.path.basename(test.command[0]))


def test_add_auto():
    """test.shader_test.ShaderTest: -auto is added to the command"""
    test = testm.ShaderTest('tests/spec/glsl-es-1.00/execution/sanity.shader_test')
    nt.assert_in('-auto', test.command)
