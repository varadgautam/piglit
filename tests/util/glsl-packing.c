/*
 * Copyright © 2012 Intel Corporation
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* TODO: Reject round options for pack/unpackHalf.
 * TODO: Discuss choices in packHalf.
 */

#define PRIf32_PRECISION "24"
#define PRIf32 "%." PRIf32_PRECISION "f"

static const char *help_text =
	"NAME\n"
	"    glsl-packing - Print the result of a GLSL packing function\n"
	"\n"
	"SYNOPSIS\n"
	"    glsl-packing PACK_FUNC X Y [FUNC_OPTS]\n"
	"    glsl-packing UNPACK_FUNC U [FUNC_OPTS]\n"
	"    glsl-packing print-float16-info\n"
	"\n"
	"COMMANDS\n"
	"    All floats are printed with the printf specifier " PRIf32 ".\n"
	"\n"
	"    glsl-packing PACK_FUNC X Y [FUNC_OPTS]\n"
	"        Print the result of calling PACK_FUNC on vec2(X, Y).\n"
	"\n"
	"        PACK_FUNC must be one of:\n"
	"            packSnorm2x16\n"
	"            packUnorm2x16\n"
	"            packHalf2x16\n"
	"\n"
	"        X and Y must be floating point numbers in a format consumable\n"
	"        by strof(3).\n"
	"\n"
	"    glsl-packing UNPACK_FUNC U [FUNC_OPTS]\n"
	"        Print the result of calling UNPACK_FUNC on uint(U).\n"
	"\n"
	"        UNPACK_FUNC must be one of:\n"
	"            unpackSnorm2x16\n"
	"            unpackUnorm2x16\n"
	"            unpackHalf2x16\n"
	"\n"
	"        U must be an unsigned integer in a format consumable by scanf(3).\n"
	"\n"
	"    glsl-packing print-float16-info\n"
	"        Print the following special values of IEEE 754 16-bit floats:\n"
	"            subnormal_min\n"
	"            subnormal_max\n"
	"            normal_min\n"
	"            normal_max\n"
	"            min_step\n"
	"            max_step\n"
	"\n"
	"FUNC_OPTS\n"
	"    flush_float16\n"
	"    flush_float32\n"
	"        All PACK_FUNC and UNPACK_FUNC commands accept the flush options.\n"
	"\n"
	"        The GLSL ES 3.00 and GLSL 4.10 specs allows implementations to truncate\n"
	"        subnormal floats to zero. From section 4.5.1 \"Range and Precision\"\n"
	"        of the two specs:\n"
	"            Any subnormal (denormalized) value input into a shader or\n"
	"            potentially generated by any operation in a shader can be\n"
	"            flushed to 0.\n"
	"\n"
	"        If flush_float32 is specified, then glsl-packing will simulate the behavior\n"
	"        of a GLSL implementation that flushes subnormal 32-bit floating-point values\n"
	"        to 0. Likewise if flush_float16 is enabled.\n"
	"\n"
	"        Enabling flush_float16 implicitly enables flush_float32.\n"
	"\n"
	"    round_to_nearest\n"
	"    round_to_even\n"
	"        All PACK_FUNC and UNPACK_FUNC commands except pack/unpackHalf2x16 accept\n"
	"        the rounding option. At most one rounding option may be specified.\n"
	"\n"
	"        For some packing functions, the GLSL ES 3.00 specification's\n"
	"        definition of the function's behavior involves the `round()`\n"
	"        function, whose behavior at 0.5 is not specified. From section \n"
	"        8.3 of the spec:\n"
	"            The fraction 0.5 will round in a direction chosen by the\n"
	"            implementation, presumably the direction that is fastest.\n"
	"\n"
	"        If a rounding option is given, it determines the rounding behavior at 0.5.\n"
	;

struct func_options;

typedef uint16_t
(*pack_1x16_func_t)(float, struct func_options*);

typedef void
(*unpack_1x16_func_t)(uint16_t, float*, struct func_options*);

typedef float
(*round_func_t)(float);

struct func_options {
	round_func_t round;
	bool flush_float16;
	bool flush_float32;
};

/**
 * Flush subnormal 32-bit floating point numbers to ±0.0, preserving the
 * sign bit.
 */
static float
flush_float32(float f)
{
	if (fpclassify(f) == FP_SUBNORMAL) {
		return copysign(0.0, f);
	} else {
		return f;
	}
}

/**
 * Flush subnormal 16-bit floating point numbers to to ±0.0, preserving the
 * sign bit.
 */
