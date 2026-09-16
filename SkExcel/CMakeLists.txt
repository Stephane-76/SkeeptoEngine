# SKLibrary Make - SkExcel PugiXML Reader =====================================
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
set(CURRENT_PROJECT SkExcel)
project(${CURRENT_PROJECT}  CXX)

# Enable parallel builds for better performance
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_UNITY_BUILD OFF)  # Disable unity builds to allow better parallelization

# specify the C++ standard ====================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

## Recupere le Compilateur =====================================================
message("Generator ->" ${CMAKE_CXX_COMPILER_ID})
# Honor -DSK_PLATFORM from the superbuild / compil*.sh. Guess only when unset.
# Do not map AppleClang -> xcode: Unix Makefiles on macOS is SK_PLATFORM=unix
# (AppleClang is not STREQUAL Clang).
if(NOT SK_PLATFORM)
	if(EMSCRIPTEN)
		set(SK_PLATFORM "wasm")
	elseif(MSVC OR CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		set(SK_PLATFORM "windows")
	elseif(CMAKE_GENERATOR STREQUAL "Xcode")
		set(SK_PLATFORM "xcode")
	else()
		set(SK_PLATFORM "unix")
	endif()
endif()
message("Platform->" ${SK_PLATFORM})

# Resolve the sker repo root from this file's location (parent of SkExcel),
# like SkPressureSp. Do not trust a stale -DROOT (e.g. a legacy C++ tree).
set(SK_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
cmake_path(GET SK_DIRECTORY PARENT_PATH SK_ROOT)
set(ROOT "${SK_ROOT}")
message("Root->" ${SK_ROOT})

set(SK_LIBRARY "${SK_ROOT}/Libraries")
set(SK_THIRD_PARTY "${SK_ROOT}/third-party")

# Dependency for libzip
# Detect libzip location based on platform (skip Homebrew paths for Emscripten)
if(EMSCRIPTEN)
	set(SK_LIBZIP "${SK_THIRD_PARTY}/libzip/wasm-install")
elseif(EXISTS "${SK_THIRD_PARTY}/libzip/windows-install/include/zip.h")
	# Windows: third-party/libzip/compilWindows.cmd
	set(SK_LIBZIP "${SK_THIRD_PARTY}/libzip/windows-install")
	set(SK_ZLIB "${SK_THIRD_PARTY}/libzip/zlib-install")
elseif(EXISTS "/opt/homebrew/opt/libzip")
    # macOS with Homebrew
    set(SK_LIBZIP "/opt/homebrew/opt/libzip")
elseif(EXISTS "/usr/lib/x86_64-linux-gnu/libzip.a" OR EXISTS "/usr/lib/libzip.so")
    # Linux (Docker/Ubuntu) - libzip is in system paths
    set(SK_LIBZIP "/usr")
elseif(EXISTS "${SK_THIRD_PARTY}/libzip")
	set(SK_LIBZIP "${SK_THIRD_PARTY}/libzip")
else()
    # Try to find libzip via pkg-config
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(LIBZIP QUIET libzip)
        if(LIBZIP_FOUND)
            set(SK_LIBZIP "${LIBZIP_PREFIX}")
        else()
            set(SK_LIBZIP "/usr")
        endif()
    else()
        set(SK_LIBZIP "/usr")
    endif()
endif()
message("SK_LIBZIP -> ${SK_LIBZIP}")

if(EMSCRIPTEN)
	include_directories(
		${SK_LIBRARY}/SkRoot/include
		${SK_LIBRARY}/SkFormat/include
		${SK_LIBRARY}/SkSpreadSheet/include
		${SK_THIRD_PARTY}/rapidjson/include
		${SK_THIRD_PARTY}/pugixml/src
		${SK_THIRD_PARTY}/libzip/wasm-install/include
		./include
	)
else()
	include_directories(
		/usr/local/include/OpenXLSX
		/usr/local/include
		${SK_LIBRARY}/SkRoot/include
		${SK_LIBRARY}/SkFormat/include
		${SK_LIBRARY}/SkSpreadSheet/include
		${SK_THIRD_PARTY}/rapidjson/include
		${SK_THIRD_PARTY}/pugixml/src
		/opt/homebrew/include
		${SK_LIBZIP}/include
		./include
	)
endif()
if(DEFINED SK_ZLIB)
	include_directories(${SK_ZLIB}/include)
endif()

# Fix output directory for Xcode builds
if(CMAKE_GENERATOR STREQUAL "Xcode")
    # Use the main xcode directory for both libraries and binaries
    set(SK_OUTPUT "${ROOT}/${SK_PLATFORM}")
    set(SK_SEARCH_LIB "${ROOT}/${SK_PLATFORM}/lib")
else()
    set(SK_OUTPUT "${ROOT}/${SK_PLATFORM}")
    set(SK_SEARCH_LIB ${SK_OUTPUT}/lib)
endif()

## Pour Visual studio et Xcode les repertoires Debug et Release sont traités automatiquement
set(CMAKE_LIB_OUTPUT_DIRECTORY ${SK_OUTPUT}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${SK_OUTPUT}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${SK_OUTPUT}/bin)
include("${ROOT}/cmake/SkEnsureOutput.cmake")

## Add executable =============================================================
file(GLOB_RECURSE PROJECT_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp")
file(GLOB_RECURSE PROJECT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

# Windows has no Homebrew/system pugixml; compile the third-party source into SkExcel.
if(MSVC AND EXISTS "${SK_THIRD_PARTY}/pugixml/src/pugixml.cpp")
	list(APPEND PROJECT_SOURCES "${SK_THIRD_PARTY}/pugixml/src/pugixml.cpp")
endif()

add_executable(${CURRENT_PROJECT}
	${PROJECT_SOURCES}
	${PROJECT_HEADERS}
)

# For Xcode, organize files in groups that match folder structure
if(CMAKE_GENERATOR STREQUAL "Xcode")
    # Create groups for include and source folders
    set_property(TARGET ${CURRENT_PROJECT} PROPERTY FOLDER "SkExcel")
    
    # Enable parallel builds in Xcode
    set_property(TARGET ${CURRENT_PROJECT} PROPERTY XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "NO")
    set_property(TARGET ${CURRENT_PROJECT} PROPERTY XCODE_ATTRIBUTE_CLANG_WARN_QUOTED_INCLUDE_IN_FRAMEWORK_HEADER "NO")
    
    # Add headers to include group
    foreach(header ${PROJECT_HEADERS})
        set_source_files_properties(${header} PROPERTIES 
            XCODE_FILE_ATTRIBUTES "Public"
            XCODE_GROUP "include"
        )
    endforeach()
    
    # Add sources to source group  
    foreach(source ${PROJECT_SOURCES})
        set_source_files_properties(${source} PROPERTIES
            XCODE_FILE_ATTRIBUTES "Public" 
            XCODE_GROUP "source"
        )
    endforeach()
endif()

# Make include paths visible to Xcode/target
if(EMSCRIPTEN)
	target_include_directories(${CURRENT_PROJECT}
		PRIVATE
		${SK_LIBRARY}/SkRoot/include
		${SK_LIBRARY}/SkFormat/include
		${SK_LIBRARY}/SkSpreadSheet/include
		${SK_THIRD_PARTY}/rapidjson/include
		${SK_THIRD_PARTY}/pugixml/src
		${SK_THIRD_PARTY}/libzip/wasm-install/include
		${CMAKE_CURRENT_SOURCE_DIR}/include
	)
else()
	target_include_directories(${CURRENT_PROJECT}
		PRIVATE
		/usr/local/include/OpenXLSX
		/usr/local/include
		${SK_LIBRARY}/SkRoot/include
		${SK_LIBRARY}/SkFormat/include
		${SK_LIBRARY}/SkSpreadSheet/include
		${SK_THIRD_PARTY}/rapidjson/include
		${SK_THIRD_PARTY}/pugixml/src
		/opt/homebrew/include
		${SK_LIBZIP}/include
		${CMAKE_CURRENT_SOURCE_DIR}/include
	)
	if(DEFINED SK_ZLIB)
		target_include_directories(${CURRENT_PROJECT} PRIVATE ${SK_ZLIB}/include)
	endif()
endif()

# Emscripten: SkTypes.hpp layout/macros must match linked libSkRoot/SkFormat/SkSpreadSheet.a.
# Release: SkExcel defines SK_RELEASE — build the three .a with SK_COMPIL=RELEASE.
# Debug: SkExcel omits SK_RELEASE — rebuild those .a with SK_COMPIL=DEBUG (same as compilWasm.sh debug).
if(EMSCRIPTEN)
	target_compile_definitions(${CURRENT_PROJECT} PRIVATE $<$<NOT:$<CONFIG:Debug>>:SK_RELEASE>)
	# C++ exceptions: must stay ENABLED for this target. Linked libSk*.a (wasm) are built with exceptions
	# (em++ default); using -fno-exceptions here breaks the link (__cxa_throw undefined / DISABLE_EXCEPTION_THROWING).
	# -sASSERTIONS: clearer messages than bare "RuntimeError: Aborted()" (see Emscripten docs).
	# Override: cmake -DSKEXCEL_WASM_ASSERTIONS=0|1|2 ...  (2 = most verbose at abort)
	set(SKEXCEL_WASM_ASSERTIONS "1" CACHE STRING "SkExcel wasm: Emscripten -sASSERTIONS (0=off, 1=default, 2=verbose abort)")
	# Stack size for the WASM conversion pipeline. 16MiB gives ample headroom for the
	# deep-recursive xlsx->api conversion while keeping the module lean (ALLOW_MEMORY_GROWTH
	# lets the heap grow on demand). Bump via -DSKEXCEL_WASM_STACK_SIZE if a huge workbook overflows.
	set(SKEXCEL_WASM_STACK_SIZE "16777216" CACHE STRING "SkExcel wasm: -sSTACK_SIZE in bytes (default 16MiB)")
	# INITIAL_MEMORY must be larger than STACK_SIZE; 32MiB leaves headroom above the 16MiB stack
	# and grows as needed (ALLOW_MEMORY_GROWTH=1). ASan runs need much more: pass -DSKEXCEL_WASM_INITIAL_MEMORY.
	set(SKEXCEL_WASM_INITIAL_MEMORY "33554432" CACHE STRING "SkExcel wasm: -sINITIAL_MEMORY in bytes (default 32MiB)")
	set(SKEXCEL_WASM_MAX_MEMORY "2147483648" CACHE STRING "SkExcel wasm: -sMAXIMUM_MEMORY in bytes (default 2GiB)")
	# Debug wasm: -O0 -g + ASSERTIONS=2 (when CMAKE_BUILD_TYPE=Debug or multi-config Debug)
	set(SKEXCEL_WASM_FORCE_DEBUG_FLAGS OFF CACHE BOOL "SkExcel wasm: always add -O0 -g (use if single-config generator omits Debug)")
	# Symbolicated Release: keep -O2 speed but add DWARF names (-g) + ASSERTIONS=2 to get
	# real C++ function names in abort/crash stack traces (Debug -O0 is too slow on huge xlsx).
	set(SKEXCEL_WASM_SYMBOLS OFF CACHE BOOL "SkExcel wasm: add -g (DWARF) + ASSERTIONS=2 without forcing -O0")
	# AddressSanitizer: pinpoints heap buffer overflow / invalid free with allocation stack.
	# Diagnostic-only (slow, more memory); needs -O1 for readable frames. Requires a bigger
	# initial memory (ASan shadow); pair with -DSKEXCEL_WASM_INITIAL_MEMORY when configuring.
	set(SKEXCEL_WASM_ASAN OFF CACHE BOOL "SkExcel wasm: build with -fsanitize=address (diagnostics)")
	# In-process library variant: build a reusable module (MODULARIZE factory) exposing
	# skexcel_convert() so a long-lived Node server can convert many files on ONE wasm instance
	# (no per-request process spawn / module instantiate). OFF => normal auto-run CLI (SkExcel.js).
	set(SKEXCEL_WASM_LIB OFF CACHE BOOL "SkExcel wasm: build reusable in-process module (SkExcelLib.js) instead of auto-run CLI")
endif()

## link with library ========================================================== 
# SK_SEARCH_LIB is already set above based on platform

if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
## link with library ========================================================== 
	set(SK_SEARCH_LIB ${SK_OUTPUT}/lib)
	set(PROJECT_LIBRARY 
		Rpcrt4.lib
		Shlwapi.lib
		debug ${SK_SEARCH_LIB}/Debug/SkRoot.lib optimized ${SK_SEARCH_LIB}/Release/SkRoot.lib
		debug ${SK_SEARCH_LIB}/Debug/SkFormat.lib optimized ${SK_SEARCH_LIB}/Release/SkFormat.lib
		debug ${SK_SEARCH_LIB}/Debug/SkSpreadSheet.lib optimized ${SK_SEARCH_LIB}/Release/SkSpreadSheet.lib
	)
	if(EXISTS "${SK_LIBZIP}/lib/zip.lib")
		list(APPEND PROJECT_LIBRARY ${SK_LIBZIP}/lib/zip.lib)
	elseif(EXISTS "${SK_LIBZIP}/lib/libzip.lib")
		list(APPEND PROJECT_LIBRARY ${SK_LIBZIP}/lib/libzip.lib)
	endif()
	if(DEFINED SK_ZLIB)
		if(EXISTS "${SK_ZLIB}/lib/zlibstatic.lib")
			list(APPEND PROJECT_LIBRARY ${SK_ZLIB}/lib/zlibstatic.lib)
		elseif(EXISTS "${SK_ZLIB}/lib/libzs.lib")
			list(APPEND PROJECT_LIBRARY ${SK_ZLIB}/lib/libzs.lib)
		elseif(EXISTS "${SK_ZLIB}/lib/zlib.lib")
			list(APPEND PROJECT_LIBRARY ${SK_ZLIB}/lib/zlib.lib)
		elseif(EXISTS "${SK_ZLIB}/lib/libz.lib")
			list(APPEND PROJECT_LIBRARY ${SK_ZLIB}/lib/libz.lib)
		endif()
	endif()
elseif(EMSCRIPTEN)
	# WASM: libzip built by third-party/libzip/compilWasm.sh; pugixml by third-party/pugixml/compilWasm.sh
	set(LIBZIP_LIB "${SK_THIRD_PARTY}/libzip/wasm-install/lib/libzip.a")
	set(PUGIXML_LIB "${SK_THIRD_PARTY}/pugixml/wasm/libpugixml.a")
	if(NOT EXISTS "${LIBZIP_LIB}")
		message(FATAL_ERROR "WASM libzip not found. Configure from the repo root so CMake builds it:\n  cmake -B build-wasm -DSK_PLATFORM=wasm\nExpected: ${LIBZIP_LIB}")
	endif()
	if(NOT EXISTS "${PUGIXML_LIB}")
		message(FATAL_ERROR "WASM pugixml not found. Configure from the repo root so CMake builds it:\n  cmake -B build-wasm -DSK_PLATFORM=wasm\nExpected: ${PUGIXML_LIB}")
	endif()
elseif(${SK_PLATFORM} STREQUAL unix)
	# Flat unix/lib/*.a (no Debug/Release). AppleClang is unix when SK_PLATFORM=unix.
	if(EXISTS "${SK_LIBZIP}/lib/libzip.a")
		set(LIBZIP_LIB "${SK_LIBZIP}/lib/libzip.a")
	elseif(EXISTS "${SK_LIBZIP}/lib/libzip.dylib")
		set(LIBZIP_LIB "${SK_LIBZIP}/lib/libzip.dylib")
	elseif(EXISTS "${SK_LIBZIP}/lib/x86_64-linux-gnu/libzip.a")
		set(LIBZIP_LIB "${SK_LIBZIP}/lib/x86_64-linux-gnu/libzip.a")
	elseif(EXISTS "/usr/lib/x86_64-linux-gnu/libzip.a")
		set(LIBZIP_LIB "/usr/lib/x86_64-linux-gnu/libzip.a")
	elseif(EXISTS "/usr/lib/libzip.a")
		set(LIBZIP_LIB "/usr/lib/libzip.a")
	else()
		set(LIBZIP_LIB "zip")
	endif()
	if(EXISTS "/opt/homebrew/lib/libpugixml.a")
		set(PUGIXML_LIB "/opt/homebrew/lib/libpugixml.a")
	elseif(EXISTS "/usr/local/lib/libpugixml.a")
		set(PUGIXML_LIB "/usr/local/lib/libpugixml.a")
	elseif(EXISTS "${SK_THIRD_PARTY}/pugixml/build/libpugixml.a")
		set(PUGIXML_LIB "${SK_THIRD_PARTY}/pugixml/build/libpugixml.a")
	else()
		set(PUGIXML_LIB "pugixml")
	endif()
	# Order: dependents before dependencies (Apple ld has no --start-group).
	set(PROJECT_LIBRARY 
		${SK_SEARCH_LIB}/libSkSpreadSheet.a 
		${SK_SEARCH_LIB}/libSkFormat.a
		${SK_SEARCH_LIB}/libSkRoot.a
		${LIBZIP_LIB}
		${PUGIXML_LIB}
	)
elseif(${SK_PLATFORM} STREQUAL xcode)
	set(PROJECT_LIBRARY 
		debug ${SK_SEARCH_LIB}/Debug/libSkRoot.a optimized  ${SK_SEARCH_LIB}/Release/libSkRoot.a
		debug ${SK_SEARCH_LIB}/Debug/libSkSpreadSheet.a optimized  ${SK_SEARCH_LIB}/Release/libSkSpreadSheet.a
		debug ${SK_SEARCH_LIB}/Debug/libSkFormat.a optimized  ${SK_SEARCH_LIB}/Release/libSkFormat.a
		debug ${SK_LIBZIP}/lib/libzip.dylib optimized ${SK_LIBZIP}/lib/libzip.dylib 
		/opt/homebrew/lib/libpugixml.a
	)
else()
	# For pugixml in other platforms
	if(EXISTS "/usr/local/lib/libpugixml.a")
		set(PUGIXML_LIB "/usr/local/lib/libpugixml.a")
	elseif(EXISTS "${SK_THIRD_PARTY}/pugixml/build/libpugixml.a")
		set(PUGIXML_LIB "${SK_THIRD_PARTY}/pugixml/build/libpugixml.a")
	else()
		set(PUGIXML_LIB "pugixml")
	endif()
	set(PROJECT_LIBRARY 
		debug ${SK_SEARCH_LIB}/Debug/libSkSpreadSheet.a optimized  ${SK_SEARCH_LIB}/Release/libSkSpreadSheet.a
		debug ${SK_SEARCH_LIB}/Debug/libSkFormat.a optimized  ${SK_SEARCH_LIB}/Release/libSkFormat.a
		debug ${SK_LIBZIP}/lib/libzip.a optimized ${SK_LIBZIP}/lib/libzip.a 
		${PUGIXML_LIB}
	)
endif()

# Link libraries
# For static libraries on Linux/Clang, use --start-group/--end-group
# to handle circular dependencies between SkRoot, SkFormat, and SkSpreadSheet
if(EMSCRIPTEN)
	target_link_libraries(${CURRENT_PROJECT}
		"-Wl,--start-group"
		${SK_SEARCH_LIB}/libSkSpreadSheet.a
		${SK_SEARCH_LIB}/libSkFormat.a
		${SK_SEARCH_LIB}/libSkRoot.a
		"-Wl,--end-group"
		${LIBZIP_LIB}
		${PUGIXML_LIB}
	)
	# zlib: Emscripten port (required by libzip)
	# NODERAWFS: under Node.js, map real paths (e.g. /Users/...) to the host FS; MEMFS has no host files by default
	target_link_options(${CURRENT_PROJECT} PRIVATE
		"SHELL:-sUSE_ZLIB=1"
		"SHELL:-sALLOW_MEMORY_GROWTH=1"
		"SHELL:-sINITIAL_MEMORY=${SKEXCEL_WASM_INITIAL_MEMORY}"
		"SHELL:-sMAXIMUM_MEMORY=${SKEXCEL_WASM_MAX_MEMORY}"
		"SHELL:-sNODERAWFS=1"
		"$<$<OR:$<CONFIG:Debug>,$<BOOL:${SKEXCEL_WASM_FORCE_DEBUG_FLAGS}>>:SHELL:-O0>"
		"$<$<OR:$<CONFIG:Debug>,$<BOOL:${SKEXCEL_WASM_FORCE_DEBUG_FLAGS}>,$<BOOL:${SKEXCEL_WASM_SYMBOLS}>>:SHELL:-g>"
		"$<$<OR:$<CONFIG:Debug>,$<BOOL:${SKEXCEL_WASM_FORCE_DEBUG_FLAGS}>,$<BOOL:${SKEXCEL_WASM_SYMBOLS}>>:SHELL:-sASSERTIONS=2>"
		"$<$<AND:$<NOT:$<CONFIG:Debug>>,$<NOT:$<BOOL:${SKEXCEL_WASM_FORCE_DEBUG_FLAGS}>>,$<NOT:$<BOOL:${SKEXCEL_WASM_SYMBOLS}>>>:SHELL:-sASSERTIONS=${SKEXCEL_WASM_ASSERTIONS}>"
		# Optimization at LINK time: Emscripten runs the final wasm codegen + wasm-opt here, so a
		# missing -O makes the link default to -O0 and produce slow, bloated wasm even when the .o
		# were compiled -O3. Release links -O3; ASan links -O1 (readable frames, still checked).
		"$<$<AND:$<NOT:$<CONFIG:Debug>>,$<NOT:$<BOOL:${SKEXCEL_WASM_FORCE_DEBUG_FLAGS}>>,$<NOT:$<BOOL:${SKEXCEL_WASM_ASAN}>>>:SHELL:-O3>"
		"$<$<BOOL:${SKEXCEL_WASM_ASAN}>:SHELL:-O1>"
		"SHELL:-sSTACK_SIZE=${SKEXCEL_WASM_STACK_SIZE}"
		"$<$<BOOL:${SKEXCEL_WASM_ASAN}>:SHELL:-fsanitize=address>"
		"$<$<BOOL:${SKEXCEL_WASM_ASAN}>:SHELL:-g>"
		# In-process module: factory export, no auto-run, keep runtime alive between calls, and
		# expose skexcel_convert()+ccall so Node can reuse a single instance across requests.
		"$<$<BOOL:${SKEXCEL_WASM_LIB}>:SHELL:-sMODULARIZE=1>"
		"$<$<BOOL:${SKEXCEL_WASM_LIB}>:SHELL:-sEXPORT_NAME=createSkExcel>"
		"$<$<BOOL:${SKEXCEL_WASM_LIB}>:SHELL:-sINVOKE_RUN=0>"
		"$<$<BOOL:${SKEXCEL_WASM_LIB}>:SHELL:-sEXIT_RUNTIME=0>"
		"$<$<BOOL:${SKEXCEL_WASM_LIB}>:SHELL:-sEXPORTED_FUNCTIONS=_skexcel_convert,_main,_malloc,_free>"
		"$<$<BOOL:${SKEXCEL_WASM_LIB}>:SHELL:-sEXPORTED_RUNTIME_METHODS=ccall,cwrap,callMain,FS>"
	)
	# In-process variant is emitted as SkExcelLib.js so it coexists with the auto-run CLI SkExcel.js.
	if(SKEXCEL_WASM_LIB)
		set_target_properties(${CURRENT_PROJECT} PROPERTIES OUTPUT_NAME "SkExcelLib")
	endif()
	target_compile_options(${CURRENT_PROJECT} PRIVATE
		"$<$<OR:$<CONFIG:Debug>,$<BOOL:${SKEXCEL_WASM_FORCE_DEBUG_FLAGS}>>:-O0>"
		"$<$<OR:$<CONFIG:Debug>,$<BOOL:${SKEXCEL_WASM_FORCE_DEBUG_FLAGS}>,$<BOOL:${SKEXCEL_WASM_SYMBOLS}>>:-g>"
		"$<$<BOOL:${SKEXCEL_WASM_ASAN}>:-fsanitize=address>"
		"$<$<BOOL:${SKEXCEL_WASM_ASAN}>:-g>"
	)
elseif(${SK_PLATFORM} STREQUAL unix AND NOT APPLE)
    # GNU ld: wrap static libs for circular SkRoot/SkFormat/SkSpreadSheet refs.
    # Apple ld64 does not support --start-group; PROJECT_LIBRARY order is used instead.
    target_link_libraries(${CURRENT_PROJECT} 
        "-Wl,--start-group"
        ${SK_SEARCH_LIB}/libSkSpreadSheet.a 
        ${SK_SEARCH_LIB}/libSkFormat.a
        ${SK_SEARCH_LIB}/libSkRoot.a
        "-Wl,--end-group"
        ${LIBZIP_LIB}
        ${PUGIXML_LIB}
    )
else()
    target_link_libraries(${CURRENT_PROJECT} ${PROJECT_LIBRARY})
endif()

# Optimize build dependencies for parallel builds
if(CMAKE_GENERATOR STREQUAL "Xcode")
    # Set target properties to enable parallel builds
    set_target_properties(${CURRENT_PROJECT} PROPERTIES
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "NO"
        XCODE_ATTRIBUTE_CLANG_WARN_QUOTED_INCLUDE_IN_FRAMEWORK_HEADER "NO"
        # Optimize per configuration: Release -O3 (the xlsx->api import lives in this target's
        # own .cpp, so forcing -O0 here made the native "Release" binary run unoptimized and slow);
        # Debug stays -O0 for fast builds/debugging.
        "XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL[variant=Release]" "3"
        "XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL[variant=Debug]" "0"
        XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf"
        XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "NO"
        XCODE_ATTRIBUTE_VALIDATE_PRODUCT "NO"
    )
    
    # Enable parallel builds by setting appropriate flags
    set_target_properties(${CURRENT_PROJECT} PROPERTIES
        XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++20"
        XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++"
    )
endif()

## Specifique au compilateur =================================================
if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
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
	#   Level of Warning
	# /FS: serialize PDB writes when MSBuild runs several CL.EXE on one project
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /FS /D_CRT_SECURE_NO_WARNINGS")
elseif(${SK_PLATFORM} STREQUAL unix)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++20")
elseif(${SK_PLATFORM} STREQUAL wasm)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
elseif(${SK_PLATFORM} STREQUAL xcode)
	# Match the minOS of the Homebrew prebuilts we link against (libzip / pugixml ship with
	# minos 15.0). Linking a 14.0 target against 15.0 dylibs/archives produces:
	#   "Building for macOS-14.0, but linking with dylib ... built for newer version 15.0"
	# Bumping to 15.0 silences both warnings and matches the available SDK.
	set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
	# Let CMake automatically detect the SDK instead of forcing a specific one
	# This avoids warnings when the specified SDK doesn't exist
	# Enable parallel builds for Xcode
	set(CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "NO")
	set(CMAKE_XCODE_ATTRIBUTE_CLANG_WARN_QUOTED_INCLUDE_IN_FRAMEWORK_HEADER "NO")
	# Per-config optimization: Release -O3 (fast native binary), Debug -O0 (fast build/debug).
	# A blanket "0" here previously left even Release builds unoptimized.
	set("CMAKE_XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL[variant=Release]" "3")
	set("CMAKE_XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL[variant=Debug]" "0")
	set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf")
endif()

## Pour Debug =================================================================
message( "CMAKE_BINARY_DIR->" ${CMAKE_BINARY_DIR})
message( "CMAKE_CXX_COMPILER_ID->" ${CMAKE_CXX_COMPILER_ID})
message( "CMAKE_GENERATOR->" ${CMAKE_GENERATOR})
message( "CMAKE_CXX_FLAGS->" ${CMAKE_CXX_FLAGS} )
message( "CMAKE_CXX_FLAGS_DEBUG->" ${CMAKE_CXX_FLAGS_DEBUG})
message( "CMAKE_CXX_FLAGS_RELEASE->" ${CMAKE_CXX_FLAGS_RELEASE})

message("PROJECT_LIBRARY->" ${PROJECT_LIBRARY})
