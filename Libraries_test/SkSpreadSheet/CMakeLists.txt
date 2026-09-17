# SKLibrary Test Make ==============================================================
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

# macOS / Xcode: deployment target must be set before project(); force cache so
# reconfigure does not leave SDK default (e.g. 26.4).
if(APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "Minimum macOS deployment version" FORCE)
	set(CMAKE_XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "14.0")
endif()

# Set Name and version ========================================================
set(CURRENT_PROJECT SkSpreadSheet_test)

# TestAll is toggled only via #define in TestRunner.cpp — never pass -DTestAll on the command line.
if(CMAKE_CXX_FLAGS MATCHES "-DTestAll" OR CMAKE_CXX_FLAGS STREQUAL "_SK_CXX_FLAGS_NO_TESTALL")
	set(CMAKE_CXX_FLAGS "" CACHE STRING "Flags used by the compiler during all build types." FORCE)
endif()

project(${CURRENT_PROJECT}  CXX)

# specify the C++ standard ====================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Get Root Directory ==========================================================
set(SK_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

cmake_path(GET SK_DIRECTORY PARENT_PATH SK_PARENT)
cmake_path(GET SK_PARENT PARENT_PATH SK_ROOT)

# Native + WASM fixtures live in <repo>/File/. WASM maps that folder into MEMFS
# /home/web_user/Projects/Excel/ (ExcelTestDataDir on emscripten). A cached path to
# ~/Projects/Excel is ignored when File/ exists (that tree has names with spaces
# that break an unquoted --preload-file glob). Override only if File/ is missing.
set(_SKER_EXCEL_DEFAULT "")
if(EXISTS "${SK_ROOT}/File/Budget.sker")
	set(_SKER_EXCEL_DEFAULT "${SK_ROOT}/File")
endif()
set(SKER_WASM_PRELOAD_EXCEL_DIR "${_SKER_EXCEL_DEFAULT}" CACHE PATH "Folder of .sker/.json fixtures; WASM link adds --preload-file into MEMFS")

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
		${SK_LIBRARY}/SkFormat/include
		${SK_LIBRARY}/SkSpreadSheet/include
		${SK_THIRD_PARTY}/rapidjson/include
		${SK_THIRD_PARTY}/cppunit/include
		${SK_THIRD_PARTY}/cppunit/${SK_PLATFORM}
		${SK_THIRD_PARTY}/cppunit/${SK_PLATFORM}/include
		${SK_THIRD_PARTY}/cppunit-wasm
)

set(SK_OUTPUT "${SK_ROOT}/${SK_PLATFORM}")
# wasm64 tests build/link/output under sker/wasm64 (matching 64-bit libs).
if(DEFINED SK_WASM_OUTPUT_SUBDIR AND NOT "${SK_WASM_OUTPUT_SUBDIR}" STREQUAL "")
	set(SK_OUTPUT "${SK_ROOT}/${SK_WASM_OUTPUT_SUBDIR}")
endif()

## Pour Visual studio et Xcode les repertoires Debug et Release sont traités automatiquement
set(CMAKE_LIB_OUTPUT_DIRECTORY ${SK_OUTPUT}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${SK_OUTPUT}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${SK_OUTPUT}/bin)

## Add executable =============================================================
add_executable(${CURRENT_PROJECT}
	${INCLUDEFILES}
	${SRCFILES}
)

# TestMultiUser off by default; enable with -DSK_SPREADSHEET_TEST_MULTI_USER=ON (see compilSpreadSheet.sh).
# WASM builds always OFF (multi-user tests run on native/Xcode only).
option(SK_SPREADSHEET_TEST_MULTI_USER "SkSpreadSheet_test: in-process multi-user tests" OFF)
if(SK_PLATFORM STREQUAL "wasm")
	set(SK_SPREADSHEET_TEST_MULTI_USER OFF CACHE BOOL "SkSpreadSheet_test: in-process multi-user tests" FORCE)
endif()
if(SK_SPREADSHEET_TEST_MULTI_USER)
	target_compile_definitions(${CURRENT_PROJECT} PRIVATE TestMultiUser)
	message(STATUS "${CURRENT_PROJECT}: TestMultiUser enabled")
endif()

