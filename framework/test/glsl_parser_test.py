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

""" This module enables the running of GLSL parser tests. """

from __future__ import print_function
import os
import os.path as path
import re
import sys

from .piglit_test import PiglitTest

__all__ = [
    'GLSLParserTest',
    'add_glsl_parser_test',
    'import_glsl_parser_tests',
]


def add_glsl_parser_test(group, filepath, test_name):
    """Add an instance of GLSLParserTest to the given group."""
    group[test_name] = GLSLParserTest(filepath)


def import_glsl_parser_tests(group, basepath, subdirectories):
    """
    Recursively register each shader source file in the given
    ``subdirectories`` as a GLSLParserTest .

    :subdirectories: A list of subdirectories under the basepath.

    The name with which each test is registered into the given group is
    the shader source file's path relative to ``basepath``. For example,
    if::
            import_glsl_parser_tests(group, 'a', ['b1', 'b2'])
    is called and the file 'a/b1/c/d.frag' exists, then the test is
    registered into the group as ``group['b1/c/d.frag']``.
    """
    for d in subdirectories:
        walk_dir = path.join(basepath, d)
        for (dirpath, dirnames, filenames) in os.walk(walk_dir):
            # Ignore dirnames.
            for f in filenames:
                # Add f as a test if its file extension is good.
                ext = f.rsplit('.')[-1]
                if ext in ['vert', 'tesc', 'tese', 'geom', 'frag', 'comp']:
                    filepath = path.join(dirpath, f)
                    # testname := filepath relative to
                    # basepath.
                    testname = os.path.relpath(filepath, basepath)
                    if os.path.sep != '/':
                        testname = testname.replace(os.path.sep, '/', -1)
                    assert isinstance(testname, basestring)
                    add_glsl_parser_test(group, filepath, testname)


class GLSLParserTest(PiglitTest):
    """ Read the options in a glsl parser test and create a Test object

    Specifically it is necessary to parse a glsl_parser_test to get information
    about it before actually creating a PiglitTest. Even though this could
    be done with a funciton wrapper, making it a distinct class makes it easier
    to sort in the profile.

    Arguments:
    filepath -- the path to a glsl_parser_test which must end in .vert,
                .tesc, .tese, .geom or .frag

    """
    _CONFIG_KEYS = frozenset(['expect_result', 'glsl_version',
                              'require_extensions', 'check_link'])

    def __init__(self, filepath):
        os.stat(filepath)

        # a set that stores a list of keys that have been found already
        self.__found_keys = set()

        # Parse the config file and get the config section, then write this
        # section to a StringIO and pass that to ConfigParser
        with open(filepath, 'r') as testfile:
            try:
                config = self.__parser(testfile, filepath)
            except GLSLParserException as e:
                print(e.message, file=sys.stderr)
                sys.exit(1)

        command = self.__get_command(config, filepath)
        super(GLSLParserTest, self).__init__(command, run_concurrent=True)

    def __get_command(self, config, filepath):
        """ Create the command argument to pass to super()

        This private helper creates a configparser object, then reads in the
        provided config (from self.__parser), and tests for required options
        that must be provided. If it does not find them it raises an exception.
        It then crafts a command which is returned, and ultimately passed to
        super()

        """
        for opt in ['expect_result', 'glsl_version']:
            if not config.get(opt):
                raise GLSLParserException("Missing required section {} "
                                          "from config".format(opt))

        # Create the command and pass it into a PiglitTest()
        command = [
            'glslparsertest',
            filepath,
            config['expect_result'],
            config['glsl_version']
        ]

        if config['check_link'].lower() == 'true':
            command.append('--check-link')
        command.extend(config['require_extensions'].split())

        return command

    def __parser(self, testfile, filepath):
        """ Private helper that parses the config file

        This method parses the lines of text file, and then returns a
        StrinIO instance suitable to be parsed by a configparser class.

        It will raise GLSLParserExceptions if any part of the parsing
        fails.

        """
        keys = {'require_extensions': '', 'check_link': 'false'}

        # Text of config section.
        # Create a generator that iterates over the lines in the test file.
        # This allows us to run the loop until we find the header, stop and
        # then run again looking for the config sections.
        # This reduces the need for if statements substantially
        lines = (l.strip() for l in testfile)

        is_header = re.compile(r'(//|/\*|\*)\s*\[config\]')
        for line in lines:
            if is_header.match(line):
                break
        else:
            raise GLSLParserException("No [config] section found!")

        is_header = re.compile(r'(//|/\*|\*)\s*\[end config\]')
        is_metadata = re.compile(
            r'(//|/\*|\*)\s*(?P<key>[a-z_]*)\:\s(?P<value>.*)')
        bad_values = re.compile(r'(?![\w\.\! ]).*')

        for line in lines:
            # If strip renendered '' that means we had a blank newline,
            # just go on
            if line in ['', '//']:
                continue
            # If we get to the end of the config break
            elif is_header.match(line):
                break

            match = is_metadata.match(line)
            if match:
                if match.group('key') not in GLSLParserTest._CONFIG_KEYS:
                    raise GLSLParserException(
                        "Key {0} in file {1} is not a valid key for a "
                        "glslparser test config block".format(
                            match.group('key'), filepath))
                elif match.group('key') in self.__found_keys:
                    # If this key has already been encountered throw an error,
                    # there are no duplicate keys allows
                    raise GLSLParserException(
                        'Duplicate entry for key {0} in file {1}'.format(
                            match.group('key'), filepath))
                else:
                    bad = bad_values.search(match.group('value'))
                    # XXX: this always seems to return a match object, even
                    # when the match is ''
                    if bad.group():
                        raise GLSLParserException(
                            'Bad character "{0}" in file: "{1}", '
                            'line: "{2}". Only alphanumerics, _, and space '
                            'are allowed'.format(
                                bad.group()[0], filepath, line))

                    # Otherwise add the key to the set of found keys, and add
                    # it to the dictionary that will be returned
                    self.__found_keys.add(match.group('key'))
                    keys[match.group('key')] = match.group('value')
            else:
                raise GLSLParserException(
                    "The config section is malformed."
                    "Check file {0} for line {1}".format(filepath, line))
        else:
            raise GLSLParserException("No [end config] section found!")

        return keys


class GLSLParserException(Exception):
    pass
