#
# Permission is hereby granted, free of charge, to any person

# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use,
# copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following
# conditions:
#
# This permission notice shall be included in all copies or
# substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
# KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
# PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHOR(S) BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
# AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
# OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

import os
import framework.exectest as exectest

glean_executable = os.path.join(exectest.TEST_BIN_DIR, "glean")


# GleanTest: Execute a sub-test of Glean
class GleanTest(exectest.Test):
    GLOBAL_PARAMS = []

    def __init__(self, name, **kwargs):
        super(GleanTest, self).__init__(
            [glean_executable, "-o", "-v", "-v", "-v", "-t", "+" + name],
            **kwargs)

    @exectest.Test.command.getter
    def command(self):
        return super(GleanTest, self).command + self.GLOBAL_PARAMS

    def interpret_result(self):
        if self.result['returncode'] != 0 or 'FAIL' in self.result['out']:
            self.result['result'] = 'fail'
        else:
            self.result['result'] = 'pass'

    def is_skip(self):
        # Glean tests require glx
        if exectest.PIGLIT_PLATFORM != 'glx':
            return True
        return super(GleanTest, self).is_skip()