# Native tests open workbooks from <repo>/File/ (SKER_EXCEL_TEST_DIR still wins at runtime).
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

set(SK_SEARCH_LIB ${SK_OUTPUT}/lib)

if (${SK_PLATFORM} STREQUAL windows)
## link with library ========================================================== 	
	set(PROJECT_LIBRARY 
		Rpcrt4.lib
		Shlwapi.lib
		debug ${SK_SEARCH_LIB}/Debug/SkRoot.lib optimized ${SK_SEARCH_LIB}/Release/SkRoot.lib
		debug ${SK_SEARCH_LIB}/Debug/SkFormat.lib optimized ${SK_SEARCH_LIB}/Release/SkFormat.lib
		debug ${SK_SEARCH_LIB}/Debug/SkSpreadSheet.lib optimized ${SK_SEARCH_LIB}/Release/SkSpreadSheet.lib
		debug ${SK_THIRD_PARTY}/cppunit/windows/Debug/cppunit.lib optimized ${SK_THIRD_PARTY}/cppunit/windows/Release/cppunit.lib 
	)
elseif(${SK_PLATFORM} STREQUAL unix)
	set(PROJECT_LIBRARY 
		${SK_SEARCH_LIB}/libSkSpreadSheet.a
		${SK_SEARCH_LIB}/libSkFormat.a
		${SK_SEARCH_LIB}/libSkRoot.a
		${SK_THIRD_PARTY}/cppunit/unix/libcppunit.a
	)