static uint16_t
flush_float16(uint16_t u)
{
	if (!(u & 0x7c00)) {
		return u & 0x8000;
	} else {
		return u;
	}
}

static float
clampf(float x, float min, float max)
{
	if (x < min)
		return min;
	else if (x > max)
		return max;
	else
		return x;
}

static float
round_to_nearest(float x)
{
	float i;
	float f;

	f = modff(x, &i);

	if (fabs(f) < 0.5f)
		return i;
	else
		return i + copysignf(1.0f, x);
}

static float
round_to_even(float x)
{
	float i;
	float f;

	f = modff(x, &i);

	if (fabs(f) < 0.5f)
		return i;
	else if (fabs(f) == 0.5f)
		return i + fmodf(i, copysignf(2.0f, x));
	else
		return i + copysignf(1.0f, x);
}

static void
func_options_init(struct func_options *func_opts)
{
	memset(func_opts, 0, sizeof(*func_opts));
}

static uint32_t
pack_2x16(pack_1x16_func_t pack_1x16,
	  float x, float y,
	  struct func_options *func_opts)
{
	uint16_t ux;
	uint16_t uy;

	if (func_opts->flush_float32) {
		x = flush_float32(x);
		y = flush_float32(y);
	}

	ux = pack_1x16(x, func_opts);
	uy = pack_1x16(y, func_opts);

	if (func_opts->flush_float16) {
		ux = flush_float16(ux);
		uy = flush_float16(uy);
	}

	return ((uint32_t) uy << 16) | ux;
}

static void
unpack_2x16(unpack_1x16_func_t unpack_1x16,
            uint32_t u,
            float *x, float *y,
            struct func_options *func_opts)

{
        uint16_t ux = u & 0xffff;
        uint16_t uy = u >> 16;

        if (func_opts->flush_float16) {
                ux = flush_float16(ux);
                uy = flush_float16(uy);
        }

        unpack_1x16(ux, x, func_opts);
        unpack_1x16(uy, y, func_opts);

        if (func_opts->flush_float32) {
                *x = flush_float32(*x);
                *y = flush_float32(*y);
        }
}

static uint16_t
pack_snorm_1x16(float x, struct func_options *func_opts)
{
	return (uint16_t) func_opts->round(clampf(x, -1.0f, +1.0f) * 32767.0f);
}

static void
unpack_snorm_1x16(uint16_t u, float *f, struct func_options *func_opts)
{
        *f = clampf((int16_t) u / 32767.0f, -1.0f, +1.0f);
}

static uint16_t
pack_unorm_1x16(float x, struct func_options *func_opts)
{
	return (uint16_t) func_opts->round(clampf(x, 0.0f, 1.0f) * 65535.0f);
}

static void
unpack_unorm_1x16(uint16_t u, float *f, struct func_options *func_opts)
{
        *f = (float) u / 65535.0f;
}

