#!/bin/bash

export PIGLIT_PLATFORM=x11_egl

for x in $(find generated_tests/spec/glsl-es-3.00 -type f | sort); do
    echo $x
    bin/shader_runner_gles3 $x -auto
    echo
done
