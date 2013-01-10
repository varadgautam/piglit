#!/usr/bin/env python2

import mako.template
import mako.runtime
import math
import optparse
import os
import sys
import textwrap as tw

from collections import namedtuple
from mako.template import Template
from subprocess import check_output

# ----------------------------------------------------------------------------
# Overview
# ----------------------------------------------------------------------------

# This scripts generates tests for the GLSL packing functions, such as
# packSnorm2x16.

# Given an input and a packing/unpacking function, there exist multiple
# possible outputs. The actual output is dependent on the GLSL compiler's and
# hardware's choice of rounding mode (to even or to nearest) and handling of
# subnormal (also called denormalized) floating point numbers. In each
# generated test, the function's actual output is compared against
# a reasonable subset set of possible outputs.

# For details on how the expected outputs are calculated, see the source of the
# glsl-packing tool.

# ----------------------------------------------------------------------------
# Templates
# ----------------------------------------------------------------------------

# Test evaluation of constant pack2x16 expressions.
const_pack_2x16_template = Template(tw.dedent("""\
    [require]
    GL >= 3.0 es
    GLSL >= 3.00 es

    [vertex shader]
    const vec4 red = vec4(1, 0, 0, 1);
    const vec4 green = vec4(0, 1, 0, 1);

    in vec4 vertex;
    out vec4 vert_color;

    void main()
    {
        ${func.return_precision} uint actual;

        gl_Position = vertex;

        % for io in func.inouts:
        actual = ${func.name}(vec2(${io.input[0]},
                                   ${io.input[1]}));

        if (true
            % for u in io.possible_outputs:
            && actual != ${u}
            % endfor
           ) {
            vert_color = red;
            return;
        }

        % endfor

        vert_color = green;
    }

    [fragment shader]
    in vec4 vert_color;
    out vec4 frag_color;

    void main()
    {
        frag_color = vert_color;
    }

    [vertex data]
    vertex/float/2
    -1.0 -1.0
     1.0 -1.0
     1.0  1.0
    -1.0  1.0

    [test]
    draw arrays GL_TRIANGLE_FAN 0 4
    probe all rgba 0.0 1.0 0.0 1.0
"""))

# Test evaluation of constant unpack2x16 expressions.
const_unpack_2x16_template = Template(tw.dedent("""\
    [require]
    GL >= 3.0 es
    GLSL >= 3.00 es

    [vertex shader]
    const vec4 red = vec4(1, 0, 0, 1);
    const vec4 green = vec4(0, 1, 0, 1);

    in vec4 vertex;
    out vec4 vert_color;

    void main()
    {
        ${func.return_precision} vec2 actual;

        gl_Position = vertex;

        % for io in func.inouts:
        actual = ${func.name}(${io.input});

        if (true
            % for v in io.possible_outputs:
            && actual != vec2(${v[0]}, ${v[1]})
            % endfor
           ) {
            vert_color = red;
            return;
        }

        % endfor

        vert_color = green;
    }

    [fragment shader]
    in vec4 vert_color;
    out vec4 frag_color;

    void main()
    {
        frag_color = vert_color;
    }

    [vertex data]
    vertex/float/2
    -1.0 -1.0
     1.0 -1.0
     1.0  1.0
    -1.0  1.0

    [test]
    draw arrays GL_TRIANGLE_FAN 0 4
    probe all rgba 0.0 1.0 0.0 1.0
"""))

