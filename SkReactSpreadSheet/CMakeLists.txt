# SKLibrary Make ==============================================================
cmake_minimum_required(VERSION 3.16)

# Locate cmake/SkDarwinSdk.cmake from repository root (Libraries/* / Libraries_test/* / top-level CMake).
set(_SK_DARWIN_SDK_SEARCH "${CMAKE_CURRENT_SOURCE_DIR}")
foreach(_SK_SDK_IDX RANGE 16)
	if(EXISTS "${_SK_DARWIN_SDK_SEARCH}/cmake/SkDarwinSdk.cmake")
		include("${_SK_DARWIN_SDK_SEARCH}/cmake/SkDarwinSdk.cmake")
		break()
	endif()
	get_filename_component(_SK_DARWIN_SDK_PARENT "${_SK_DARWIN_SDK_SEARCH}" DIRECTORY)
	if(_SK_DARWIN_SDK_SEARCH STREQUAL _SK_DARWIN_SDK_PARENT)
		break()
	endif()
	set(_SK_DARWIN_SDK_SEARCH "${_SK_DARWIN_SDK_PARENT}")
endforeach()

# Set Name and version ========================================================
set(CURRENT_PROJECT SkReactSpreadSheet)
project(${CURRENT_PROJECT}  CXX)

# specify the C++ standard ====================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Get Root Directory ==========================================================
set(SK_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

cmake_path(GET SK_DIRECTORY PARENT_PATH SK_ROOT)

## for  Visual studio and Xcode :
## the directory  Debug and Release are configurated by compiler =============
set(SK_OUTPUT "${SK_ROOT}/${SK_PLATFORM}")
# wasm64 gets its own output tree (sker/wasm64) so wasm32 and wasm64 artifacts
# never clobber each other. Driven by -DSK_WASM_OUTPUT_SUBDIR from compilWasm.sh
# (empty => sker/wasm unchanged).
if(DEFINED SK_WASM_OUTPUT_SUBDIR AND NOT "${SK_WASM_OUTPUT_SUBDIR}" STREQUAL "")
	set(SK_OUTPUT "${SK_ROOT}/${SK_WASM_OUTPUT_SUBDIR}")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${SK_OUTPUT}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${SK_OUTPUT}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${SK_OUTPUT}/bin)
include("${SK_ROOT}/cmake/SkEnsureOutput.cmake")

set(SK_LIBRARY "${SK_ROOT}/Libraries")
set(SK_THIRD_PARTY "${SK_ROOT}/third-party")

# Add source files ============================================================
file(GLOB_RECURSE INCLUDEFILES "./include/*.hpp")
file(GLOB_RECURSE SRCFILES "./source/*.cpp")

include_directories(
		${SK_LIBRARY}/SkRoot/include
		${SK_LIBRARY}/SkSpreadSheet/include
		${SK_LIBRARY}/SkFormat/include
		${SK_THIRD_PARTY}/rapidjson/include
)

## Add executable =============================================================
add_executable(${CURRENT_PROJECT}
	${INCLUDEFILES}
	${SRCFILES}
)

## link with library ========================================================== 
set(SK_SEARCH_LIB ${SK_OUTPUT}/lib)

## Compiler specific ==========================================================
if (${SK_PLATFORM} STREQUAL windows)
	## link with library ====================================================== 
	set(SK_SEARCH_LIB ${SK_OUTPUT}/lib)
	set(PROJECT_LIBRARY 
		Rpcrt4.lib
		Shlwapi.lib
		debug ${SK_SEARCH_LIB}/Debug/SkRoot.lib optimized ${SK_SEARCH_LIB}/Release/SkRoot.lib
		debug ${SK_SEARCH_LIB}/Debug/SkSpreadSheet.lib optimized ${SK_SEARCH_LIB}/Release/SkSpreadSheet.lib
		debug ${SK_SEARCH_LIB}/Debug/SkFormat.lib optimized ${SK_SEARCH_LIB}/Release/SkFormat.lib
	)
elseif(${SK_PLATFORM} STREQUAL unix)
	# Dependents before dependencies (Apple ld64 has no --start-group).
	# Do not list libstdc++.a by filename — Ubuntu CI cannot find it.
	set(PROJECT_LIBRARY
		${SK_SEARCH_LIB}/libSkSpreadSheet.a
		${SK_SEARCH_LIB}/libSkFormat.a
		${SK_SEARCH_LIB}/libSkRoot.a
	)
