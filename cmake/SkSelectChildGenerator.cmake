# WASM / Unix children need Ninja or Make. Visual Studio cannot drive emcc.
# On Windows, `make` is usually missing; prefer Ninja from PATH or the VS
# CMake tools (CommonExtensions/Microsoft/CMake/Ninja).

set(SK_CHILD_MAKE_PROGRAM "${SK_CHILD_MAKE_PROGRAM}" CACHE FILEPATH
	"make or ninja used by ExternalProject children")
set(SK_CHILD_CMAKE_CACHE_ARGS "")

function(sk_normalize_program _in _out)
	file(TO_CMAKE_PATH "${_in}" _norm)
	set(${_out} "${_norm}" PARENT_SCOPE)
endfunction()

function(sk_find_ninja _out)
	if(SK_CHILD_MAKE_PROGRAM AND EXISTS "${SK_CHILD_MAKE_PROGRAM}")
		get_filename_component(_name "${SK_CHILD_MAKE_PROGRAM}" NAME_WE)
		if(_name STREQUAL "ninja")
			sk_normalize_program("${SK_CHILD_MAKE_PROGRAM}" _norm)
			set(${_out} "${_norm}" PARENT_SCOPE)
			return()
		endif()
	endif()

	find_program(_ninja NAMES ninja ninja.exe)
	if(_ninja)
		sk_normalize_program("${_ninja}" _norm)
		set(${_out} "${_norm}" PARENT_SCOPE)
		return()
	endif()

	# Visual Studio's CMake bundle: .../CMake/CMake/bin/cmake.exe
	# and ninja lives next to it in .../CMake/Ninja/ninja.exe
	get_filename_component(_cmake_bin "${CMAKE_COMMAND}" DIRECTORY)
	get_filename_component(_cmake_root "${_cmake_bin}" DIRECTORY)
	get_filename_component(_cmake_bundle "${_cmake_root}" DIRECTORY)
	if(EXISTS "${_cmake_bundle}/Ninja/ninja.exe")
		sk_normalize_program("${_cmake_bundle}/Ninja/ninja.exe" _norm)
		set(${_out} "${_norm}" PARENT_SCOPE)
		return()
	endif()

	set(_pf "$ENV{ProgramFiles}")
	set(_pf86 "$ENV{ProgramFiles\(x86\)}")
	file(GLOB _vs_ninjas
		"${_pf}/Microsoft Visual Studio/*/*/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe"
		"${_pf86}/Microsoft Visual Studio/*/*/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe")
	if(_vs_ninjas)
		list(SORT _vs_ninjas ORDER DESCENDING)
		list(GET _vs_ninjas 0 _first)
		sk_normalize_program("${_first}" _norm)
		set(${_out} "${_norm}" PARENT_SCOPE)
		return()
	endif()

	set(${_out} "" PARENT_SCOPE)
endfunction()

function(sk_find_make _out)
	if(SK_CHILD_MAKE_PROGRAM AND EXISTS "${SK_CHILD_MAKE_PROGRAM}")
		get_filename_component(_name "${SK_CHILD_MAKE_PROGRAM}" NAME_WE)
		if(_name STREQUAL "make" OR _name STREQUAL "gmake")
			sk_normalize_program("${SK_CHILD_MAKE_PROGRAM}" _norm)
			set(${_out} "${_norm}" PARENT_SCOPE)
			return()
		endif()
	endif()
	find_program(_make NAMES make make.exe gmake)
	if(_make)
		sk_normalize_program("${_make}" _norm)
		set(${_out} "${_norm}" PARENT_SCOPE)
		return()
	endif()
	set(${_out} "" PARENT_SCOPE)
endfunction()

function(sk_find_mingw_make _out)
	if(SK_CHILD_MAKE_PROGRAM AND EXISTS "${SK_CHILD_MAKE_PROGRAM}")
		get_filename_component(_name "${SK_CHILD_MAKE_PROGRAM}" NAME_WE)
		if(_name STREQUAL "mingw32-make")
			sk_normalize_program("${SK_CHILD_MAKE_PROGRAM}" _norm)
			set(${_out} "${_norm}" PARENT_SCOPE)
			return()
		endif()
	endif()
	find_program(_mingw NAMES mingw32-make mingw32-make.exe)
	if(_mingw)
		sk_normalize_program("${_mingw}" _norm)
		set(${_out} "${_norm}" PARENT_SCOPE)
		return()
	endif()
	set(${_out} "" PARENT_SCOPE)
endfunction()

# Only makefile/ninja children need a host build tool. VS/Xcode already have one.
if(NOT SK_CHILD_GENERATOR STREQUAL "Unix Makefiles")
	return()
endif()

sk_find_ninja(_sk_ninja)
sk_find_make(_sk_make)
sk_find_mingw_make(_sk_mingw)

if(WIN32)
	# Emscripten on Windows: Ninja is the supported generator. Unix Makefiles
	# fails with "CMAKE_MAKE_PROGRAM is not set" when make is absent (default).
	if(_sk_ninja)
		set(SK_CHILD_GENERATOR "Ninja")
		set(SK_CHILD_MAKE_PROGRAM "${_sk_ninja}" CACHE FILEPATH
			"make or ninja used by ExternalProject children" FORCE)
	elseif(_sk_mingw)
		set(SK_CHILD_GENERATOR "MinGW Makefiles")
		set(SK_CHILD_MAKE_PROGRAM "${_sk_mingw}" CACHE FILEPATH
			"make or ninja used by ExternalProject children" FORCE)
	elseif(_sk_make)
		set(SK_CHILD_GENERATOR "Unix Makefiles")
		set(SK_CHILD_MAKE_PROGRAM "${_sk_make}" CACHE FILEPATH
			"make or ninja used by ExternalProject children" FORCE)
	else()
		message(FATAL_ERROR
			"SK_PLATFORM=${SK_PLATFORM} needs Ninja (or make) to compile children.\n"
			"Windows has no Unix `make` by default, so CMake cannot use "
			"\"Unix Makefiles\". Install Ninja, or the Visual Studio "
			"\"C++ CMake tools for Windows\" workload, then reconfigure.\n"
			"  winget install Ninja-build.Ninja\n"
			"  cmake -G Ninja -B build-wasm -DSK_PLATFORM=wasm")
	endif()
else()
	if(_sk_make)
		set(SK_CHILD_MAKE_PROGRAM "${_sk_make}" CACHE FILEPATH
			"make or ninja used by ExternalProject children" FORCE)
	elseif(_sk_ninja)
		set(SK_CHILD_GENERATOR "Ninja")
		set(SK_CHILD_MAKE_PROGRAM "${_sk_ninja}" CACHE FILEPATH
			"make or ninja used by ExternalProject children" FORCE)
	endif()
endif()

if(SK_CHILD_MAKE_PROGRAM)
	list(APPEND SK_CHILD_CMAKE_CACHE_ARGS
		"-DCMAKE_MAKE_PROGRAM:FILEPATH=${SK_CHILD_MAKE_PROGRAM}")
	message(STATUS "skeepto-engine: child CMAKE_MAKE_PROGRAM=${SK_CHILD_MAKE_PROGRAM}")
endif()
