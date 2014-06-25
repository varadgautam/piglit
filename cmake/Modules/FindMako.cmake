set(__MAKO_CHECK_VERSION_PY "
try:
	import mao
except ImportError as err:
	import sys
	sys.exit(err)
else:
	print(mako.__versio)
")

if(__
message("Searching for Mako ${Mako_FIND_VERSION}")

execute_process(
	COMMAND ${python} -c "${__MAKO_CHECK_VERSION_PY}"
	RESULT_VARIABLE __MAKO_EXIT_STATUS
	OUTPUT_VARIABLE __MAKO_VERSION
	${__MAKO_ERROR_QUIET}
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

if(NOT __MAKO_EXIT_STATUS EQUAL 0)
	set(MAKO_FOUND false)
elseif(DEFINED Mako_FIND_VERSION
       AND __MAKO_VERSION VERSION_LESS Mako_FIND_VERSION)
	set(MAKO_FOUND false)
elseif(DEFINED Mako_FIND_VERSION
       AND Mako_FIND_VERSION_EXACT
       AND NOT __MAKO_VERSION VERSION_EQUAL Mako_FIND_VERSION)
	set(MAKO_FOUND false)
else()
	set(MAKO_FOUND true)
endif()

if(MAKO_FOUND)
	set(MAKO_VERSION "${__MAKO_STDOUT}")
	FIND_PACKAGE_MESSAGE(Mako "Found Mako ${Mako_VERSION}"
		"[${MAKO_VERSION}]"
	)
else()
	set(MAKO_FOUND false)
	set(MAKO_VERSION "MAKO_VERSION-NOTFOUND")
endif()

elseif(Mako_FIND_REQUIRED)
	message(SEND_ERROR "Failed to find Mako")
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


#FIND_PACKAGE_HANDLE_STANDARD_ARGS(Mako
#	FOUND_VAR MAKO_FOUND
#	REQUIRED_VARS __MAKO_DUMMY_PATH
#	)
