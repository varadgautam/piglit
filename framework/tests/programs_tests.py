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


""" Tests for the programs package

Currently there aren't very many tests for the modules in this package, so just
having a single test module seems appropriate

"""

import os
import shutil
import ConfigParser
import framework.core as core
import framework.programs.run as run
import framework.tests.utils as utils
import nose.tools as nt

CONF_FILE = """
[nose-test]
; a section for testing behavior
dir = foo
"""


class TestGetConfigEnv(utils.TestWithEnvClean):
    def test(self):
        """ get_config() finds $XDG_CONFIG_HOME/piglit.conf """
        self.defer(lambda: core.PIGLIT_CONFIG == ConfigParser.SafeConfigParser)
        self.add_teardown('XDG_CONFIG_HOME')
        if os.path.exists('piglit.conf'):
            shutil.move('piglit.conf', 'piglit.conf.restore')
            self.defer(shutil.move, 'piglit.conf.restore', 'piglit.conf')

        with utils.tempdir() as tdir:
            os.environ['XDG_CONFIG_HOME'] = tdir
            with open(os.path.join(tdir, 'piglit.conf'), 'w') as f:
                f.write(CONF_FILE)
            core.get_config()

        nt.ok_(core.PIGLIT_CONFIG.has_section('nose-test'),
               msg='$XDG_CONFIG_HOME not found')


class TestGetConfigHomeFallback(utils.TestWithEnvClean):
    def test(self):
        """ get_config() finds $HOME/.config/piglit.conf """
        self.defer(lambda: core.PIGLIT_CONFIG == ConfigParser.SafeConfigParser)
        self.add_teardown('HOME')
        self.add_teardown('XDG_CONFIG_HOME')
        if os.path.exists('piglit.conf'):
            shutil.move('piglit.conf', 'piglit.conf.restore')
            self.defer(shutil.move, 'piglit.conf.restore', 'piglit.conf')

        with utils.tempdir() as tdir:
            os.environ['HOME'] = tdir
            os.mkdir(os.path.join(tdir, '.config'))
            with open(os.path.join(tdir, '.config/piglit.conf'), 'w') as f:
                f.write(CONF_FILE)

        nt.ok_(core.PIGLIT_CONFIG.has_section('nose-test'),
               msg='$HOME/.config not found')


class TestGetConfigLocal(utils.TestWithEnvClean):
    # These need to be empty to force '.' to be used
    def test(self):
        """ get_config() finds ./piglit.conf """
        self.defer(lambda: core.PIGLIT_CONFIG == ConfigParser.SafeConfigParser)
        self.add_teardown('HOME')
        self.add_teardown('XDG_CONFIG_HOME')
        if os.path.exists('piglit.conf'):
            shutil.move('piglit.conf', 'piglit.conf.restore')
            self.defer(shutil.move, 'piglit.conf.restore', 'piglit.conf')

        with utils.tempdir() as tdir:
            self.defer(os.chdir, os.getcwd())
            os.chdir(tdir)

            with open(os.path.join(tdir, 'piglit.conf'), 'w') as f:
                f.write(CONF_FILE)

            core.get_config()

        nt.ok_(core.PIGLIT_CONFIG.has_section('nose-test'),
               msg='./piglit.conf not found')


class TestGetConfigRoot(utils.TestWithEnvClean):
    def test(self):
        """ get_config() finds "piglit root"/piglit.conf """
        self.defer(lambda: core.PIGLIT_CONFIG == ConfigParser.SafeConfigParser)
        self.add_teardown('HOME')
        self.add_teardown('XDG_CONFIG_HOME')

        if os.path.exists('piglit.conf'):
            shutil.move('piglit.conf', 'piglit.conf.restore')
            self.defer(shutil.move, 'piglit.conf.restore', 'piglit.conf')

        with open('piglit.conf', 'w') as f:
            f.write(CONF_FILE)
        self.defer(os.unlink, 'piglit.conf')
        self.defer(os.chdir, os.getcwd())
        os.chdir('..')

        core.get_config()

        nt.ok_(core.PIGLIT_CONFIG.has_section('nose-test'),
               msg='$PIGLIT_ROOT not found')
