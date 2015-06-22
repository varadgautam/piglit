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

""" Tests for the exectest module """

import nose.tools as nt
from framework.test.piglit_test import (PiglitBaseTest, PiglitGLTest,
                                        PiglitCLTest)


def test_initialize_piglitgltest():
    """ Test that PiglitGLTest initializes correctly """
    try:
        PiglitGLTest('/bin/true')
    except Exception as e:
        raise AssertionError(e)


def test_initialize_piglitcltest():
    """ Test that PiglitCLTest initializes correctly """
    try:
        PiglitCLTest('/bin/true')
    except Exception as e:
        raise AssertionError(e)


def test_piglittest_interpret_result():
    """ PiglitBaseTest.interpret_result() works no subtests """
    test = PiglitBaseTest('foo')
    test.result['out'] = 'PIGLIT: {"result": "pass"}\n'
    test.interpret_result()
    assert test.result['result'] == 'pass'


def test_piglittest_interpret_result_subtest():
    """ PiglitBaseTest.interpret_result() works with subtests """
    test = PiglitBaseTest('foo')
    test.result['out'] = ('PIGLIT: {"result": "pass"}\n'
                          'PIGLIT: {"subtest": {"subtest": "pass"}}\n')
    test.interpret_result()
    assert test.result['subtest']['subtest'] == 'pass'


def test_piglitest_no_clobber():
    """ PiglitBaseTest.interpret_result() does not clobber subtest entires """
    test = PiglitBaseTest(['a', 'command'])
    test.result['out'] = (
        'PIGLIT: {"result": "pass"}\n'
        'PIGLIT: {"subtest": {"test1": "pass"}}\n'
        'PIGLIT: {"subtest": {"test2": "pass"}}\n'
    )
    test.interpret_result()

    nt.assert_dict_equal(test.result['subtest'],
                         {'test1': 'pass', 'test2': 'pass'})
