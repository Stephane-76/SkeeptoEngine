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

# macOS / Xcode: deployment target must be set before project(), and must stick in
# CMake cache — otherwise Xcode falls back to the SDK default (e.g. 26.4) and the
# app cannot run on older macOS builds (e.g. 26.3.x).
if(APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "Minimum macOS deployment version" FORCE)
	set(CMAKE_XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "14.0")
endif()

# Set Name and version ========================================================
set(CURRENT_PROJECT SkRoot_test)
project(${CURRENT_PROJECT}  CXX)

# specify the C++ standard ====================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Get Root Directory ==========================================================
set(SK_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

cmake_path(GET SK_DIRECTORY PARENT_PATH SK_PARENT)
cmake_path(GET SK_PARENT PARENT_PATH SK_ROOT)

message("Root->" ${SK_ROOT})
message("Platform->" ${SK_PLATFORM})

## for  Visual studio and Xcode :
## the directory  Debug and Release are configurated by compiler =============
set(SK_OUTPUT "${SK_ROOT}/${SK_PLATFORM}")
# wasm64 tests build/link/output under sker/wasm64 (matching 64-bit libs).
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
		${SK_THIRD_PARTY}/rapidjson/include
		${SK_THIRD_PARTY}/cppunit/include
		${SK_THIRD_PARTY}/cppunit/${SK_PLATFORM}
		${SK_THIRD_PARTY}/cppunit/${SK_PLATFORM}/include
		${SK_THIRD_PARTY}/cppunit-wasm
)

## Add executable =============================================================
add_executable(${CURRENT_PROJECT}
	${INCLUDEFILES}
	${SRCFILES}
)

if(EXISTS "${SK_ROOT}/File")
	file(TO_CMAKE_PATH "${SK_ROOT}/File" _SKER_FILE_DIR)
	target_compile_definitions(${CURRENT_PROJECT} PRIVATE "SKER_FILE_DIR=\"${_SKER_FILE_DIR}\"")
endif()

if(APPLE)
	set_target_properties(${CURRENT_PROJECT} PROPERTIES
		MACOSX_DEPLOYMENT_TARGET "14.0"
		XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "14.0"
	)
endif()

## link with library ========================================================== 
set(SK_SEARCH_LIB ${SK_OUTPUT}/lib)

if (${SK_PLATFORM} STREQUAL windows)
	set(PROJECT_LIBRARY 
		Rpcrt4.lib
		Shlwapi.lib
		debug ${SK_SEARCH_LIB}/Debug/SkRoot.lib optimized ${SK_SEARCH_LIB}/Release/SkRoot.lib
		debug ${SK_THIRD_PARTY}/cppunit/windows/Debug/cppunit.lib  optimized ${SK_THIRD_PARTY}/cppunit/windows/Release/cppunit.lib 
	)
elseif(${SK_PLATFORM} STREQUAL unix)
	set(PROJECT_LIBRARY 
		${SK_SEARCH_LIB}/libSkRoot.a 
		${SK_THIRD_PARTY}/cppunit/unix/libcppunit.a
	)
elseif(${SK_PLATFORM} STREQUAL wasm)
	set(CMAKE_CXX_COMPILER "emcc")
	include("${SK_ROOT}/cmake/SkWasmCompil.cmake")
	message("Compil Node -->" $ENV{SK_NODE})
	message("Compil Mode -->" ${SK_COMPIL_MODE})

	# 64-bit tests must link the wasm64 cppunit (ABI match) and carry -sMEMORY64.
	# Rebuild cppunit into third-party/cppunit/wasm64 first (see docs).
	if(SK_WASM_MEMORY64)
		set(SK_CPPUNIT_WASM_DIR "${SK_THIRD_PARTY}/cppunit/wasm64")
		# -sMEMORY64=1 + native Wasm EH/longjmp (see SkWasmCompil.cmake note).
		# Node 23's V8 rejects WebAssembly.Memory.grow() on a memory64 heap
		# ("Cannot convert a BigInt value to a number"), so growth crashes at
		# runtime. Preallocate 256 MiB so the unit tests never trigger a heap
		# grow. This is a test-only workaround; the desktop module keeps its own
		# (smaller) initial memory and relies on Electron/Chromium's V8 for grow.
		set(_SK_TEST_MEM64_LINK "${SK_WASM_MEM64_LINK_FLAGS} -sINITIAL_MEMORY=268435456")
		set(_SK_TEST_MAX_MEMORY "8GB")
	else()
		set(SK_CPPUNIT_WASM_DIR "${SK_THIRD_PARTY}/cppunit/wasm")
		set(_SK_TEST_MEM64_LINK "")
		set(_SK_TEST_MAX_MEMORY "2GB")
	endif()
	
	if("$ENV{SK_NODE}" STREQUAL "1")
		if(SK_WASM_RELEASE)
			add_definitions(-DSK_RELEASE)
			# Set library paths for WASM
			set(PROJECT_LIBRARY 
				${SK_SEARCH_LIB}/libSkRoot.a 
				${SK_SEARCH_LIB}/libSkSpreadSheet.a 
				${SK_SEARCH_LIB}/libSkFormat.a
				${SK_CPPUNIT_WASM_DIR}/Release/libcppunit.a
			)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -O3")
		else()
			# Set library paths for WASM
			set(PROJECT_LIBRARY 
				${SK_SEARCH_LIB}/libSkRoot.a 
				${SK_SEARCH_LIB}/libSkSpreadSheet.a 
				${SK_SEARCH_LIB}/libSkFormat.a
				${SK_CPPUNIT_WASM_DIR}/Debug/libcppunit.a
			)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -g3 -O0")
		endif()
		set_property(TARGET ${CURRENT_PROJECT} PROPERTY LINK_FLAGS "\
			--bind \
			--no-entry \
			-s WASM=1 \
			-s MODULARIZE=1 \
			-s EXPORT_NAME='createModule' \
			-s EXPORTED_FUNCTIONS=[] \
			${_SK_TEST_MEM64_LINK} \
			-o SkReactSpreadSheet.js \
			--source-map \
			--source-map-name=SkReactSpreadSheet.wasm.map \
		")
	else()
		if(SK_WASM_RELEASE)
			add_definitions(-DSK_RELEASE)
			# Set library paths for WASM
			set(PROJECT_LIBRARY 
				${SK_SEARCH_LIB}/libSkRoot.a 
				${SK_SEARCH_LIB}/libSkSpreadSheet.a 
				${SK_SEARCH_LIB}/libSkFormat.a
				${SK_CPPUNIT_WASM_DIR}/Release/libcppunit.a
			)
			
	
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -O3")
			set_property(TARGET ${CURRENT_PROJECT} PROPERTY LINK_FLAGS "\
				-O3 \
				--bind \
				-s ALLOW_MEMORY_GROWTH \
				-s MAXIMUM_MEMORY=${_SK_TEST_MAX_MEMORY} ${_SK_TEST_MEM64_LINK} \
				--source-map \
				--source-map-name=SkReactSpreadSheet.wasm.map \
			")
		else()
				# Set library paths for WASM
			set(PROJECT_LIBRARY 
				${SK_SEARCH_LIB}/libSkRoot.a 
				${SK_SEARCH_LIB}/libSkSpreadSheet.a 
				${SK_SEARCH_LIB}/libSkFormat.a
				${SK_CPPUNIT_WASM_DIR}/Debug/libcppunit.a
			)

			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -g3 -O0")
			set_property(TARGET ${CURRENT_PROJECT} PROPERTY LINK_FLAGS "\
				-O1 \
				--bind \
				-s ALLOW_MEMORY_GROWTH \
				-s MAXIMUM_MEMORY=${_SK_TEST_MAX_MEMORY} ${_SK_TEST_MEM64_LINK} \
				--source-map \
				--source-map-name=SkReactSpreadSheet.wasm.map \
			")
		endif()
	endif()

