# Copyright 2012 Intel Corporation
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# - Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# - Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#
# Set Android toolchain specifics

SET(CMAKE_SYSTEM_NAME Android)
SET(CMAKE_SYSTEM_VERSION 1)
SET(UNIX True)
SET(ANDROID True)

# check build/envsetup.sh was sourced
IF(NOT DEFINED ENV{ANDROID_BUILD_TOP})
    message(FATAL_ERROR
    "
    build/envsetup.sh is not sourced.
     Go to your Android top directory and choose your platform:
     . build/envsetup.sh
     lunch
    "
    )
ENDIF()

# the Compiler file is parsed each time a compiler detection is performed.
# this works around running get_build_var each time this file is parsed.
IF($ENV{PLATFORM_ANDROID})
    return()
ENDIF()


SET(ENV{PLATFORM_ANDROID} 1)

SET(PIGLIT_HAS_ANDROID TRUE)
ADD_DEFINITIONS(-DPIGLIT_HAS_ANDROID)


# needed by get_build_var
SET(ENV{CALLED_FROM_SETUP} true)
SET(ENV{BUILD_SYSTEM} build/core)

MACRO(get_build_var arg1)
    IF("${ARGN}" STREQUAL "")
       SET( _output ${arg1})
    ELSE()
       SET( _output ${ARGN})
    ENDIF()
    EXECUTE_PROCESS(COMMAND make --no-print-directory -C $ENV{ANDROID_BUILD_TOP} -f build/core/config.mk dumpvar-${arg1}
        OUTPUT_VARIABLE ${_output}
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
ENDMACRO()

get_build_var(TARGET_CC   CMAKE_C_COMPILER)
get_build_var(TARGET_CXX  CMAKE_CXX_COMPILER)
SET(CMAKE_C_COMPILER   $ENV{ANDROID_BUILD_TOP}/${CMAKE_C_COMPILER} CACHE STRING "")
SET(CMAKE_CXX_COMPILER $ENV{ANDROID_BUILD_TOP}/${CMAKE_CXX_COMPILER} CACHE STRING "")

get_build_var(TARGET_ARCH)
get_build_var(TARGET_GLOBAL_CFLAGS)
get_build_var(TARGET_GLOBAL_LDFLAGS)
get_build_var(TARGET_DEFAULT_SYSTEM_SHARED_LIBRARIES)
STRING(REGEX REPLACE "lib" "-l" TARGET_DEFAULT_SYSTEM_SHARED_LIBRARIES ${TARGET_DEFAULT_SYSTEM_SHARED_LIBRARIES})

SET(__NDK_API_LEVEL 14)
SET(__NDK_VERSION   r8)
SET(__NDK_SYSROOT   $ENV{ANDROID_BUILD_TOP}/prebuilts/ndk/android-ndk-${__NDK_VERSION}/platforms/android-${__NDK_API_LEVEL}/arch-${TARGET_ARCH})
IF(NOT IS_DIRECTORY ${__NDK_SYSROOT})
    MESSAGE(FATAL_ERROR "NDK sysroot not found:" ${__NDK_SYSROOT})
ENDIF()

SET(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${TARGET_GLOBAL_CFLAGS} -I$ENV{ANDROID_BUILD_TOP} --sysroot=${__NDK_SYSROOT} -Wno-error=return-type " CACHE STRING "c flags")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TARGET_GLOBAL_CFLAGS} -I$ENV{ANDROID_BUILD_TOP} --sysroot=${__NDK_SYSROOT} -Wno-error=return-type " CACHE STRING "c++ flags")
SET(CMAKE_LD_FLAGS  "${CMAKE_LD_FLAGS}  ${TARGET_GLOBAL_LDFLAGS} ${TARGET_DEFAULT_SYSTEM_SHARED_LIBRARIES} --sysroot=${__NDK_SYSROOT}" CACHE STRING "linker flags")

# Search only inside Android directories, not the remainder of the host filesystem
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
