# Resolve SK_EMSCRIPTEN_TOOLCHAIN without a machine-specific path.
# Order: cache/arg, CMAKE_TOOLCHAIN_FILE, EMSDK, emcc on PATH,
# $HOME/emsdk, $HOME/Projects/emsdk, %USERPROFILE% variants, sibling emsdk next to this repo.

if(NOT SK_EMSCRIPTEN_TOOLCHAIN OR SK_EMSCRIPTEN_TOOLCHAIN STREQUAL "")
	if(DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "Emscripten.cmake")
		set(SK_EMSCRIPTEN_TOOLCHAIN "${CMAKE_TOOLCHAIN_FILE}")
	elseif(DEFINED ENV{EMSDK} AND EXISTS "$ENV{EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
		set(SK_EMSCRIPTEN_TOOLCHAIN "$ENV{EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
	else()
		find_program(_SK_EMCC emcc)
		if(_SK_EMCC)
			get_filename_component(_SK_EM_DIR "${_SK_EMCC}" DIRECTORY)
			if(EXISTS "${_SK_EM_DIR}/cmake/Modules/Platform/Emscripten.cmake")
				set(SK_EMSCRIPTEN_TOOLCHAIN "${_SK_EM_DIR}/cmake/Modules/Platform/Emscripten.cmake")
			endif()
		endif()
	endif()
	if(NOT SK_EMSCRIPTEN_TOOLCHAIN OR SK_EMSCRIPTEN_TOOLCHAIN STREQUAL "")
		foreach(_SK_EMSDK_CAND IN ITEMS
			"$ENV{HOME}/emsdk"
			"$ENV{HOME}/Projects/emsdk"
			"$ENV{USERPROFILE}/emsdk"
			"$ENV{USERPROFILE}/Projects/emsdk"
			"$ENV{USERPROFILE}/source/emsdk"
			"${CMAKE_SOURCE_DIR}/../emsdk")
			if(EXISTS "${_SK_EMSDK_CAND}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
				set(SK_EMSCRIPTEN_TOOLCHAIN "${_SK_EMSDK_CAND}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
				break()
			endif()
		endforeach()
	endif()
endif()
set(SK_EMSCRIPTEN_TOOLCHAIN "${SK_EMSCRIPTEN_TOOLCHAIN}" CACHE FILEPATH "Emscripten.cmake toolchain")