# Test execution of pack2x16 functions in the vertex shader.
vs_pack_2x16_template = Template(tw.dedent("""\
    [require]
    GL >= 3.0 es
    GLSL >= 3.00 es

    [vertex shader]
    const vec4 red = vec4(1, 0, 0, 1);
    const vec4 green = vec4(0, 1, 0, 1);

    uniform vec2 func_input;

    % for j in range(func.num_possible_outputs):
    uniform ${func.return_precision} uint expect${j};
    % endfor

    in vec4 vertex;
    out vec4 vert_color;

    void main()
    {
        gl_Position = vertex;
        ${func.return_precision} uint actual = ${func.name}(func_input);

        if (false
            % for j in range(func.num_possible_outputs):
            || actual == expect${j}
            % endfor
           ) {
           vert_color = green;
        } else {
            vert_color = red;
        }
    }

    [fragment shader]
    in vec4 vert_color;
    out vec4 frag_color;

    void main()
    {
        frag_color = vert_color;
    }

    [vertex data]
    vertex/float/2
    -1.0 -1.0
     1.0 -1.0
     1.0  1.0
    -1.0  1.0

    [test]
    % for io in func.inouts:
    uniform vec2 func_input ${io.input[0]} ${io.input[1]}
    % for j in range(func.num_possible_outputs):
    uniform uint expect${j} ${io.possible_outputs[j]}
    % endfor
    draw arrays GL_TRIANGLE_FAN 0 4
    probe all rgba 0.0 1.0 0.0 1.0

    % endfor
"""))

# Test execution of unpack2x16 functions in the vertex shader.
vs_unpack_2x16_template = Template(tw.dedent("""\
    [require]
    GL >= 3.0 es
    GLSL >= 3.00 es

    [vertex shader]
    const vec4 red = vec4(1, 0, 0, 1);
    const vec4 green = vec4(0, 1, 0, 1);

    uniform highp uint func_input;

    % for j in range(func.num_possible_outputs):
    uniform ${func.return_precision} vec2 expect${j};
    % endfor

    in vec4 vertex;
    out vec4 vert_color;

    void main()
    {
        gl_Position = vertex;

        ${func.return_precision} vec2 actual = ${func.name}(func_input);

        if (false
            % for j in range(func.num_possible_outputs):
            || actual == expect${j}
            % endfor
           ) {
            vert_color = green;
        } else {
            vert_color = red;
        }
    }

    [fragment shader]
    in vec4 vert_color;
    out vec4 frag_color;

    void main()
    {
        frag_color = vert_color;
    }

    [vertex data]
    vertex/float/2
    -1.0 -1.0
     1.0 -1.0
     1.0  1.0
    -1.0  1.0

    [test]
    % for io in func.inouts:
    uniform uint func_input ${io.input}
    % for j in range(func.num_possible_outputs):
    uniform vec2 expect${j} ${io.possible_outputs[j][0]} ${io.possible_outputs[j][1]}
    % endfor
    draw arrays GL_TRIANGLE_FAN 0 4
    probe all rgba 0.0 1.0 0.0 1.0

    % endfor
"""))


# Test execution of pack2x16 functions in the fragment shader.
fs_pack_2x16_template = Template(tw.dedent("""\
    [require]
    GL >= 3.0 es
    GLSL >= 3.00 es

    [vertex shader]
    in vec4 vertex;

    void main()
    {
        gl_Position = vertex;
    }

    [fragment shader]
    const vec4 red = vec4(1, 0, 0, 1);
    const vec4 green = vec4(0, 1, 0, 1);

    uniform vec2 func_input;

    % for i in range(func.num_possible_outputs):
    uniform ${func.return_precision} uint expect${i};
    % endfor

    out vec4 frag_color;

    void main()
    {
        ${func.return_precision} uint actual = ${func.name}(func_input);

        if (false
            % for i in range(func.num_possible_outputs):
            || actual == expect${i}
            % endfor
           ) {
            frag_color = green;
        } else {
            frag_color = red;
        }
    }

    [vertex data]
    vertex/float/2
    -1.0 -1.0
     1.0 -1.0
     1.0  1.0
    -1.0  1.0

    [test]
    % for io in func.inouts:
    uniform vec2 func_input ${io.input[0]} ${io.input[1]}
    % for i in range(func.num_possible_outputs):
    uniform uint expect${i} ${io.possible_outputs[i]}
    % endfor
    draw arrays GL_TRIANGLE_FAN 0 4
    probe all rgba 0.0 1.0 0.0 1.0

    % endfor
"""))