elseif(${SK_PLATFORM} STREQUAL wasm)
	set(CMAKE_CXX_COMPILER "emcc")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++20 -Wextra")
	include("${SK_ROOT}/cmake/SkWasmCompil.cmake")
	message("Compil Mode -->" ${SK_COMPIL_MODE})
	
	# Set library paths for WASM
	set(PROJECT_LIBRARY 
		${SK_SEARCH_LIB}/libSkRoot.a 
		${SK_SEARCH_LIB}/libSkSpreadSheet.a 
		${SK_SEARCH_LIB}/libSkFormat.a 
	)
	# WASM source maps: normalize paths to repo-relative (Libraries/..., SkReactSpreadSheet/..., third-party/...)
	# so Chrome can fetch sources from SK_SOURCE_MAP_BASE (SkServer). Optional env SK_SOURCE_MAP_BASE, default below.
	get_filename_component(SK_ROOT_ABS "${SK_ROOT}" REALPATH)
	if(DEFINED ENV{SK_SOURCE_MAP_BASE} AND NOT "$ENV{SK_SOURCE_MAP_BASE}" STREQUAL "")
		set(SK_WASM_SOURCE_MAP_BASE "$ENV{SK_SOURCE_MAP_BASE}")
	else()
		set(SK_WASM_SOURCE_MAP_BASE "http://127.0.0.1:8000")
	endif()
	string(REGEX REPLACE "/+$" "" SK_WASM_SOURCE_MAP_BASE "${SK_WASM_SOURCE_MAP_BASE}")
	set(SK_WASM_SOURCE_MAP_BASE "${SK_WASM_SOURCE_MAP_BASE}/")
	# Default Emscripten wasm stack is small; large .sker + ReadJson/RecalculateAll can overflow (seen as "memory access out of bounds" in Node).
	set(SK_REACT_WASM_STACK_SIZE "8388608" CACHE STRING "SkReactSpreadSheet wasm: -sSTACK_SIZE bytes (default 8MiB)")
	set(SK_REACT_WASM_MAX_MEMORY "4294967296" CACHE STRING "SkReactSpreadSheet wasm: -sMAXIMUM_MEMORY bytes (default 4GiB, wasm32 ceiling)")
	# Node vs browser: CMake option (not $ENV{SK_NODE}) so the superbuild can
	# compile both without wiping the child tree. Env SK_NODE=1 still works
	# for a standalone child configure.
	set(SK_REACT_NODE OFF CACHE BOOL "wasm: Node.js glue (-DSK_NODE, ENVIRONMENT=node)")
	if(NOT SK_REACT_NODE AND "$ENV{SK_NODE}" STREQUAL "1")
		set(SK_REACT_NODE ON)
	endif()

	# 64-bit (Memory64) build: raise the memory ceiling above 4GiB and emit a
	# distinct module name so the wasm32 (web/Safari) and wasm64 (desktop)
	# artifacts can live side by side. SK_WASM_MEMORY64 comes from SkWasmCompil.cmake.
	if(SK_WASM_MEMORY64)
		set(SK_REACT_WASM_MAX_MEMORY "8589934592") # 8 GiB
		set(SK_REACT_MODULE_NAME "SkReactSpreadSheet64")
		# Current V8 (Node 23 / Electron 32 = Chromium 128) can't grow a memory64
		# heap: WebAssembly.Memory.grow() throws "Cannot convert a BigInt value to
		# a number", so the app crashes the moment the heap needs to grow past the
		# initial size. Until Chromium/V8 ships a fixed grow (Chrome 133+),
		# preallocate a large INITIAL_MEMORY so normal workbooks never trigger a
		# grow. Usable memory is effectively capped at this value; raise it (up to
		# MAXIMUM_MEMORY) with -DSK_REACT_WASM_INIT_MEMORY=<bytes> for bigger files.
		set(SK_REACT_WASM_INIT_MEMORY "1073741824" CACHE STRING "SkReactSpreadSheet wasm64: -sINITIAL_MEMORY bytes (default 1GiB; avoids the broken memory64 grow)")
		# -sMEMORY64=1 + native Wasm EH/longjmp (see SkWasmCompil.cmake note).
		set(_SK_REACT_MEM64_LINK "${SK_WASM_MEM64_LINK_FLAGS} -sINITIAL_MEMORY=${SK_REACT_WASM_INIT_MEMORY}")
	else()
		set(SK_REACT_MODULE_NAME "SkReactSpreadSheet")
		set(_SK_REACT_MEM64_LINK "")
	endif()
	# Node artifacts must not overwrite the browser files in wasm/bin/.
	if(SK_REACT_NODE)
		set(SK_REACT_MODULE_NAME "${SK_REACT_MODULE_NAME}Node")
	endif()
	# CMake appends its own "-o <RUNTIME_OUTPUT_DIR>/<OUTPUT_NAME>.js" AFTER our
	# LINK_FLAGS, and emcc keeps the last -o. So the final artifact name must be
	# driven by OUTPUT_NAME (a -o inside LINK_FLAGS would be overridden). This is
	# what makes the wasm64 build emit SkReactSpreadSheet64.js/.wasm instead of
	# overwriting the wasm32 files.
	set_target_properties(${CURRENT_PROJECT} PROPERTIES OUTPUT_NAME "${SK_REACT_MODULE_NAME}")
	# -sASSERTIONS: clearer abort messages than "RuntimeError: Aborted(undefined)" under Node (see Emscripten docs).
	# Debug wasm (SK_COMPIL != RELEASE): use 2. Release Node/Shell: override with cmake -DSK_REACT_WASM_ASSERTIONS=0|1|2
	set(SK_REACT_WASM_ASSERTIONS "1" CACHE STRING "SkReactSpreadSheet wasm (Node/Shell release): -sASSERTIONS (0=off, 1=default, 2=verbose abort)")
	if(SK_WASM_RELEASE)
		message("Compil with Release")
		add_definitions(-DSK_RELEASE)
		set(_SK_REACT_ASSERT_LEVEL "${SK_REACT_WASM_ASSERTIONS}")
	else()
		message("Compil with Debug")
		set(_SK_REACT_ASSERT_LEVEL "2")
	endif()
	if("$ENV{SK_WEB_WORKER}" STREQUAL "1")
		message("Compil with Web Worker")
		add_definitions(-DSK_WEB_WORKER)
	endif()

	if(SK_REACT_NODE)
		message("Compil for Server (SK_NODE)")
		add_definitions(-DSK_NODE)
		if(SK_WASM_RELEASE)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -O3")
		else()
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -g3 -gsource-map -O0 -ffile-prefix-map=${SK_ROOT_ABS}/= -fdebug-compilation-dir=${SK_ROOT_ABS}")
		endif()
		set_property(TARGET ${CURRENT_PROJECT} PROPERTY LINK_FLAGS "\
			--bind \
			--no-entry \
			-s WASM=1 \
			-s MODULARIZE=1 \
			-s EXPORT_ES6=1 \
			-s EXPORT_NAME='createModule' \
			-s EXPORTED_FUNCTIONS=[] \
			-s ALLOW_MEMORY_GROWTH=1 \
			-s STACK_SIZE=${SK_REACT_WASM_STACK_SIZE} \
			-s MAXIMUM_MEMORY=${SK_REACT_WASM_MAX_MEMORY} ${_SK_REACT_MEM64_LINK} \
			-s ASSERTIONS=${_SK_REACT_ASSERT_LEVEL} \
			-s ENVIRONMENT=node \
			-o ${SK_REACT_MODULE_NAME}.js \
			--source-map \
			--source-map-name=${SK_REACT_MODULE_NAME}.wasm.map \
			--source-map-base=${SK_WASM_SOURCE_MAP_BASE} \
		")
	elseif("$ENV{SK_SHELL}" STREQUAL "1")
		message("Compil for Shell")
		add_definitions(-DSK_SHELL)
		if(SK_WASM_RELEASE)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -O3")
		else()
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -g3 -gsource-map -O0 -ffile-prefix-map=${SK_ROOT_ABS}/= -fdebug-compilation-dir=${SK_ROOT_ABS}")
		endif()
		set_property(TARGET ${CURRENT_PROJECT} PROPERTY LINK_FLAGS "\
			--bind \
			--no-entry \
			-s WASM=1 \
			-s MODULARIZE=1 \
			-s EXPORT_ES6=1 \
			-s EXPORT_NAME='createModule' \
			-s EXPORTED_FUNCTIONS=[] \
			-s ALLOW_MEMORY_GROWTH=1 \
			-s STACK_SIZE=${SK_REACT_WASM_STACK_SIZE} \
			-s MAXIMUM_MEMORY=${SK_REACT_WASM_MAX_MEMORY} ${_SK_REACT_MEM64_LINK} \
			-s ASSERTIONS=${_SK_REACT_ASSERT_LEVEL} \
			-s ENVIRONMENT=shell \
			-o ${SK_REACT_MODULE_NAME}.js \
			--source-map \
			--source-map-name=${SK_REACT_MODULE_NAME}.wasm.map \
			--source-map-base=${SK_WASM_SOURCE_MAP_BASE} \
		")
	else()
		message("Compil for Web Browser")
		if(SK_WASM_RELEASE)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -O3")
		else()
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -g3 -gsource-map -O0 -ffile-prefix-map=${SK_ROOT_ABS}/= -fdebug-compilation-dir=${SK_ROOT_ABS}")
		endif()

		set_property(TARGET ${CURRENT_PROJECT} PROPERTY LINK_FLAGS "\
			--bind \
			--no-entry \
			-s WASM=1 \
			-s EXPORT_ES6=1 \
			-s MODULARIZE=1 \
			-s EXPORT_NAME='createModule' \
			-s EXPORTED_FUNCTIONS=[] \
			-s ALLOW_MEMORY_GROWTH=1 \
			-s STACK_SIZE=${SK_REACT_WASM_STACK_SIZE} \
			-s MAXIMUM_MEMORY=${SK_REACT_WASM_MAX_MEMORY} ${_SK_REACT_MEM64_LINK} \
			-s ENVIRONMENT=web \
			-s ASSERTIONS=0 \
			-o ${SK_REACT_MODULE_NAME}.js \
			--source-map \
			--source-map-name=${SK_REACT_MODULE_NAME}.wasm.map \
			--source-map-base=${SK_WASM_SOURCE_MAP_BASE} \
		")
	endif()