elseif(${SK_PLATFORM} STREQUAL wasm)
	set(CMAKE_CXX_COMPILER "emcc")
	include("${SK_ROOT}/cmake/SkWasmCompil.cmake")
	message("Node -->" $ENV{SK_NODE})
	message("Compil Mode -->" ${SK_COMPIL_MODE})

	# 64-bit tests link wasm64 cppunit (ABI match) + carry -sMEMORY64.
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
		# Native Wasm EH is incompatible with -sDISABLE_EXCEPTION_CATCHING (JS EH).
		set(_SK_TEST_EXC_CATCH "")
	else()
		set(SK_CPPUNIT_WASM_DIR "${SK_THIRD_PARTY}/cppunit/wasm")
		set(_SK_TEST_MEM64_LINK "")
		set(_SK_TEST_MAX_MEMORY "4GB")
		set(_SK_TEST_EXC_CATCH "-s DISABLE_EXCEPTION_CATCHING=0")
	endif()
	# Emscripten link: clearer aborts than "Aborted(undefined)"; stack size matches SkReactSpreadSheet (large workbooks).
	set(SK_TEST_WASM_ASSERTIONS "1" CACHE STRING "SkSpreadSheet_test wasm: -sASSERTIONS when SK_COMPIL=RELEASE (0|1|2)")
	set(SK_TEST_WASM_STACK_SIZE "8388608" CACHE STRING "SkSpreadSheet_test wasm: -sSTACK_SIZE bytes")
	if(SK_WASM_RELEASE)
		set(_SK_TEST_WASM_ASSERT_LEVEL "${SK_TEST_WASM_ASSERTIONS}")
	else()
		set(_SK_TEST_WASM_ASSERT_LEVEL "2")
	endif()
	# Set library paths for WASM
	set(PROJECT_LIBRARY 
		${SK_SEARCH_LIB}/libSkRoot.a 
		${SK_SEARCH_LIB}/libSkSpreadSheet.a 
		${SK_SEARCH_LIB}/libSkFormat.a 
	)
	
	if("$ENV{SK_NODE}" STREQUAL "1")
		message("Compil with Node --")
		if(SK_WASM_RELEASE)
			add_definitions(-DSK_RELEASE)
			# Set library paths for WASM
			set(PROJECT_LIBRARY 
				${SK_CPPUNIT_WASM_DIR}/Release/libcppunit.a
			)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -O3")
		else()
			# Set library paths for WASM
			set(PROJECT_LIBRARY 
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
			-s ASSERTIONS=${_SK_TEST_WASM_ASSERT_LEVEL} \
			-s STACK_SIZE=${SK_TEST_WASM_STACK_SIZE} \
			-o SkReactSpreadSheet.js \
			--source-map \
			--source-map-name=SkSpreadSheet.wasm.map \
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
				${_SK_TEST_EXC_CATCH} \
				-s ASSERTIONS=${_SK_TEST_WASM_ASSERT_LEVEL} \
				-s STACK_SIZE=${SK_TEST_WASM_STACK_SIZE} \
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
				${_SK_TEST_EXC_CATCH} \
				-s ASSERTIONS=${_SK_TEST_WASM_ASSERT_LEVEL} \
				-s STACK_SIZE=${SK_TEST_WASM_STACK_SIZE} \
				--source-map \
				--source-map-name=SkReactSpreadSheet.wasm.map \
			")
		endif()
	endif()
elseif(${SK_PLATFORM} STREQUAL xcode)
	set(PROJECT_LIBRARY 
	debug libz.a optimized libz.a
	debug libbz2.a optimized libbz2.a
	debug ${SK_SEARCH_LIB}/Debug/libSkRoot.a optimized  ${SK_SEARCH_LIB}/Release/libSkRoot.a
	debug ${SK_SEARCH_LIB}/Debug/libSkSpreadSheet.a optimized  ${SK_SEARCH_LIB}/Release/libSkSpreadSheet.a
	debug ${SK_SEARCH_LIB}/Debug/libSkFormat.a optimized ${SK_SEARCH_LIB}/Release/libSkFormat.a
	debug ${SK_THIRD_PARTY}/cppunit/xcode/Debug/libcppunit.a optimized ${SK_THIRD_PARTY}/cppunit/xcode/Release/libcppunit.a
	)
endif()

target_link_libraries(${CURRENT_PROJECT} ${PROJECT_LIBRARY} )

# Emscripten: preload the whole fixtures directory (one mapping, no glob). File names
# with spaces (Calendrier sur 12 mois1.sker) stay inside the packager; they must not
# appear as separate emcc argv tokens. Prefer <repo>/File over a stale cache that
# still points at ~/Projects/Excel (Book 12.sker used to split --preload-file).
if(${SK_PLATFORM} STREQUAL wasm)
	set(_SKER_PRELOAD_EXCEL_DIR "")
	if(EXISTS "${SK_ROOT}/File/Budget.sker")
		set(_SKER_PRELOAD_EXCEL_DIR "${SK_ROOT}/File")
	elseif(NOT "${SKER_WASM_PRELOAD_EXCEL_DIR}" STREQUAL "" AND EXISTS "${SKER_WASM_PRELOAD_EXCEL_DIR}")
		set(_SKER_PRELOAD_EXCEL_DIR "${SKER_WASM_PRELOAD_EXCEL_DIR}")
	endif()
	if(NOT _SKER_PRELOAD_EXCEL_DIR STREQUAL "")
		set(_SKER_WASM_PRELOAD_MEMFS "/home/web_user/Projects/Excel")
		target_link_options(${CURRENT_PROJECT} PRIVATE
			"SHELL:--preload-file \"${_SKER_PRELOAD_EXCEL_DIR}@${_SKER_WASM_PRELOAD_MEMFS}\"")
		message(STATUS "WASM preload TestSkExcel: ${_SKER_PRELOAD_EXCEL_DIR} -> MEMFS ${_SKER_WASM_PRELOAD_MEMFS}")
	endif()
endif()

## Compiler specific ===========================================================
if (${SK_PLATFORM} STREQUAL windows)
# Set Runtime library
	if ($ENV{SKLINK} STREQUAL "DYNAMIC") 
	message("<-LINK DYNAMIC->" )
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")		
		set(CMAKE_CXX_FLAGS_RELEASE "/MDd")
	else()
		set(CMAKE_CXX_FLAGS_RELEASE "/MD")
	endif()	
	elseif()
# Set Static runtime library
	message("<-LINK STATIC->" )
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")		
		set(CMAKE_CXX_FLAGS_RELEASE "/MTd")
	else()
		set(CMAKE_CXX_FLAGS_RELEASE "/MT")
	endif()	
	endif() 
#   Level of Warning W3
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
message( "INCLUDEFILES->" ${INCLUDEFILES})
message( "SRCFILES->" ${SRCFILES})
message( "SK_OUTPUT->" ${SK_OUTPUT})