# Test execution of unpack2x16 functions in the fragment shader.
fs_unpack_2x16_template = Template(tw.dedent("""\
    [require]
    GL >= 3.0 es
    GLSL >= 3.00 es

    [vertex shader]
    in vec4 vertex;

    void main()
    {
        gl_Position = vertex;
    }

    [fragment shader]
    const vec4 red = vec4(1, 0, 0, 1);
    const vec4 green = vec4(0, 1, 0, 1);

    uniform highp uint func_input;

    % for i in range(func.num_possible_outputs):
    uniform ${func.return_precision} vec2 expect${i};
    % endfor

    out vec4 frag_color;

    void main()
    {
        ${func.return_precision} vec2 actual = ${func.name}(func_input);

        if (false
            % for i in range(func.num_possible_outputs):
            || actual == expect${i}
            % endfor
           ) {
            frag_color = green;
        } else {
            frag_color = red;
        }
    }

    [vertex data]
    vertex/float/2
    -1.0 -1.0
     1.0 -1.0
     1.0  1.0
    -1.0  1.0

    [test]
    % for io in func.inouts:
    uniform uint func_input ${io.input}
    % for i in range(func.num_possible_outputs):
    uniform vec2 expect${i} ${io.possible_outputs[i][0]} ${io.possible_outputs[i][1]}
    % endfor
    draw arrays GL_TRIANGLE_FAN 0 4
    probe all rgba 0.0 1.0 0.0 1.0

    % endfor
"""))

template_table = {
    ("const", "p", "2x16") : const_pack_2x16_template,
    ("const", "u", "2x16") : const_unpack_2x16_template,
    ("vs",    "p", "2x16") : vs_pack_2x16_template,
    ("vs",    "u", "2x16") : vs_unpack_2x16_template,
    ("fs",    "p", "2x16") : fs_pack_2x16_template,
    ("fs",    "u", "2x16") : fs_unpack_2x16_template,
}

# ----------------------------------------------------------------------------
# Function inputs and expected outputs
# ----------------------------------------------------------------------------

inout_tuple = namedtuple("inout_tuple", ("input", "possible_outputs"))

glsl_packing_exe_path = None

def set_glsl_packing_exe_path():
    global glsl_packing_exe_path

    if glsl_packing_exe_path is not None:
        return glsl_packing_exe_path

    if "PIGLIT_BUILD_DIR" in os.environ:
        piglit_build_dir = os.path.join(os.environ["PIGLIT_BUILD_DIR"])
    else:
        piglit_build_dir = os.path.join(os.path.dirname(__file__), os.pardir)

    glsl_packing_exe_path = os.path.join(piglit_build_dir,
                                           "bin",
                                           "glsl-packing")

    if not os.path.exists(glsl_packing_exe_path):
        print(("error: file {0!r} does not exist. maybe you forgot to " + \
               "build it or forgot to define environment var " + \
              "PIGLIT_BUILD_DIR.").format(glsl_packing_exe_path))
        sys.exit(1)

    return glsl_packing_exe_path

def glsl_literal(x):
    """Represent x as a string suitable as a GLSL literal.

    x must be an int or float.

    If x is an integer, then it is assumed to be unsigned 32-bit integer. For
    example, glsl_literal(1) returns "1u" and glsl_literal(-1) returns
    "0xffffffffu".
    """
    if type(x) == int:
        # The packing functions are concerned only with unsigned integers.
        return "{0}u".format(x % 2**32)
    elif type(x) == float:
        if math.isnan(x):
            # GLSL ES 3.00 and GLSL 4.10 do not require implementations to
            # support NaN, so we do not test it.
            assert(False)
        elif math.isinf(x):
            # GLSL ES 3.00 lacks a literal for infinity. However, a literal
            # value which is too large to stored as a float will be converted to
            # infintiy. From page 31 of the GLSL ES 3.00 spec:
            #
            #   If the value of the floating point number is too large (small)
            #   to be stored as a single precision value, it is converted to
            #   positive (negative) infinity.
            #
            return repr(math.copysign(1.0e256, x))
        else:
            return repr(x)
    else:
        assert(False)

