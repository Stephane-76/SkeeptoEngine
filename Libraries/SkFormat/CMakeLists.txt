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
set(CURRENT_PROJECT SkFormat)
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
# wasm64 gets its own output tree (sker/wasm64) so wasm32 and wasm64 .a never
# clobber each other in a shared lib dir (mixing ABIs => wasm-ld errors). Driven
# by -DSK_WASM_OUTPUT_SUBDIR from compilWasm.sh (empty => sker/wasm unchanged).
if(DEFINED SK_WASM_OUTPUT_SUBDIR AND NOT "${SK_WASM_OUTPUT_SUBDIR}" STREQUAL "")
	set(SK_OUTPUT "${SK_ROOT}/${SK_WASM_OUTPUT_SUBDIR}")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${SK_OUTPUT}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${SK_OUTPUT}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${SK_OUTPUT}/bin)
include("${SK_ROOT}/cmake/SkEnsureOutput.cmake")


# Add source files ============================================================
file(GLOB_RECURSE INCLUDEFILES "./include/*.hpp")
file(GLOB_RECURSE SRCFILES "./source/*.cpp")

set(SK_LIBRARY "${SK_ROOT}/Libraries")
set(SK_THIRD_PARTY "${SK_ROOT}/third-party")

include_directories(
		${SK_LIBRARY}/SkRoot/include
		${SK_THIRD_PARTY}/rapidjson/include
		${SK_THIRD_PARTY}/rapidxml
)

## Add librairy =============================================================== 
add_library(${CURRENT_PROJECT}
 	${INCLUDEFILES}
	${SRCFILES}
)

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
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++20 -Wunused-variable -Wsign-compare")
elseif(${SK_PLATFORM} STREQUAL wasm)
	set(CMAKE_CXX_COMPILER "emcc")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
	include("${SK_ROOT}/cmake/SkWasmCompil.cmake")
	if(SK_WASM_RELEASE)
		add_definitions(-DSK_RELEASE)
		message("<-MAKE RELEASE->" )
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -O3")
	else()
		message("<-MAKE DEBUG->" )
		get_filename_component(SK_ROOT_ABS "${SK_ROOT}" REALPATH)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -g3 -O0 -ffile-prefix-map=${SK_ROOT_ABS}/= -fdebug-compilation-dir=${SK_ROOT_ABS}")
	endif()
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