elseif(${SK_PLATFORM} STREQUAL xcode)
	# Set library paths for Xcode with proper debug/optimized configuration
	set(PROJECT_LIBRARY 
		debug ${SK_SEARCH_LIB}/Debug/libSkRoot.a optimized ${SK_SEARCH_LIB}/Release/libSkRoot.a
		debug ${SK_SEARCH_LIB}/Debug/libSkSpreadSheet.a optimized ${SK_SEARCH_LIB}/Release/libSkSpreadSheet.a
		debug ${SK_SEARCH_LIB}/Debug/libSkFormat.a optimized ${SK_SEARCH_LIB}/Release/libSkFormat.a	
	)
endif()

if(${SK_PLATFORM} STREQUAL unix)
	include("${SK_ROOT}/cmake/SkLinkUnixStatics.cmake")
	sk_link_unix_statics(${CURRENT_PROJECT})
else()
	target_link_libraries(${CURRENT_PROJECT} ${PROJECT_LIBRARY})
endif()

## Compiler specific ===========================================================
if (${SK_PLATFORM} STREQUAL windows)
	# Set Runtime library
	if ("$ENV{SKLINK}" STREQUAL "DYNAMIC") 
		message("<-LINK DYNAMIC->" )
		if (CMAKE_BUILD_TYPE STREQUAL "Debug")		
			set(CMAKE_CXX_FLAGS_RELEASE "/MDd")
		else()
			set(CMAKE_CXX_FLAGS_RELEASE "/MD")
		endif()	
	else()
	# Set Static runtime library
		message("<-LINK STATIC->" )
		if (CMAKE_BUILD_TYPE STREQUAL "Debug")		
			set(CMAKE_CXX_FLAGS_RELEASE "/MTd")
		else()
			set(CMAKE_CXX_FLAGS_RELEASE "/MT")
		endif()
	endif() 
	#   Level of Warning
	# /FS: serialize PDB writes when MSBuild runs several CL.EXE on one project
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /FS")
elseif(${SK_PLATFORM} STREQUAL unix)
	set(CMAKE_CXX_COMPILER "/usr/bin/g++")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++20")