class PackUnpackFunc(object):

    def __init__(self, name, dimension):
        assert(dimension in ["2x16", "4x8"])

        self.__name = name
        self.__dimension = dimension

    @property
    def name(self):
        return self.__name

    @property
    def dimension(self):
        return self.__dimension

    @property
    def num_possible_outputs(self):
        #print(self.inouts)
        #print(len(self.inouts[0].possible_outputs))
        return len(self.inouts[0].possible_outputs)

class Pack2x16Func(PackUnpackFunc):

    def __init__(self, name):
        PackUnpackFunc.__init__(self, name=name, dimension="2x16")
        self.__inouts = None

    @property
    def return_precision(self):
        return "highp"

    @property
    def inouts(self):
        if self.__inouts is not None:
            return self.__inouts

        def get_expected_output(x, y, func_opts):
            args = [glsl_packing_exe_path, self.name, repr(x), repr(y)]
            args.extend(func_opts)
            return glsl_literal(int(check_output(args)))

        self.__inouts = []

        for x in self.float_inputs:
                xl = glsl_literal(x)
                y = 0.0
                yl = glsl_literal(y)
                input = (xl, yl)

                possible_outputs = []
                for func_opts in self.func_opts_seq:
                    possible_outputs.append(get_expected_output(x, y, func_opts))

                self.__inouts.append(
                    inout_tuple(
                        input=(xl, yl),
                        possible_outputs=possible_outputs))

        return self.__inouts

class Unpack2x16Func(PackUnpackFunc):

    def __init__(self, name, return_precision):
        PackUnpackFunc.__init__(self, name=name, dimension="2x16")
        self.__return_precision = return_precision
        self.__inouts = None

    @property
    def return_precision(self):
        return self.__return_precision

    @property
    def func_opts_seq(self):
        return ((),
                ("flush_float16",),
                ("flush_float32",))

    @property
    def inouts(self):
        if self.__inouts is not None:
            return self.__inouts

        def get_expected_output(uint32, func_opts):
            args = [glsl_packing_exe_path, self.name, repr(uint32)]
            args.extend(func_opts)
            out = check_output(args)
            vec2 = out.strip().split()
            return (glsl_literal(float(vec2[0])),
                    glsl_literal(float(vec2[1])))

        self.__inouts = []

        for y in self.uint16_inputs:
            for x in self.uint16_inputs:
                    uint32 = (y << 16) | x

                    possible_outputs = []
                    for func_opts in self.func_opts_seq:
                        possible_outputs.append(get_expected_output(uint32, func_opts))

                    self.__inouts.append(
                        inout_tuple(
                            input=glsl_literal(uint32),
                            possible_outputs=possible_outputs))

        return self.__inouts

class PackSnorm2x16Func(Pack2x16Func):

    def __init__(self):
        Pack2x16Func.__init__(self, name="packSnorm2x16")
        self.__float_inputs = None

    @property
    def func_opts_seq(self):
        return (("round_to_even",),
                ("round_to_even", "flush_float16"),
                ("round_to_even", "flush_float32"),
                ("round_to_nearest",),
                ("round_to_nearest", "flush_float16"),
                ("round_to_nearest", "flush_float32"))
    @property
    def float_inputs(self):
        if self.__float_inputs is not None:
            return self.__float_inputs

        # The domain of packSnorm2x16 is [-inf, +inf]^2, yet the function clamps
        # its input to range [-1, +1]^2.
        #
        # Below are listed important classes in the function's domain, and test
        # inputs chosen from each class.
        #   - zero: -0.0, 0.0
        #   - near zero: -0.1, 0.1
        #   - just inside the clamp range: -0.9, 0.9
        #   - on the clamp boundary: -1.0, 1.0
        #   - just outside the clamp range: -1.1, 1.1
        #   - infinity: -inf, +inf
        #
        # We test -0.0 in order to stress the implementation's handling of zero.
        # The implementation should return a uint16_t that encodes -0.0; that is,
        # with the sign bit set.

        pos_floats = (0.0, 0.1, 0.9, 1.0, 1.1, float("+inf"))
        neg_floats = tuple(reversed(tuple((-x for x in pos_floats))))
        all_floats = pos_floats + neg_floats

        self.__float_inputs = all_floats
        return self.__float_inputs

