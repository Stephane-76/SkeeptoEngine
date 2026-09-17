# skeepto-engine superbuild — configure once at the repo root and compile
# every C++ module. Build system is CMake only.
#
# Use a distinct -B dir per generator (CMake cannot mix Xcode and Makefiles
# in the same folder).
#
# Native (Unix Makefiles):
#   cmake -B build-unix
#   cmake --build build-unix --parallel
#
# Xcode (Debug + Release):
#   cmake -G Xcode -B build-xcode -DSK_PLATFORM=xcode
#   cmake --build build-xcode --parallel
#   open build-xcode/skeepto-engine.xcodeproj
#
# Windows:
#   cmake -G "Visual Studio 17 2022" -A x64 -B build-windows -DSK_PLATFORM=windows
#   cmake --build build-windows --parallel
#
# WebAssembly (emsdk on PATH or EMSDK set; SK_COMPIL=DEBUG|RELEASE,
# SK_MEMORY64=ON for wasm64). On Windows, children use Ninja (Unix Makefiles
# need `make`, which is not installed by default):
#   cmake -B build-wasm -DSK_PLATFORM=wasm -DSK_SKEEPTO_DIR=/path/to/skeepto
#   cmake --build build-wasm --parallel
# Sibling ../skeepto is used when SK_SKEEPTO_DIR is empty. NONE skips the copy.
#
# Then (optional): cmake --build build-unix --target run-tests
#
# Third-party (cloned into third-party/, then compiled as needed):
#   rapidjson + rapidxml (headers), cppunit (tests), pugixml + libzip (SkExcel).
#   wasm also builds SkExcelLib.js (in-process module) next to SkExcel.js.
#   pugixml/libzip are skipped when their install artifacts already exist
#   (override with -DSK_FORCE_THIRD_PARTY=ON).
#
# Child CMakeLists stay standalone: this file only sequences them
# (ExternalProject) into per-module dirs (Libraries/SkRoot/unix, …) and the
# same artifact tree (<repo>/<platform>/lib|bin).

cmake_minimum_required(VERSION 3.16)
# ExternalProject + Xcode "new build system" (step targets / dependencies).
if(POLICY CMP0114)
	cmake_policy(SET CMP0114 NEW)
endif()
# URL downloads: stamp extracted files with extract time (avoids stale rebuilds).
if(POLICY CMP0135)
	cmake_policy(SET CMP0135 NEW)
endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
	message(FATAL_ERROR "In-source configure is not supported. Use: cmake -B build")
endif()

project(skeepto-engine NONE)

include(ExternalProject)

# --- Platform ----------------------------------------------------------------
set(SK_PLATFORM "" CACHE STRING "unix, xcode, windows, or wasm")
if(SK_PLATFORM STREQUAL "")
	if(EMSCRIPTEN)
		set(SK_PLATFORM "wasm")
	elseif(WIN32)
		set(SK_PLATFORM "windows")
	elseif(CMAKE_GENERATOR STREQUAL "Xcode")
		set(SK_PLATFORM "xcode")
	else()
		set(SK_PLATFORM "unix")
	endif()
	set(SK_PLATFORM "${SK_PLATFORM}" CACHE STRING "unix, xcode, windows, or wasm" FORCE)
endif()

option(SK_BUILD_TESTS "Build Libraries_test executables" ON)
option(SK_BUILD_APPS "Build SkExcel, SkPressureSp, and SkReactSpreadSheet" ON)
option(SK_BUILD_SKEXCEL_LIB "Build SkExcelLib.js/.wasm (in-process wasm module, wasm only)" ON)
option(SK_BUILD_REACT_NODE "Build Node.js SkReactSpreadSheet (ENVIRONMENT=node, wasm32 only)" ON)
option(SK_SPREADSHEET_TEST_MULTI_USER "Pass TestMultiUser to SkSpreadSheet (native tests only)" OFF)
option(SK_FETCH_THIRD_PARTY "Clone missing third-party repos into third-party/" ON)
option(SK_BUILD_THIRD_PARTY "Compile cppunit / pugixml / libzip when needed" ON)
option(SK_FORCE_THIRD_PARTY "Rebuild third-party even if artifacts already exist" OFF)

