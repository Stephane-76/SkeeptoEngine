# SKLibrary Make ==============================================================
cmake_minimum_required(VERSION 3.16)

# Set Name and version ========================================================
set(CURRENT_PROJECT SkPressureSp)
project(${CURRENT_PROJECT}  CXX)

# specify the C++ standard ====================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Get Root Directory ==========================================================
# Resolve the engine repo root from this file's location (parent of SkPressureSp),
# like SkReactSpreadSheet and the Libraries_test targets. Artifacts therefore go
# under <repo>/<platform> (e.g. xcode/bin, wasm/bin; wasm64 via SK_WASM_OUTPUT_SUBDIR).
set(SK_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
cmake_path(GET SK_DIRECTORY PARENT_PATH SK_ROOT)

message("Root->" ${SK_ROOT})
message("Platform->" ${SK_PLATFORM})

## for  Visual studio and Xcode :
## the directory  Debug and Release are configurated by compiler =============
set(SK_OUTPUT "${SK_ROOT}/${SK_PLATFORM}")
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
	)
elseif(${SK_PLATFORM} STREQUAL unix)
	set(PROJECT_LIBRARY 
	libstdc++.a
	${SK_SEARCH_LIB}/libSkRoot.a 
	${SK_SEARCH_LIB}/libSkSpreadSheet.a
)
elseif(${SK_PLATFORM} STREQUAL wasm)
	set(PROJECT_LIBRARY 
	${SK_SEARCH_LIB}/libSkRoot.a 
	${SK_SEARCH_LIB}/libSkSpreadSheet.a 
	${SK_SEARCH_LIB}/libSkFormat.a 
	)
	elseif(${SK_PLATFORM} STREQUAL xcode)
	set(PROJECT_LIBRARY 
	debug ${SK_SEARCH_LIB}/Debug/libSkRoot.a optimized  ${SK_SEARCH_LIB}/Release/libSkRoot.a
	debug ${SK_SEARCH_LIB}/Debug/libSkSpreadSheet.a optimized  ${SK_SEARCH_LIB}/Release/libSkSpreadSheet.a
	)
endif()


target_link_libraries(${CURRENT_PROJECT} ${PROJECT_LIBRARY} )

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
	elseif()
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
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++2a -lpthread")
elseif(${SK_PLATFORM} STREQUAL wasm)
	set(CMAKE_CXX_COMPILER "emcc")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++20")
	# Shared wasm settings (DEBUG/RELEASE via SK_COMPIL, Memory64 via SK_64Bit).
	# Adds -m64 + native Wasm EH/longjmp to CMAKE_CXX_FLAGS when 64-bit; exposes
	# SK_WASM_RELEASE / SK_WASM_MEMORY64 / SK_WASM_MEM64_LINK_FLAGS.
	include("${SK_ROOT}/cmake/SkWasmCompil.cmake")
	message("Compil Mode -->" ${SK_COMPIL_MODE})

	# Default Emscripten stack is small; large sheets / RecalculateAll can overflow it.
	set(SK_PRESSURE_WASM_STACK_SIZE "8388608" CACHE STRING "SkPressureSp wasm: -sSTACK_SIZE bytes (default 8MiB)")
	if(SK_WASM_MEMORY64)
		# wasm64: raise the ceiling above 4GiB. Native Wasm EH is incompatible with
		# -sDISABLE_EXCEPTION_CATCHING; preallocate some heap to dodge memory64 grow bugs.
		set(_SK_PRESSURE_MAX_MEMORY "8GB")
		set(_SK_PRESSURE_MEM64_LINK "${SK_WASM_MEM64_LINK_FLAGS} -sINITIAL_MEMORY=268435456")
		set(_SK_PRESSURE_EXC_CATCH "")
	else()
		set(_SK_PRESSURE_MAX_MEMORY "4GB")
		set(_SK_PRESSURE_MEM64_LINK "")
		set(_SK_PRESSURE_EXC_CATCH "-s DISABLE_EXCEPTION_CATCHING=0")
	endif()
	# ASSERTIONS: verbose (2) in DEBUG for clearer aborts; RELEASE keeps 1 (override with -DSK_PRESSURE_WASM_ASSERTIONS).
	set(SK_PRESSURE_WASM_ASSERTIONS "1" CACHE STRING "SkPressureSp wasm (RELEASE): -sASSERTIONS (0|1|2)")
	if(SK_WASM_RELEASE)
		add_definitions(-DSK_RELEASE)
		set(_SK_PRESSURE_ASSERT_LEVEL "${SK_PRESSURE_WASM_ASSERTIONS}")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
	else()
		set(_SK_PRESSURE_ASSERT_LEVEL "2")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -gsource-map -O0")
	endif()

	set_property(TARGET ${CURRENT_PROJECT} PROPERTY LINK_FLAGS "\
		-lembind \
		-s WASM=1 \
		-s ERROR_ON_UNDEFINED_SYMBOLS=0 \
		-s EXPORTED_FUNCTIONS=_main,_Launch \
		-s ALLOW_MEMORY_GROWTH \
		-s STACK_SIZE=${SK_PRESSURE_WASM_STACK_SIZE} \
		-s MAXIMUM_MEMORY=${_SK_PRESSURE_MAX_MEMORY} ${_SK_PRESSURE_MEM64_LINK} \
		${_SK_PRESSURE_EXC_CATCH} \
		-s ASSERTIONS=${_SK_PRESSURE_ASSERT_LEVEL} \
		--source-map \
		--source-map-name=SkPressureSp.wasm.map \
	")
elseif(${SK_PLATFORM} STREQUAL xcode)	
	set(CMAKE_OSX_DEPLOYMENT_TARGET 15.0)	
	set(PROJECT_LIBRARY 
	debug libz.a optimized libz.a
	debug libbz2.a optimized libbz2.a
	debug ${SK_SEARCH_LIB}/Debug/libSkRoot.a optimized  ${SK_SEARCH_LIB}/Release/libSkRoot.a
	debug ${SK_SEARCH_LIB}/Debug/libSkSpreadSheet.a optimized  ${SK_SEARCH_LIB}/Release/libSkSpreadSheet.a
	debug ${SK_THIRD_PARTY}/cppunit/xcode/Debug/libcppunit.a optimized ${SK_THIRD_PARTY}/cppunit/xcode/Release/libcppunit.a
	)
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