class UnpackSnorm2x16Func(Unpack2x16Func):

    def __init__(self):
        Unpack2x16Func.__init__(
            self,
            name="unpackSnorm2x16",
            return_precision="highp")

    @property
    def uint16_inputs(self):
        return (0, 1, 2, 3,
                2**15 - 1,
                2**15,
                2**15 + 1,
                2**16 - 1, # max uint16
                )

class PackUnorm2x16Func(Pack2x16Func):

    def __init__(self):
        Pack2x16Func.__init__(self, name="packUnorm2x16")

    @property
    def func_opts_seq(self):
        return (("round_to_even",),
                ("round_to_even", "flush_float16"),
                ("round_to_even", "flush_float32"),
                ("round_to_nearest",),
                ("round_to_nearest", "flush_float16"),
                ("round_to_nearest", "flush_float32"))

    @property
    def float_inputs(self):

        # The domain of packUnorm2x16 is [-inf, +inf]^2, yet the function clamps
        # its input to range [0, 1]^2.
        #
        # Below are listed important classes in the function's domain, and test
        # inputs chosen from each class.
        #   - zero: -0.0, 0.0
        #   - just inside the clamp range: 0.1, 0.9
        #   - on the clamp boundary: 0.0, 1.0
        #   - just outside the clamp range: -0.1, 1.1
        #   - infinity: -inf, +inf
        #
        # We test -0.0 in order to stress the implementation's handling of zero.
        # The implementation should return a uint16_t that encodes +0.0; that is,
        # without the sign bit set.

        return (float("-inf"),
                -0.1, -0.0,
                +0.0, 0.1, 0.9, 1.0, 1.1,
                float("+inf"))

class UnpackUnorm2x16Func(Unpack2x16Func):

    def __init__(self):
        Unpack2x16Func.__init__(
            self,
            name="unpackUnorm2x16",
            return_precision="highp")

    @property
    def uint16_inputs(self):
        return (0, 1, 2, 3,
                2**15 - 1,
                2**15,
                2**15 + 1,
                2**16 - 1, # max uint16
                )

class PackHalf2x16Func(Pack2x16Func):

    def __init__(self):
        Pack2x16Func.__init__(self, name="packHalf2x16")
        self.__float_inputs = None

    @property
    def func_opts_seq(self):
        return ((),)

    @property
    def float_inputs(self):
        if self.__float_inputs is not None:
            return self.__float_inputs

        # The domain of packHalf2x16 is ([-inf, +inf] + {NaN})^2 and the function
        # does not clamp its input.
        #
        # For the min and max value of the two classes of float16 values,
        # subnormal and normalized, choose the following test inputs:
        #   - slightly below the minmax
        #   - the exact minmax value
        #   - slightly above the minmax
        #
        # We also test -0.0 and +0.0 in order to stress the implementation's
        # handling of zero.

        # Get info from `glsl-packing print-float16-info`.
        info = dict()
        output = check_output([glsl_packing_exe_path, "print-float16-info"])
        output = output.rstrip() # Remove trailing newline
        lines = output.split("\n")
        for line in lines:
            s = line.split(":")
            name = s[0]
            value = s[1]
            info[name] = float(value)

        subnormal_min = info["subnormal_min"]
        subnormal_max = info["subnormal_max"]
        normal_min = info["normal_min"]
        normal_max = info["normal_max"]
        min_step = info["min_step"]
        max_step = info["max_step"]

        pos_floats = (
            normal_min + 0.00 * min_step,
            normal_min + 0.25 * min_step,
            normal_min + 0.50 * min_step,
            normal_min + 0.75 * min_step,
            normal_min + 1.00 * min_step,
            normal_min + 1.25 * min_step,
            normal_min + 1.50 * min_step,
            normal_min + 1.75 * min_step,
            normal_min + 2.00 * min_step,
            )

        self.__float_inputs = pos_floats
        return self.__float_inputs