# Defaults when unset / empty: RELEASE wasm, Memory64 OFF (wasm32).
# Write CACHE with FORCE so ExternalProject does not inherit an empty string.
set(SK_COMPIL_MODE "$ENV{SK_COMPIL}" CACHE STRING "wasm: DEBUG or RELEASE (same as SK_COMPIL)")
if(SK_COMPIL_MODE STREQUAL "")
	set(SK_COMPIL_MODE "RELEASE" CACHE STRING "wasm: DEBUG or RELEASE (same as SK_COMPIL)" FORCE)
endif()

set(SK_MEMORY64 "$ENV{SK_64Bit}" CACHE STRING "wasm Memory64: ON or OFF")
if(SK_MEMORY64 STREQUAL "")
	set(SK_MEMORY64 "OFF" CACHE STRING "wasm Memory64: ON or OFF" FORCE)
endif()
string(TOUPPER "${SK_MEMORY64}" _SK_MEMORY64_UP)

set(SK_VS_GENERATOR "Visual Studio 17 2022" CACHE STRING "Visual Studio generator for SK_PLATFORM=windows")
set(SK_CONFIGS "Debug;Release" CACHE STRING "Configs built for Xcode / Visual Studio")

# --- Child generator / build subdir ------------------------------------------
set(SK_MULTI_CONFIG FALSE)
set(SK_CHILD_GENERATOR_PLATFORM "")
if(SK_PLATFORM STREQUAL "xcode")
	set(SK_CHILD_GENERATOR "Xcode")
	set(SK_BUILD_SUBDIR "xcode")
	set(SK_OUTPUT_DIR "xcode")
	set(SK_MULTI_CONFIG TRUE)
elseif(SK_PLATFORM STREQUAL "windows")
	set(SK_CHILD_GENERATOR "${SK_VS_GENERATOR}")
	set(SK_CHILD_GENERATOR_PLATFORM "x64")
	set(SK_BUILD_SUBDIR "windows")
	set(SK_OUTPUT_DIR "windows")
	set(SK_MULTI_CONFIG TRUE)
elseif(SK_PLATFORM STREQUAL "wasm")
	set(SK_CHILD_GENERATOR "Unix Makefiles")
	if(_SK_MEMORY64_UP STREQUAL "ON" OR _SK_MEMORY64_UP STREQUAL "1"
		OR _SK_MEMORY64_UP STREQUAL "TRUE" OR _SK_MEMORY64_UP STREQUAL "YES")
		set(SK_BUILD_SUBDIR "wasm64")
		set(SK_WASM_MEMORY64 TRUE)
	else()
		set(SK_BUILD_SUBDIR "wasm")
		set(SK_WASM_MEMORY64 FALSE)
	endif()
	set(SK_OUTPUT_DIR "${SK_BUILD_SUBDIR}")
else()
	set(SK_CHILD_GENERATOR "Unix Makefiles")
	set(SK_BUILD_SUBDIR "unix")
	set(SK_OUTPUT_DIR "unix")
endif()

# Windows has no `make` by default: pick Ninja (or mingw32-make) for children.
include("${CMAKE_SOURCE_DIR}/cmake/SkSelectChildGenerator.cmake")

# Artifact tree <repo>/<platform>/{lib,bin}. A stray file of that name (common)
# after a zsh paste on case-insensitive APFS) would make ar fail at link time.
include("${CMAKE_SOURCE_DIR}/cmake/SkEnsureOutput.cmake")
sk_ensure_output_dir("${CMAKE_SOURCE_DIR}/${SK_OUTPUT_DIR}")

# macOS unix/xcode: refresh CMAKE_OSX_SYSROOT (Xcode updates drop old SDK folders)
# and FORCE it into ExternalProject child caches (Libraries/*/unix/CMakeCache.txt).
if(APPLE AND NOT SK_PLATFORM STREQUAL "wasm")
	include("${CMAKE_SOURCE_DIR}/cmake/SkDarwinSdk.cmake")
	if(CMAKE_OSX_SYSROOT)
		list(APPEND SK_CHILD_CMAKE_CACHE_ARGS
			"-DCMAKE_OSX_SYSROOT:PATH=${CMAKE_OSX_SYSROOT}")
	endif()