static uint16_t
pack_half_1x16(float x, struct func_options *func_opts)
{
	/* The bit layout of a float16 is:
	 *   sign: 15
	 *   exponent: 10:14
	 *   mantissa: 0:9
	 *
	 * The sign, exponent, and mantissa of a float16 determine its value
	 * thus:
	 *
	 *  if e = 0 and m = 0, then zero:       (-1)^s * 0
	 *  if e = 0 and m != 0, then subnormal: (-1)^s * 2^(e - 14) * m / 2^10
	 *  if 0 < e < 31, then normal:          (-1)^s * 2^(e - 15) * (1 + m / 2^10)
	 *  if e = 31 and m = 0, then inf:       (-1)^s * inf
	 *  if e = 31 and m != 0, then nan
	 *
	 *  where 0 <= m < 2^10 .
	 */

	/* Calculate the resultant float16's sign, exponent, and mantissa
	 * bits.
	 */
	const int s = (copysignf(1.0f, x) < 0) ? 1 : 0;
	int e;
	int m;

	switch (fpclassify(x)) {
	case FP_NAN:
		return 0xffffu;
	case FP_INFINITE:
		e = 31;
		m = 0;
		break;
	case FP_SUBNORMAL:
	case FP_ZERO:
		e = 0;
		m = 0;
		break;
	case FP_NORMAL: {
		/* Recall that the form of subnormal and normal float16 values
		 * are
		 *
		 *   subnormal: 2^(e - 14) * m / 2^10 where e = 0
		 *   normal: 2^(e - 15) * (1 + m / 2^10) where 1 <= e <= 30
		 *
		 * where 0 <= m < 2^10. Therefore some key boundary values of
		 * float16 are:
		 *
		 *   min_subnormal = 2^(-14) * 1 / 2^10
		 *   max_subnormal = 2^(-14) * 1023 / 2^10
		 *   min_normal    = 2^(1-15) * (1 + 0 / 2^10)
		 *   max_normal    = 2^(30 - 15) * (1 + 1023 / 2^10)
		 *
		 * Representing the same boundary values in the form returned
		 * by frexpf(), 2^e * f where 0.5 <= f < 1, gives
		 *
		 *   min_subnormal = 2^(-14) * 1 / 2^10
		 *                 = 2^(-23) * 1 / 2
		 *                 = 2^(-23) * 0.5
		 *
		 *   max_subnormal = 2^(-14) * 1023 / 2^10
		 *                 = 2^(-14) * 0.9990234375
		 *
		 *   min_normal    = 2^(1 - 15) * (1 + 0 / 2^10)
		 *                 = 2^(-14)
		 *                 = 2^(-13) * 0.5
		 *
		 *   max_normal    = 2^(30 - 15) * (1 + 1023 / 2^10)
		 *                 = 2^15 * (2^10 + 1023) / 2^10
		 *                 = 2^16 * (2^10 + 1023) / 2^11
		 *                 = 2^16 * 0.99951171875
		 */

		/* Represent the absolute value of the input in form 2^E * F
		 * where 0.5 <= F < 1.
		 */
		int E;
		float F;

		F = frexpf(fabs(x), &E);

		/* Now calculate the results's exponent and mantissa by
		 * comparing the input against the boundary values above.
		 */
		if (E == -23 && F < 0.5f) {
			/* The float32 input is too small to be represented as
			 * a float16. The result is zero.
			 */
			e = 0;
			m = 0;
		} else if (E < -13 || (E == -13 && F < 0.5f)) {
			/* The resultant float16 value is subnormal. Let's
			 * calculate m.
			 *
			 *   2^E * F = 2^(-14) * m / 2^10
			 *         m = 2^(E + 24) * F
			 */
			e = 0;
			m = powf(2, E + 24) * F;
		} else if (E < 16 || (E == 16 && F <= 0.99951171875f)) {
			/* The resultant float16 is normal. Let's calculate
			 * e and m.
			 *
			 *   2^E * F = 2^(e - 15) * (1 + m / 2^10)          (1)
			 *           = 2^(e - 15) * (2^10 + m) / 2^10       (2)
			 *           = 2^(e - 14) * (2^10 + m) / 2^11       (3)
			 *
			 * Substituting
			 *
			 *   e1 := E                                        (4)
			 *   f1 := F                                        (5)
			 *   e2 := e - 14                                   (6)
			 *   f2 := (2^10 + m) / 2^11                        (7)
			 *
			 * transforms the equation to
			 *
			 *   2^e1 * f1 = 2^e2 * f2                          (8)
			 *
			 * By definition, f1 lies in the range [0.5, 1). By
			 * equation 7, f2 lies there also. This observation
			 * combined with equation 8 implies f1 = f2, which in
			 * turn implies e1 = e2. Therefore
			 *
			 *   e = E + 14
			 *   m = 2^11 * F - 2^10
			 */
			e = E + 14;
			m = powf(2, 11) * F - powf(2, 10);
		} else {
			/* The float32 input is too large to represent as a
			 * float16. The result is infinite.
			 */
			e = 31;
			m = 0;
		}
		break;
	}
	default:
		assert(0);
		break;
	}

	assert(s == 0 || s == 1);
	assert(0 <= e && e <= 31);
	assert(0 <= m && m <= 1023);

	return (s << 15) | (e << 10) | m;
}

static void
unpack_half_1x16(uint16_t u, float *f, struct func_options *func_opts)
{
	/* The bit layout of a float16 is:
	 *   sign: 15
	 *   exponent: 10:14
	 *   mantissa: 0:9
	 *
	 * The sign, exponent, and mantissa of a float16 determine its value
	 * thus:
	 *
	 *  if e = 0 and m = 0, then zero:       (-1)^s * 0
	 *  if e = 0 and m != 0, then subnormal: (-1)^s * 2^(e - 14) * m / 2^10
	 *  if 0 < e < 31, then normal:          (-1)^s * 2^(e - 15) * (1 + m / 2^10)
	 *  if e = 31 and m = 0, then inf:       (-1)^s * inf
	 *  if e = 31 and m != 0, then nan
	 *
	 *  where 0 <= m < 2^10 .
	 */

	int s = (u >> 15) & 0x1;
	int e = (u >> 10) & 0x1f;
	int m = u & 0x3ff;

	float sign = s ? -1 : 1;

	if (e == 0) {
		*f = sign * pow(2, -24) * m;
	} else if (1 <= e && e <= 30) {
		*f = sign * pow(2, e - 15) * (1.0 + m / 1024.0);
	} else if (e == 31 && m == 0) {
		*f = sign * INFINITY;
	} else if (e == 31 && m != 0) {
		*f = NAN;
	} else {
		assert(0);
	}
}