class UnpackHalf2x16Func(Unpack2x16Func):

    def __init__(self):
        Unpack2x16Func.__init__(
            self,
            name="unpackHalf2x16",
            return_precision="mediump")

        self.__uint16_inputs = None

    @property
    def uint16_inputs(self):
        if self.__uint16_inputs is not None:
            return self.__uint16_inputs

        # Encode a float16 with the given sign, exponent, and mantissa bits into
        # a uint16.
        def float16(s, e, m):
            return (s << 15) | (e << 10) | m

        # For each of the two classes of float16 values, subnormal and normalized,
        # below are listed the exponent and mantissa of the class's maximum and
        # minimum values and of some values slightly inside the bounds.
        bounds = (
            ( 0,    0), # zero

            ( 0,    1), # subnormal_min
            ( 0,    2), # subnormal_min + subnormal_step

            ( 0, 1022), # subnormal_max - subnormal_step
            ( 0, 1023), # subnormal_max

            ( 1,    0), # normal_min
            ( 1,    1), # normal_min + normal_min_step

            (30, 1022), # normal_max - normal_max_step
            (30, 1023), # normal_max

            (31,    0), # inf
        )

        pos_float16 = tuple(float16(0, e, m) for (e, m) in bounds)
        neg_float16 = tuple(float16(1, e, m) for (e, m) in reversed(bounds))
        all_float16 = neg_float16 + pos_float16

        self.__uint16_inputs = all_float16
        return self.__uint16_inputs

class Test(object):

    def __init__(self, func, execution_stage):
        assert(isinstance(func, PackUnpackFunc))
        assert(execution_stage in ("const", "vs", "fs"))

        self.__template = template_table[(execution_stage,
                                          func.name[0],
                                          func.dimension)]
        self.__func = func
        self.__filename = os.path.join(
                            "spec",
                            "glsl-es-3.00",
                            "execution",
                            "built-in-functions",
                            "{0}-{1}.shader_test"\
                            .format(execution_stage, func.name))

    @property
    def filename(self):
        return self.__filename

    def write_file(self):
        dirname = os.path.dirname(self.filename)
        if not os.path.exists(dirname):
            os.makedirs(dirname)

        with open(self.filename, "w") as buffer:
            ctx = mako.runtime.Context(buffer, func=self.__func)
            self.__template.render_context(ctx)

def all_tests_iter():
    # Format these master lists as one item per line. This allows one to
    # easily debug by commenting out individual items.

    func_classes = (
        PackHalf2x16Func,
        )

    execution_stages = (
        "vs",
        "fs",
        )

    for stage in execution_stages:
        for func in func_classes:
            yield Test(func(), stage)

def main():
    set_glsl_packing_exe_path()

    parser = optparse.OptionParser(
                description="Generate shader tests that test the built-in " + \
                            "packing functions",
                usage="usage: %prog [-h] [--names-only]")
    parser.add_option(
        '--names-only',
        dest='names_only',
        action='store_true',
        help="Don't output files, just generate a list of filenames to stdout")

    (options, args) = parser.parse_args()

    if len(args) != 0:
        # User gave extra args.
        parser.print_help()
        sys.exit(1)

    for test in all_tests_iter():
        print(test.filename)
        sys.stdout.flush()

        if not options.names_only:
            test.write_file()

if __name__ == '__main__':
    main()