endif()

# --- Common child cmake args -------------------------------------------------
set(SK_COMMON_CMAKE_ARGS
	-DSK_PLATFORM=${SK_PLATFORM}
)
if(CMAKE_BUILD_TYPE)
	list(APPEND SK_COMMON_CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
endif()

if(SK_PLATFORM STREQUAL "wasm")
	include("${CMAKE_SOURCE_DIR}/cmake/SkFindEmscripten.cmake")
	if(NOT EXISTS "${SK_EMSCRIPTEN_TOOLCHAIN}")
		message(FATAL_ERROR
			"SK_PLATFORM=wasm but Emscripten toolchain not found.\n"
			"Set EMSDK, put emcc on PATH, or pass -DSK_EMSCRIPTEN_TOOLCHAIN=.../Emscripten.cmake")
	endif()
	message(STATUS "skeepto-engine: Emscripten toolchain=${SK_EMSCRIPTEN_TOOLCHAIN}")
	# Child builds always pass CMAKE_BUILD_TYPE=Release; SK_COMPIL_MODE selects debug vs -O3.
	list(APPEND SK_COMMON_CMAKE_ARGS
		-DCMAKE_TOOLCHAIN_FILE=${SK_EMSCRIPTEN_TOOLCHAIN}
		-DEMSCRIPTEN=1
		-DCMAKE_BUILD_TYPE=Release
		-DSK_COMPIL_MODE=${SK_COMPIL_MODE}
		-DSK_MEMORY64=${SK_MEMORY64}
		-DSK_WASM_OUTPUT_SUBDIR=${SK_BUILD_SUBDIR}
	)
endif()

message(STATUS "skeepto-engine: SK_PLATFORM=${SK_PLATFORM} generator=${SK_CHILD_GENERATOR} subdir=${SK_BUILD_SUBDIR}")
if(SK_PLATFORM STREQUAL "wasm")
	message(STATUS "skeepto-engine: SK_COMPIL_MODE=${SK_COMPIL_MODE} SK_MEMORY64=${SK_MEMORY64}")
endif()

include("${CMAKE_SOURCE_DIR}/cmake/SkThirdParty.cmake")

# --- Helper: one ExternalProject per existing module CMakeLists.txt ---------
function(sk_add_module target_name source_dir)
	set(options TEST APP)
	set(oneValueArgs BINARY_SUBDIR)
	set(multiValueArgs DEPENDS EXTRA_ARGS)
	cmake_parse_arguments(SK "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	if(SK_TEST AND NOT SK_BUILD_TESTS)
		return()
	endif()
	if(SK_APP AND NOT SK_BUILD_APPS)
		return()
	endif()

	set(_src "${CMAKE_SOURCE_DIR}/${source_dir}")
	if(NOT EXISTS "${_src}/CMakeLists.txt")
		message(FATAL_ERROR "Missing ${_src}/CMakeLists.txt")
	endif()

	set(_bin_subdir "${SK_BUILD_SUBDIR}")
	if(SK_BINARY_SUBDIR)
		set(_bin_subdir "${SK_BINARY_SUBDIR}")
	endif()

	sk_reset_stale_cmake_cache("${_src}/${_bin_subdir}" "${_src}")

	set(_cmake_args ${SK_COMMON_CMAKE_ARGS} ${SK_EXTRA_ARGS})

	set(_gen_plat)
	if(SK_CHILD_GENERATOR_PLATFORM)
		set(_gen_plat CMAKE_GENERATOR_PLATFORM ${SK_CHILD_GENERATOR_PLATFORM})
	endif()

	if(SK_MULTI_CONFIG)
		set(_build_cmd)
		set(_first TRUE)
		foreach(_cfg IN LISTS SK_CONFIGS)
			if(_first)
				set(_build_cmd ${CMAKE_COMMAND} --build <BINARY_DIR> --config ${_cfg} --parallel)
				set(_first FALSE)
			else()
				list(APPEND _build_cmd COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config ${_cfg} --parallel)
			endif()
		endforeach()
	else()
		set(_build_cmd ${CMAKE_COMMAND} --build <BINARY_DIR> --parallel)
	endif()

	ExternalProject_Add(${target_name}
		SOURCE_DIR "${_src}"
		BINARY_DIR "${_src}/${_bin_subdir}"
		PREFIX "${CMAKE_BINARY_DIR}/ep/${target_name}"
		DOWNLOAD_COMMAND ""
		UPDATE_COMMAND ""
		PATCH_COMMAND ""
		INSTALL_COMMAND ""
		CMAKE_GENERATOR "${SK_CHILD_GENERATOR}"
		${_gen_plat}
		CMAKE_ARGS ${_cmake_args}
		CMAKE_CACHE_ARGS ${SK_CHILD_CMAKE_CACHE_ARGS}
		BUILD_COMMAND ${_build_cmd}
		# Always run the child cmake --build. ExternalProject stamps ignore .cpp
		# edits in SOURCE_DIR, so tests/apps would keep a stale binary after a lib
		# change. Incremental child builds are cheap when nothing changed.
		BUILD_ALWAYS TRUE
		DEPENDS ${SK_DEPENDS}
		USES_TERMINAL_CONFIGURE TRUE
		USES_TERMINAL_BUILD TRUE
		LOG_OUTPUT_ON_FAILURE TRUE
	)

	if(SK_TEST)
		set_property(GLOBAL APPEND PROPERTY SK_TEST_TARGETS ${target_name})
	endif()
endfunction()

# --- Libraries ----------------------------------------------------------------
sk_add_module(SkRoot        Libraries/SkRoot)
sk_add_module(SkFormat      Libraries/SkFormat      DEPENDS SkRoot)
sk_add_module(SkSpreadSheet Libraries/SkSpreadSheet DEPENDS SkRoot SkFormat
	EXTRA_ARGS -DSK_SPREADSHEET_TEST_MULTI_USER=${SK_SPREADSHEET_TEST_MULTI_USER})

# --- Tests --------------------------------------------------------------------
# Tests link the static libs by path (not CMake targets). DEPENDS must cover every
# .a they pass to the linker, otherwise a parallel build can link a stale archive.
sk_add_module(SkRoot_test        Libraries_test/SkRoot        TEST DEPENDS SkRoot SkFormat SkSpreadSheet ${SK_TP_TEST_DEPS})
sk_add_module(SkFormat_test      Libraries_test/SkFormat      TEST DEPENDS SkRoot SkFormat SkSpreadSheet ${SK_TP_TEST_DEPS}
	EXTRA_ARGS -DSK_SPREADSHEET_TEST_MULTI_USER=${SK_SPREADSHEET_TEST_MULTI_USER})
sk_add_module(SkSpreadSheet_test Libraries_test/SkSpreadSheet TEST DEPENDS SkRoot SkFormat SkSpreadSheet ${SK_TP_TEST_DEPS}
	EXTRA_ARGS -DSK_SPREADSHEET_TEST_MULTI_USER=${SK_SPREADSHEET_TEST_MULTI_USER})

# --- Apps ---------------------------------------------------------------------
# SkExcel CLI (SkExcel.js). wasm also builds SkExcelLib.js in a second tree:
# SKEXCEL_WASM_LIB changes OUTPUT_NAME + link flags, so it cannot share SkExcel/wasm/.
sk_add_module(SkExcel            SkExcel            APP DEPENDS SkSpreadSheet ${SK_TP_EXCEL_DEPS}
	EXTRA_ARGS -DSKEXCEL_WASM_LIB=OFF)
if(SK_PLATFORM STREQUAL "wasm" AND SK_BUILD_SKEXCEL_LIB)
	sk_add_module(SkExcelLib        SkExcel            APP DEPENDS SkSpreadSheet ${SK_TP_EXCEL_DEPS}
		BINARY_SUBDIR ${SK_BUILD_SUBDIR}_lib
		EXTRA_ARGS -DSKEXCEL_WASM_LIB=ON)
endif()
sk_add_module(SkPressureSp       SkPressureSp       APP DEPENDS SkSpreadSheet)
# Browser module (ENVIRONMENT=web). Node needs a second tree: SK_REACT_NODE
# changes -DSK_NODE + ENVIRONMENT=node + OUTPUT_NAME, so it cannot share
# SkReactSpreadSheet/wasm/ (same leftover-CMakeCache problem as SK_NODE env).
sk_add_module(SkReactSpreadSheet SkReactSpreadSheet APP DEPENDS SkSpreadSheet
	EXTRA_ARGS -DSK_REACT_NODE=OFF)
if(SK_PLATFORM STREQUAL "wasm" AND SK_BUILD_REACT_NODE AND NOT SK_WASM_MEMORY64)
	sk_add_module(SkReactSpreadSheetNode SkReactSpreadSheet APP DEPENDS SkSpreadSheet
		BINARY_SUBDIR ${SK_BUILD_SUBDIR}_node
		EXTRA_ARGS -DSK_REACT_NODE=ON)
endif()

if(SK_PLATFORM STREQUAL "wasm" AND SK_BUILD_APPS)
	include("${CMAKE_SOURCE_DIR}/cmake/SkSyncSkeepto.cmake")
endif()

# --- run-tests ----------------------------------------------------------------
if(SK_BUILD_TESTS)
	get_property(_sk_tests GLOBAL PROPERTY SK_TEST_TARGETS)
	if(SK_MULTI_CONFIG)
		list(GET SK_CONFIGS 0 _sk_run_cfg)
		set(_sk_bin "${CMAKE_SOURCE_DIR}/${SK_OUTPUT_DIR}/bin/${_sk_run_cfg}")
	else()
		set(_sk_bin "${CMAKE_SOURCE_DIR}/${SK_OUTPUT_DIR}/bin")
	endif()

	set(_sk_run_cmds)
	# Fixtures opened as relative paths from the test working directory.
	list(APPEND _sk_run_cmds COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${CMAKE_SOURCE_DIR}/Libraries_test/SkSpreadSheet/Test.json" "${_sk_bin}/Test.json")
	list(APPEND _sk_run_cmds COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${CMAKE_SOURCE_DIR}/Libraries_test/SkFormat/SpreadSheet.json" "${_sk_bin}/SpreadSheet.json")
	foreach(_t IN ITEMS SkRoot_test SkFormat_test SkSpreadSheet_test)
		if(SK_PLATFORM STREQUAL "wasm")
			set(_sk_node "node")
			if(DEFINED ENV{SK_TEST_NODE} AND NOT "$ENV{SK_TEST_NODE}" STREQUAL "")
				set(_sk_node "$ENV{SK_TEST_NODE}")
			endif()
			list(APPEND _sk_run_cmds COMMAND ${CMAKE_COMMAND} -E echo "===== ${_t} =====")
			list(APPEND _sk_run_cmds COMMAND ${_sk_node} "${_sk_bin}/${_t}")
		elseif(SK_PLATFORM STREQUAL "windows")
			list(APPEND _sk_run_cmds COMMAND ${CMAKE_COMMAND} -E echo "===== ${_t} =====")
			list(APPEND _sk_run_cmds COMMAND "${_sk_bin}/${_t}.exe")
		else()
			list(APPEND _sk_run_cmds COMMAND ${CMAKE_COMMAND} -E echo "===== ${_t} =====")
			list(APPEND _sk_run_cmds COMMAND "${_sk_bin}/${_t}")
		endif()
	endforeach()

	add_custom_target(run-tests
		DEPENDS ${_sk_tests}
		${_sk_run_cmds}
		WORKING_DIRECTORY "${_sk_bin}"
		USES_TERMINAL
		COMMENT "Run skeepto-engine unit tests from ${_sk_bin}"
	)
endif()