#else Xcode
else()
	set(PROJECT_LIBRARY 
		debug ${SK_SEARCH_LIB}/Debug/libSkRoot.a optimized  ${SK_SEARCH_LIB}/Release/libSkRoot.a
		debug ${SK_THIRD_PARTY}/cppunit/xcode/Debug/libcppunit.a optimized ${SK_THIRD_PARTY}/cppunit/xcode/Release/libcppunit.a
	)
endif()

if(${SK_PLATFORM} STREQUAL unix)
	include("${SK_ROOT}/cmake/SkLinkUnixStatics.cmake")
	sk_link_unix_statics(${CURRENT_PROJECT}
		LIBS ${SK_SEARCH_LIB}/libSkRoot.a
		EXTRA ${SK_THIRD_PARTY}/cppunit/unix/libcppunit.a)
else()
	target_link_libraries(${CURRENT_PROJECT} ${PROJECT_LIBRARY})
endif()

## Compiler specific ===========================================================
if (${SK_PLATFORM} STREQUAL windows)
	# Set Runtime library
	# Keep /MD (CMake/MSVC default) unless SKLINK=DYNAMIC is set. A real else()
	# /MT here used to LNK2038 against cppunit.lib and SkRoot.lib (/MD).
	if ("$ENV{SKLINK}" STREQUAL "DYNAMIC")
		message("<-LINK DYNAMIC->" )
		if (CMAKE_BUILD_TYPE STREQUAL "Debug")
			set(CMAKE_CXX_FLAGS_RELEASE "/MDd")
		else()
			set(CMAKE_CXX_FLAGS_RELEASE "/MD")
		endif()
	elseif ("$ENV{SKLINK}" STREQUAL "STATIC")
		message("<-LINK STATIC->" )
		if (CMAKE_BUILD_TYPE STREQUAL "Debug")
			set(CMAKE_CXX_FLAGS_RELEASE "/MTd")
		else()
			set(CMAKE_CXX_FLAGS_RELEASE "/MT")
		endif()
	endif()
	# /FS: serialize PDB writes when MSBuild runs several CL.EXE on one project
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /FS")

elseif(${SK_PLATFORM} STREQUAL unix)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++20")

elseif(${SK_PLATFORM} STREQUAL wasm)
	set(CMAKE_CXX_COMPILER "emcc")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++20")
elseif(${SK_PLATFORM} STREQUAL xcode)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()


## for Debug =================================================================
message( "CMAKE_BINARY_DIR->" ${CMAKE_BINARY_DIR})
message( "CMAKE_CXX_COMPILER ->" ${CMAKE_CXX_COMPILER})
message( "CMAKE_CXX_COMPILER_ID->" ${CMAKE_CXX_COMPILER_ID})
message( "CMAKE_GENERATOR->" ${CMAKE_GENERATOR})
message( "CMAKE_CXX_FLAGS->" ${CMAKE_CXX_FLAGS} )
message( "CMAKE_CXX_FLAGS_DEBUG->" ${CMAKE_CXX_FLAGS_DEBUG})
message( "CMAKE_CXX_FLAGS_RELEASE->" ${CMAKE_CXX_FLAGS_RELEASE})
#message( "INCLUDEFILES->" ${INCLUDEFILES})
message( "SRCFILES->" ${SRCFILES})
message( "SK_OUTPUT->" ${SK_OUTPUT})




