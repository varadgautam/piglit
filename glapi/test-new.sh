#!/bin/bash

python2.7 ./parse_glspec.py gl.tm gl.spec enum.spec enumext.spec GLES1/gl.h GLES1/glext.h GLES3/gl3.h GLES2/gl2ext.h test.json