elseif(${SK_PLATFORM} STREQUAL xcode)
	# Improved Xcode configuration
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -Wall -Wextra")
	set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0")
	
	# Set proper compiler flags for Xcode
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
	endif()
endif()

## for Debug =================================================================
message( "ROOT->" ${SK_ROOT})
message( "PLATFORM->" ${SK_PLATFORM})
message( "LIBRARY->" ${SK_LIBRARY})
message( "OUTPUT->" ${SK_OUTPUT})
message( "CMAKE_BINARY_DIR->" ${CMAKE_BINARY_DIR})
message( "CMAKE_CXX_COMPILER ->" ${CMAKE_CXX_COMPILER})
message( "CMAKE_CXX_COMPILER_ID->" ${CMAKE_CXX_COMPILER_ID})
message( "CMAKE_GENERATOR->" ${CMAKE_GENERATOR})
message( "CMAKE_CXX_FLAGS->" ${CMAKE_CXX_FLAGS} )
message( "CMAKE_CXX_FLAGS_DEBUGSK->" ${CMAKE_CXX_FLAGS_DEBUGSK})
message( "CMAKE_CXX_FLAGS_RELEASE->" ${CMAKE_CXX_FLAGS_RELEASE})
#message( "INCLUDEFILES->" ${INCLUDEFILES})
message( "SRCFILES->" ${SRCFILES})
message( "SK_OUTPUT->" ${SK_OUTPUT})

