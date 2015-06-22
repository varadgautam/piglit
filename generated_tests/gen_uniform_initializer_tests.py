# coding=utf-8
#
# Copyright © 2012 Intel Corporation
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

import os
import os.path
from mako.template import Template


def open_src_file(filename):
    """Open a file relative to the source directory"""
    local_dir = os.path.dirname(__file__)
    return open(os.path.join(local_dir, filename))


def get_value(type, idx):
    """Get a string representing a number in the specified GLSL type"""

    idx = idx % len(random_numbers)
    value = random_numbers[idx]

    if type[0] == 'b':
        if (value * 10) > 5:
            return "1"
        else:
            return "0"
    elif type[0] == 'i':
        return str(int(value * 100) - 50)
    elif type[0] == 'u':
        return str(int(value * 50))
    else:
        return str((value * 20.0) - 10.0)


def generate_tests(type_list, base_name, major, minor):
    for target in ("vs", "fs"):
        for t in all_templates:
            template_file_name = (
                "uniform-initializer-templates/"
                "{0}-initializer{1}.template".format(target, t))
            f = open_src_file(template_file_name)
            template = f.read()
            f.close()

            test_file_name = os.path.join(
                'spec',
                'glsl-{0}.{1}'.format(major, minor),
                'execution',
                'uniform-initializer',
                '{0}-{1}{2}.shader_test'.format(target, base_name, t))
            print test_file_name

            dirname = os.path.dirname(test_file_name)
            if not os.path.exists(dirname):
                os.makedirs(dirname)

            # Generate the test vectors.  This is a list of tuples.  Each
            # tuple is a type name paired with a value.  The value is
            # formatted as a GLSL constructor.
            #
            # A set of types and values is also generated that can be set via
            # the OpenGL API.  Some of the tests use this information.
            test_vectors = []
            api_vectors = []
            j = 0
            for (type, num_values) in type_list:
                numbers = []
                alt_numbers = []
                for i in range(num_values):
                    numbers.append(get_value(type, i + j))
                    alt_numbers.append(get_value(type, i + j + 7))

                value = "{0}({1})".format(type, ", ".join(numbers))

                api_type = type
                if type == "bool":
                    api_type = "int"
                elif type[0] == 'b':
                    api_type = "ivec{0}".format(type[-1])

                if type[-1] in ["2", "3", "4"]:
                    name = "".join(["u", type[0], type[-1]])
                else:
                    name = "".join(["u", type[0]])

                test_vectors.append((type, name, value))
                api_vectors.append((api_type, name, alt_numbers))
                j = j + 1

            f = open(test_file_name, "w")
            f.write(Template(template).render(type_list=test_vectors,
                                              api_types=api_vectors,
                                              major=major,
                                              minor=minor))
            f.close()


def generate_array_tests(type_list, base_name, major, minor):
    for target in ("vs", "fs"):
        template_file_name = \
            "uniform-initializer-templates/{0}-initializer.template".format(target)
        f = open_src_file(template_file_name)
        template = f.read()
        f.close()

        test_file_name = os.path.join(
            'spec',
            'glsl-{0}.{1}'.format(major, minor),
            'execution',
            'uniform-initializer',
            '{0}-{1}-array.shader_test'.format(target, base_name))
        print test_file_name

        dirname = os.path.dirname(test_file_name)
        if not os.path.exists(dirname):
            os.makedirs(dirname)

        test_vectors = []
        j = 0
        for (type, num_values) in type_list:

            constructor_parts = []
            for element in range(2):
                numbers = []
                for i in range(num_values):
                    numbers.append(get_value(type, i + element + j))

                constructor_parts.append("{0}({1})".format(type,
                                                           ", ".join(numbers)))

            if type[-1] in ["2", "3", "4"]:
                name = "".join(["u", type[0], type[-1]])
            else:
                name = "".join(["u", type[0]])

            array_type = "".join([format(type), "[2]"])
            value = "{0}({1})".format(array_type, ", ".join(constructor_parts))
            test_vectors.append((array_type, name, value))
            j = j + 1

        f = open(test_file_name, "w")
        f.write(Template(template).render(type_list=test_vectors,
                                          major=major,
                                          minor=minor))
        f.close()

# These are a set of pseudo random values used by the number sequence
# generator.  See get_value above.
random_numbers = (0.78685, 0.89828, 0.36590, 0.92504, 0.48998, 0.27989,
                  0.08693, 0.48144, 0.87644, 0.18080, 0.95147, 0.18892,
                  0.45851, 0.76423, 0.78659, 0.97998, 0.24352, 0.60922,
                  0.45241, 0.33045, 0.27233, 0.92331, 0.63593, 0.67826,
                  0.12195, 0.24853, 0.35977, 0.41759, 0.79119, 0.54281,
                  0.04089, 0.03877, 0.58445, 0.43017, 0.58635, 0.48151,
                  0.58778, 0.37033, 0.47464, 0.17470, 0.18308, 0.49466,
                  0.45838, 0.30337, 0.71273, 0.45083, 0.88339, 0.47350,
                  0.86539, 0.48355, 0.92923, 0.79107, 0.77266, 0.71677,
                  0.79860, 0.95149, 0.05604, 0.16863, 0.14072, 0.29028,
                  0.57637, 0.13572, 0.36011, 0.65431, 0.38951, 0.73245,
                  0.69497, 0.76041, 0.31016, 0.48708, 0.96677, 0.58732,
                  0.33741, 0.73691, 0.24445, 0.35686, 0.72645, 0.65438,
                  0.00824, 0.00923, 0.87650, 0.43315, 0.67256, 0.66939,
                  0.87706, 0.73880, 0.96248, 0.24148, 0.24126, 0.24673,
                  0.18999, 0.10330, 0.78826, 0.23209, 0.59548, 0.23134,
                  0.72414, 0.88036, 0.54498, 0.32668, 0.02967, 0.12643)

all_templates = ("",
                 "-from-const",
                 "-set-by-API",
                 "-set-by-other-stage")

bool_types = [("bool", 1), ("bvec2", 2), ("bvec3", 3), ("bvec4", 4)]
int_types = [("int", 1), ("ivec2", 2), ("ivec3", 3), ("ivec4", 4)]
float_types = [("float", 1), ("vec2", 2), ("vec3", 3), ("vec4", 4)]
uint_types = [("uint", 1), ("uvec2", 2), ("uvec3", 3), ("uvec4", 4)]
mat2_types = [("mat2x2", 4), ("mat2x3", 6), ("mat2x4", 8)]
mat3_types = [("mat3x2", 6), ("mat3x3", 9), ("mat3x4", 12)]
mat4_types = [("mat4x2", 8), ("mat4x3", 12), ("mat4x4", 16)]

for (types, base_name, major, minor) in [(bool_types,  "bool",  1, 20),
                                         (int_types,   "int",   1, 20),
                                         (float_types, "float", 1, 20),
                                         (mat2_types,  "mat2",  1, 20),
                                         (mat3_types,  "mat3",  1, 20),
                                         (mat4_types,  "mat4",  1, 20),
                                         (uint_types,  "uint",  1, 30)]:
    generate_tests(types, base_name, major, minor)
    generate_array_tests(types, base_name, major, minor)
