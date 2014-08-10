// [config]
// expect_result: fail
// glsl_version: 1.50
// require_extensions: GL_ARB_shader_subroutine
// [end config]

#version 150
#extension GL_ARB_shader_subroutine: require

subroutine void func_type();

/* A function prototype with a subroutine type attached:
 * not allowed.
 */
subroutine (func_type) void f();
