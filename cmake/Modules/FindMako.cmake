include(FindPackageHandleStandardArgs)
#set(mako_required_version "0.7")

set(__MAKO_CHECK_VERSION_PY "
try:
	import mao
except ImportError as err:
	import sys
	sys.exit(err)
else:
	print(mako.__versio)
")

execute_process(
	COMMAND ${python} -c "${mako_check_version_py}"
	#try:
	#	import mako
	#except ImportError as err:
	#	sys.exit(err)
	#else:
	#	print(mako.__versio)
	#"
	OUTPUT_VARIABLE __MAKO_STDOUT
	RESULT_VARIABLE __MAKO_EXIT_STATUS
)

#if(__MAKO_EXIT_STATUS EQUAL 0)
#	if(mako_actual_version VERSION_LESS mako_required_version)
#		message(SEND_ERROR "found mako-${mako_actual_version}, but mako-${make_required_version} is required")
#	else()
#		message(STATUS "found make-${mako_actual_version}")
#	endif()
#else()
#	#message(SEND_ERROR "failed to check mako version due to Python errors
#	message(SEND_ERROR "Python errors:\n${mako_stderr}")
#endif()
if(__MAKO_EXIT_STATUS EQUAL 0)
	set(__MAKO_FOUND true)
	set(__MAKO_VERSION "${__MAKO_STDOUT}")
else()
	set(__MAKO_FOUND false)
	set(__MAKO_VERSION "0.8")
	#if(NOT __MAKO_EXIT_STATUS EQUAL 0)
	#	message(SEND_ERROR "failed to find Python errors:\n${mako_stderr}")
endif()

# This module defines the following variables:
#
#   OPENCL_FOUND
#       True if OpenCL is installed.
#
#   OPENCL_INCLUDE_PATH
#
#   OPENCL_opencl_LIBRARY
#       Path to OpenCL's library.

FIND_PACKAGE_HANDLE_STANDARD_ARGS(Mako
	#VERSION_VAR MAKO_VERSION
	REQUIRED_VARS __MAKO_VERSION
)
