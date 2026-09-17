# Pin CMAKE_OSX_SYSROOT to the SDK that xcrun currently reports.
# Include BEFORE project() in child CMakeLists (and from the superbuild after
# SK_PLATFORM is known). After an Xcode upgrade the cached path
# (e.g. MacOSX26.5.sdk) often disappears; Apple Clang then fails with
# 'cstdlib' / 'vector' / 'stdio.h' not found.

if(NOT APPLE AND NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
	return()
endif()

# Emscripten ships its own sysroot. Never apply the Mac SDK to a wasm child.
if(EMSCRIPTEN)
	return()
endif()
if(DEFINED SK_PLATFORM AND SK_PLATFORM STREQUAL "wasm")
	return()
endif()

execute_process(
	COMMAND xcrun --sdk macosx --show-sdk-path
	OUTPUT_VARIABLE _SK_OSX_SDK
	OUTPUT_STRIP_TRAILING_WHITESPACE
	RESULT_VARIABLE _SK_OSX_SDK_RC
	ERROR_QUIET
)
if(NOT _SK_OSX_SDK_RC EQUAL 0 OR _SK_OSX_SDK STREQUAL "" OR NOT EXISTS "${_SK_OSX_SDK}")
	return()
endif()

set(_SK_NEED_SDK FALSE)
if(NOT CMAKE_OSX_SYSROOT)
	set(_SK_NEED_SDK TRUE)
elseif(NOT EXISTS "${CMAKE_OSX_SYSROOT}")
	set(_SK_NEED_SDK TRUE)
endif()

if(_SK_NEED_SDK)
	set(CMAKE_OSX_SYSROOT "${_SK_OSX_SDK}" CACHE PATH "macOS SDK" FORCE)
	set(CMAKE_OSX_SYSROOT "${_SK_OSX_SDK}")
	message(STATUS "skeepto-engine: CMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT}")
endif()