struct round_func_key {
	const char *name;
	round_func_t func;
};

static struct round_func_key round_func_list[] = {
	{"round_to_even",           round_to_even},
	{"round_to_nearest",        round_to_nearest},
	{0,                         0},
};

struct pack_func_2x16_key {
	const char *name;
	pack_1x16_func_t func;
};

static struct pack_func_2x16_key pack_func_2x16_list[] = {
	{"packSnorm2x16",           pack_snorm_1x16},
	{"packUnorm2x16",           pack_unorm_1x16},
	{"packHalf2x16",            pack_half_1x16},
	{0,                         0},
};

struct unpack_2x16_func_key {
	const char *name;
	unpack_1x16_func_t func;
};

static struct unpack_2x16_func_key unpack_2x16_func_list[] = {
	{"unpackSnorm2x16",         unpack_snorm_1x16},
	{"unpackUnorm2x16",         unpack_unorm_1x16},
	{"unpackHalf2x16",          unpack_half_1x16},
	{0,                         0},
};

struct pack_2x16_args {
	pack_1x16_func_t pack_func;
	float x;
	float y;
	struct func_options func_opts;
};

struct unpack_2x16_args {
	unpack_1x16_func_t unpack_func;
	uint32_t u;
	struct func_options func_opts;
};

struct args {
	enum {
		ARG_HELP,
		ARG_PACK_2x16,
		ARG_UNPACK_2x16,
		ARG_PRINT_FLOAT16_INFO,
	} tag;

	union {
		struct pack_2x16_args pack_2x16;
		struct unpack_2x16_args unpack_2x16;
	};
};

static void
usage_error(const char *fmt, ...)
{
	printf("usage error");
	if (fmt && strcmp(fmt, "") != 0) {
		va_list va;
		va_start(va, fmt);
		printf(": ");
		vprintf(fmt, va);
		va_end(va);
	}
	printf("\n");
	printf("for help, call `glsl-packing -h`\n");
	exit(1);
}

static void
parse_func_opts(struct func_options *func_opts,
                const char *command_name,
                int argc, char **argv)
{
	int i;

	assert(strcmp(command_name, "print-float16-info") != 0);

	func_options_init(func_opts);

	for (i = 0; i < argc; ++i) {
		const char *arg = argv[i];

		if (strcmp(arg, "flush_float16") == 0) {
			/* flush_float16 implies flush_float32. */
			func_opts->flush_float16 = true;
			func_opts->flush_float32 = true;
			continue;
		} else if (strcmp(arg, "flush_float32") == 0) {
			func_opts->flush_float32 = true;
			continue;
		} else {
			/* Assume the arg is a rounding options. */
			round_func_t round_func = NULL;
			int i;

			for (i = 0; round_func_list[i].name != NULL; ++i) {
				if (strcmp(arg, round_func_list[i].name) == 0) {
					round_func = round_func_list[i].func;
					break;
				}
			}

			if (round_func) {
				if (func_opts->round) {
					usage_error("multiple rounding "
					            "options were given");
				}

				func_opts->round = round_func;
				continue;
			}
		}

		usage_error("unrecognized option: %s", arg);
	}

	if (func_opts->round != NULL
	    && (strncmp(command_name, "packHalf", 8) == 0 ||
	        strncmp(command_name, "unpackHalf", 10) == 0)) {
	   usage_error("Half functions do not accept any rounding options");
	}

	if (!func_opts->round ) {
		/* default */
		func_opts->round = round_to_even;
	}
}

static bool
parse_pack_2x16_args(struct pack_2x16_args *args, int argc, char **argv)
{
	int i;
	char *endptr;
	const char *func_name = NULL;

	memset(args, 0, sizeof(*args));

	/* Parse function name. */
	if (argc < 2)
		return false;

	for (i = 0; (func_name = pack_func_2x16_list[i].name) != NULL; ++i) {
		if (strcmp(argv[1], func_name) == 0) {
			args->pack_func = pack_func_2x16_list[i].func;
			break;
		}
	}

	if (args->pack_func == NULL) {
		/* Failed to parse function name. */
		return false;
	}

	/* Parse inputs to packing function. */
	if (argc < 4)
		usage_error("not enough inputs for pack function");

	args->x = strtof(argv[2], &endptr);
	if (endptr == argv[2])
		usage_error("unable parse input to pack function: %s", argv[2]);

	args->y = strtof(argv[3], &endptr);
	if (endptr == argv[3])
		usage_error("unable parse input to pack function: %s", argv[3]);

	parse_func_opts(&args->func_opts, func_name, argc - 4, argv + 4);
	return true;
}

static bool
parse_unpack_2x16_args(struct unpack_2x16_args *args, int argc, char **argv)
{
	int i;
        const char *func_name = NULL;

	memset(args, 0, sizeof(*args));

	/* Parse function name. */
	if (argc < 2)
		return false;

	for (i = 0; (func_name = unpack_2x16_func_list[i].name) != NULL; ++i) {
		if (strcmp(argv[1], func_name) == 0) {
			args->unpack_func = unpack_2x16_func_list[i].func;
			break;
		}
	}

	if (args->unpack_func == NULL) {
		/* Failed to parse function name. */
		return false;
	}

	/* Parse inputs to unpacking function. */
	if (argc < 3)
		usage_error("not enough inputs for unpack function");
	if (sscanf(argv[2],"%u", &args->u) != 1)
		usage_error("unable to parse input to unpack function: %s", argv[2]);

	parse_func_opts(&args->func_opts, func_name, argc - 3, argv + 3);
	return true;
}

static void
parse_args(struct args *args, int argc, char **argv)
{
	memset(args, 0, sizeof(*args));

	if (argc < 2) {
		usage_error("no command was given");
	}

	if (strcmp(argv[1], "-h") == 0 ||
	    strcmp(argv[1], "--help") == 0) {
	   args->tag = ARG_HELP;
	   return;
	}

	if (parse_pack_2x16_args(&args->pack_2x16, argc, argv)) {
		args->tag = ARG_PACK_2x16;
		return;
	}

	if (parse_unpack_2x16_args(&args->unpack_2x16, argc, argv)) {
		args->tag = ARG_UNPACK_2x16;
		return;
	}

	if (strcmp(argv[1], "print-float16-info") == 0) {
		if (argc > 2) {
			usage_error("print-float16-info takes no args");
		}
		args->tag = ARG_PRINT_FLOAT16_INFO;
		return;
	};

	usage_error("unrecognized command: %s", argv[1]);
}

static void
cmd_pack_2x16(struct pack_2x16_args *args)
{
	uint32_t u = pack_2x16(args->pack_func,
	                       args->x, args->y,
	                       &args->func_opts);
	printf("%u\n", u);
}

static void
cmd_unpack_2x16(struct unpack_2x16_args *args)
{
	float x;
	float y;

	unpack_2x16(args->unpack_func,
	            args->u, &x, &y,
	            &args->func_opts);
	printf(PRIf32 " " PRIf32 "\n", x, y);
}

static void
print_float16_value(const char *name, int e, int m)
{
	struct func_options func_opts;
	int s = 0;
	float f;

	func_options_init(&func_opts);
	unpack_half_1x16((s << 15) | (e << 10) | m, &f, &func_opts);
	printf("%s: " PRIf32 "\n", name, f);

}

static void
print_float16_step(const char *name, int exp)
{
	printf("%s: " PRIf32 "\n", name, powf(2, exp));
}

static void
cmd_print_float16_info(void)
{
	print_float16_value("subnormal_min", 0, 1);
	print_float16_value("subnormal_max", 0, 1023);
	print_float16_value("normal_min", 1, 0);
	print_float16_value("normal_max", 30, 1023);
	print_float16_step("min_step", -14 - 10);
	print_float16_step("max_step", 15 - 10);
}

static void
exec_args(struct args *args)
{
	switch (args->tag) {
	case ARG_HELP:
		printf("%s", help_text);
		break;
	case ARG_PACK_2x16:
		cmd_pack_2x16(&args->pack_2x16);
		break;
	case ARG_UNPACK_2x16:
		cmd_unpack_2x16(&args->unpack_2x16);
		break;
	case ARG_PRINT_FLOAT16_INFO:
		cmd_print_float16_info();
		break;
	default:
		assert(0);
		break;
	}
}

int
main(int argc, char **argv)
{
	struct args args;

	parse_args(&args, argc, argv);
	exec_args(&args);

	return 0;
}
