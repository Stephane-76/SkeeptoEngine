# Fetch (git clone into third-party/) and compile the deps the sker CMakeLists expect.
# Header-only: rapidjson, rapidxml.
# Compiled: cppunit (tests), pugixml + zlib/libzip (SkExcel wasm/windows, pugixml also unix).

set(SK_TP "${CMAKE_SOURCE_DIR}/third-party")
set(SK_TP_TEST_DEPS "")
set(SK_TP_EXCEL_DEPS "")

set(SK_ZLIB_VERSION "1.3.1" CACHE STRING "zlib version downloaded for libzip")
set(SK_LIBZIP_VERSION "1.11.4" CACHE STRING "libzip version downloaded when building from source")

# Drop a child CMakeCache.txt that was generated for another source or build path
# (CMake refuses to reconfigure: "does not match the source ... used to generate cache").
function(sk_reset_stale_cmake_cache binary_dir expected_source)
	set(_cache "${binary_dir}/CMakeCache.txt")
	if(NOT EXISTS "${_cache}")
		return()
	endif()
	get_filename_component(_want_src "${expected_source}" REALPATH)
	get_filename_component(_want_bin "${binary_dir}" REALPATH)
	file(STRINGS "${_cache}" _home_lines REGEX "^CMAKE_HOME_DIRECTORY:")
	file(STRINGS "${_cache}" _bin_lines REGEX "^CMAKE_CACHEFILE_DIR:")
	file(STRINGS "${_cache}" _gen_lines REGEX "^CMAKE_GENERATOR:")
	set(_stale FALSE)
	if(_gen_lines AND SK_CHILD_GENERATOR)
		list(GET _gen_lines 0 _gen)
		string(REGEX REPLACE "^CMAKE_GENERATOR:[^=]*=" "" _have_gen "${_gen}")
		if(NOT _have_gen STREQUAL SK_CHILD_GENERATOR)
			set(_stale TRUE)
		endif()
	endif()
	if(_home_lines)
		list(GET _home_lines 0 _home)
		string(REGEX REPLACE "^CMAKE_HOME_DIRECTORY:[^=]*=" "" _have_src "${_home}")
		get_filename_component(_have_src "${_have_src}" REALPATH)
		if(NOT _have_src STREQUAL _want_src)
			set(_stale TRUE)
		endif()
	endif()
	if(NOT _stale AND _bin_lines)
		list(GET _bin_lines 0 _bin)
		string(REGEX REPLACE "^CMAKE_CACHEFILE_DIR:[^=]*=" "" _have_bin "${_bin}")
		get_filename_component(_have_bin "${_have_bin}" REALPATH)
		if(NOT _have_bin STREQUAL _want_bin)
			set(_stale TRUE)
		endif()
	endif()
	if(_stale)
		message(STATUS "Removing stale CMake cache in ${binary_dir}")
		file(REMOVE "${_cache}")
		if(EXISTS "${binary_dir}/CMakeFiles")
			file(REMOVE_RECURSE "${binary_dir}/CMakeFiles")
		endif()
	endif()
endfunction()

# --- git clone if missing ----------------------------------------------------
if(SK_FETCH_THIRD_PARTY)
	find_package(Git QUIET)
endif()

function(sk_ensure_git name url)
	cmake_parse_arguments(A "" "TAG;HINT" "" ${ARGN})
	set(_dir "${SK_TP}/${name}")
	if(A_HINT)
		set(_ok "${_dir}/${A_HINT}")
	else()
		set(_ok "${_dir}")
	endif()
	# Drop a previous empty / interrupted clone (git clone into a non-empty dir fails).
	if(EXISTS "${_dir}" AND NOT EXISTS "${_ok}")
		message(STATUS "third-party/${name}: incomplete clone, removing ${_dir}")
		file(REMOVE_RECURSE "${_dir}")
	endif()
	if(EXISTS "${_ok}")
		message(STATUS "third-party/${name}: present")
		return()
	endif()
	if(NOT SK_FETCH_THIRD_PARTY)
		message(FATAL_ERROR "Missing ${_ok}. Enable SK_FETCH_THIRD_PARTY so CMake can clone it.")
	endif()
	if(NOT GIT_EXECUTABLE)
		find_package(Git REQUIRED)
	endif()
	file(MAKE_DIRECTORY "${SK_TP}")
	set(_args clone --depth 1)
	if(A_TAG)
		list(APPEND _args --branch "${A_TAG}")
	endif()
	list(APPEND _args "${url}" "${_dir}")
	message(STATUS "Cloning ${name} -> ${_dir}")
	execute_process(
		COMMAND "${GIT_EXECUTABLE}" ${_args}
		RESULT_VARIABLE _rc
		ERROR_VARIABLE _err
		OUTPUT_VARIABLE _out
	)
	if(NOT _rc EQUAL 0)
		message(FATAL_ERROR "git clone ${name} failed:\n${_err}${_out}")
	endif()
	if(NOT EXISTS "${_ok}")
		message(FATAL_ERROR "git clone ${name} succeeded but missing ${_ok} (check URL/tag)")
	endif()
endfunction()

# Drop a pre-C++17 CppUnit clone (dlrdave 1.11). Do not key off auto_ptr:
# a local header patch would hide the stale source tree.
function(sk_discard_stale_cppunit)
	set(_cm "${SK_TP}/cppunit/CMakeLists.txt")
	if(NOT EXISTS "${_cm}")
		return()
	endif()
	file(READ "${_cm}" _txt)
	if(_txt MATCHES "CPPUNIT_MINOR_VERSION 11" OR _txt MATCHES "CPPUNIT_MICRO_VERSION")
		message(STATUS "third-party/cppunit: discarding 1.11 clone, will fetch Ultimaker 1.14.2")
		file(REMOVE_RECURSE "${SK_TP}/cppunit")
	endif()
endfunction()

sk_ensure_git(rapidjson https://github.com/Tencent/rapidjson.git HINT include/rapidjson/document.h)
sk_ensure_git(rapidxml https://github.com/discord/rapidxml.git HINT rapidxml.hpp)
if(SK_BUILD_TESTS)
	# LibreOffice cppunit 1.15 is autotools-only. Ultimaker/CppUnit is the CMake
	# port of 1.14.2 (unique_ptr + enum class StringHelper). dlrdave/cppunit is 1.11.6.
	sk_discard_stale_cppunit()
	sk_ensure_git(cppunit https://github.com/Ultimaker/CppUnit.git HINT CMakeLists.txt)
endif()
if(SK_BUILD_APPS)
	# Upstream is zeux/pugixml. github.com/pugixml/pugixml is an empty placeholder
	# (git clone --depth 1 succeeds but checks out nothing).
	sk_ensure_git(pugixml https://github.com/zeux/pugixml.git TAG v1.15 HINT src/pugixml.cpp)
endif()

# --- shared ExternalProject cmake invocation ---------------------------------
function(sk_tp_cmake target_name source_dir binary_dir)
	cmake_parse_arguments(A "" "" "DEPENDS;CMAKE_ARGS;INSTALL_COMMAND" ${ARGN})
	sk_reset_stale_cmake_cache("${binary_dir}" "${source_dir}")

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

	# Xcode drops an empty INSTALL_COMMAND "" and falls back to
	# `cmake --build --target install` (Ultimaker then writes to /usr/local).
	if(A_INSTALL_COMMAND)
		set(_install_cmd ${A_INSTALL_COMMAND})
	else()
		set(_install_cmd ${CMAKE_COMMAND} -E true)
	endif()

	ExternalProject_Add(${target_name}
		SOURCE_DIR "${source_dir}"
		BINARY_DIR "${binary_dir}"
		PREFIX "${CMAKE_BINARY_DIR}/ep/${target_name}"
		DOWNLOAD_COMMAND ""
		UPDATE_COMMAND ""
		PATCH_COMMAND ""
		CMAKE_GENERATOR "${SK_CHILD_GENERATOR}"
		${_gen_plat}
		CMAKE_ARGS ${A_CMAKE_ARGS}
		CMAKE_CACHE_ARGS ${SK_CHILD_CMAKE_CACHE_ARGS}
		BUILD_COMMAND ${_build_cmd}
		INSTALL_COMMAND ${_install_cmd}
		DEPENDS ${A_DEPENDS}
		USES_TERMINAL_CONFIGURE TRUE
		USES_TERMINAL_BUILD TRUE
		USES_TERMINAL_INSTALL TRUE
		LOG_OUTPUT_ON_FAILURE TRUE
	)
endfunction()

function(sk_tp_already_or_build target_name artifact)
	if(EXISTS "${artifact}" AND NOT SK_FORCE_THIRD_PARTY)
		message(STATUS "third-party ${target_name}: using ${artifact}")
		add_custom_target(${target_name})
		return()
	endif()
	if(NOT SK_BUILD_THIRD_PARTY)
		message(FATAL_ERROR "Missing ${artifact}. Enable SK_BUILD_THIRD_PARTY or build that third-party first.")
	endif()
	# Caller defines ExternalProject with the same target_name after this returns FALSE.
	set(SK_TP_NEED_BUILD TRUE PARENT_SCOPE)
endfunction()

# Artifact missing but PREFIX stamps still say configure/build succeeded (e.g. after
# replacing the git clone). Drop stamps + child cache so ExternalProject starts over.
function(sk_invalidate_ep target_name)
	cmake_parse_arguments(A "" "BINARY_DIR" "" ${ARGN})
	set(_ep "${CMAKE_BINARY_DIR}/ep/${target_name}")
	if(EXISTS "${_ep}")
		message(STATUS "third-party/${target_name}: clearing ExternalProject stamps")
		file(REMOVE_RECURSE "${_ep}")
	endif()
	if(A_BINARY_DIR)
		if(EXISTS "${A_BINARY_DIR}/CMakeCache.txt")
			file(REMOVE "${A_BINARY_DIR}/CMakeCache.txt")
		endif()
		if(EXISTS "${A_BINARY_DIR}/CMakeFiles")
			file(REMOVE_RECURSE "${A_BINARY_DIR}/CMakeFiles")
		endif()
	endif()
endfunction()

# --- cppunit (unit tests) -------------------------------------------------------
if(SK_BUILD_TESTS)
	if(SK_PLATFORM STREQUAL "windows")
		set(_cppunit_art "${SK_TP}/cppunit/${SK_BUILD_SUBDIR}/Debug/cppunit.lib")
	elseif(SK_PLATFORM STREQUAL "xcode")
		set(_cppunit_art "${SK_TP}/cppunit/xcode/Debug/libcppunit.a")
	elseif(SK_PLATFORM STREQUAL "wasm")
		set(_cppunit_art "${SK_TP}/cppunit/${SK_BUILD_SUBDIR}/Release/libcppunit.a")
	else()
		set(_cppunit_art "${SK_TP}/cppunit/unix/libcppunit.a")
	endif()

	sk_reset_stale_cmake_cache("${SK_TP}/cppunit/${SK_BUILD_SUBDIR}" "${SK_TP}/cppunit")

	set(SK_TP_NEED_BUILD FALSE)
	sk_tp_already_or_build(cppunit "${_cppunit_art}")
	if(SK_TP_NEED_BUILD)
		sk_invalidate_ep(cppunit BINARY_DIR "${SK_TP}/cppunit/${SK_BUILD_SUBDIR}")
		set(_cppunit_args)
		if(SK_PLATFORM STREQUAL "wasm")
			list(APPEND _cppunit_args
				-DCMAKE_TOOLCHAIN_FILE=${SK_EMSCRIPTEN_TOOLCHAIN}
				-DEMSCRIPTEN=1
				-DCMAKE_BUILD_TYPE=Release
			)
			if(_SK_MEMORY64_UP STREQUAL "ON" OR _SK_MEMORY64_UP STREQUAL "1"
				OR _SK_MEMORY64_UP STREQUAL "TRUE" OR _SK_MEMORY64_UP STREQUAL "YES")
				list(APPEND _cppunit_args -DCMAKE_CXX_FLAGS=-m64)
			endif()
		elseif(CMAKE_BUILD_TYPE)
			list(APPEND _cppunit_args -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
		endif()

		# Ultimaker writes config-auto.h to <BINARY_DIR>/cppunit/config-auto.h.
		# Wasm tests link Release/libcppunit.a (single-config emcc emits libcppunit.a).
		set(_cppunit_install "")
		if(SK_PLATFORM STREQUAL "wasm")
			set(_cppunit_install
				${CMAKE_COMMAND} -E make_directory <BINARY_DIR>/Release
				COMMAND ${CMAKE_COMMAND} -E make_directory <BINARY_DIR>/Debug
				COMMAND ${CMAKE_COMMAND} -E copy_if_different <BINARY_DIR>/libcppunit.a <BINARY_DIR>/Release/libcppunit.a
				COMMAND ${CMAKE_COMMAND} -E copy_if_different <BINARY_DIR>/libcppunit.a <BINARY_DIR>/Debug/libcppunit.a
			)
		endif()

		sk_tp_cmake(cppunit
			"${SK_TP}/cppunit"
			"${SK_TP}/cppunit/${SK_BUILD_SUBDIR}"
			CMAKE_ARGS ${_cppunit_args}
			INSTALL_COMMAND ${_cppunit_install}
		)
	endif()
	set(SK_TP_TEST_DEPS cppunit)
endif()

# --- pugixml (SkExcel: wasm archive, unix fallback, Windows compiles the .cpp) -
if(SK_BUILD_APPS)
	set(_pugixml_do FALSE)
	if(SK_PLATFORM STREQUAL "wasm")
		set(_pugixml_art "${SK_TP}/pugixml/wasm/libpugixml.a")
		set(_pugixml_bin "${SK_TP}/pugixml/wasm")
		set(_pugixml_do TRUE)
	elseif(SK_PLATFORM STREQUAL "unix")
		set(_pugixml_art "${SK_TP}/pugixml/build/libpugixml.a")
		set(_pugixml_bin "${SK_TP}/pugixml/build")
		if(NOT EXISTS "/usr/local/lib/libpugixml.a" AND NOT EXISTS "/opt/homebrew/lib/libpugixml.a")
			set(_pugixml_do TRUE)
		endif()
	endif()

	if(_pugixml_do)
		set(SK_TP_NEED_BUILD FALSE)
		sk_tp_already_or_build(pugixml "${_pugixml_art}")
		if(SK_TP_NEED_BUILD)
			set(_pugixml_args -DBUILD_SHARED_LIBS=OFF -DPUGIXML_BUILD_TESTS=OFF)
			if(SK_PLATFORM STREQUAL "wasm")
				list(APPEND _pugixml_args
					-DCMAKE_TOOLCHAIN_FILE=${SK_EMSCRIPTEN_TOOLCHAIN}
					-DEMSCRIPTEN=1
					-DCMAKE_BUILD_TYPE=Release
					-DPUGIXML_NO_EXCEPTIONS=ON
				)
			elseif(CMAKE_BUILD_TYPE)
				list(APPEND _pugixml_args -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
			endif()
			sk_tp_cmake(pugixml
				"${SK_TP}/pugixml"
				"${_pugixml_bin}"
				CMAKE_ARGS ${_pugixml_args}
			)
		endif()
		list(APPEND SK_TP_EXCEL_DEPS pugixml)
	endif()
endif()

# --- zlib + libzip (SkExcel wasm + windows; mac/linux use Homebrew/system) --
if(SK_BUILD_APPS AND (SK_PLATFORM STREQUAL "wasm" OR SK_PLATFORM STREQUAL "windows"))
	if(SK_PLATFORM STREQUAL "wasm")
		set(_libzip_prefix "${SK_TP}/libzip/wasm-install")
		set(_zlib_prefix "${SK_TP}/libzip/zlib-install")
		set(_libzip_art "${_libzip_prefix}/lib/libzip.a")
	else()
		set(_libzip_prefix "${SK_TP}/libzip/windows-install")
		set(_zlib_prefix "${SK_TP}/libzip/zlib-install")
		set(_libzip_art "${_libzip_prefix}/lib/zip.lib")
		if(NOT EXISTS "${_libzip_art}")
			set(_libzip_art "${_libzip_prefix}/lib/libzip.lib")
		endif()
	endif()

	set(SK_TP_NEED_BUILD FALSE)
	sk_tp_already_or_build(libzip_tp "${_libzip_art}")
	if(SK_TP_NEED_BUILD)
		file(MAKE_DIRECTORY "${SK_TP}/libzip/deps")

		set(_zlib_src "${SK_TP}/libzip/deps/zlib-${SK_ZLIB_VERSION}")
		set(_libzip_src "${SK_TP}/libzip/deps/libzip-${SK_LIBZIP_VERSION}")
		if(EXISTS "${SK_TP}/libzip/CMakeLists.txt")
			set(_libzip_src "${SK_TP}/libzip")
		endif()

		set(_z_args -DCMAKE_INSTALL_PREFIX=${_zlib_prefix} -DCMAKE_INSTALL_LIBDIR=lib)
		set(_z_args_libzip
			-DCMAKE_INSTALL_PREFIX=${_libzip_prefix}
			-DCMAKE_INSTALL_LIBDIR=lib
			-DCMAKE_POLICY_VERSION_MINIMUM=3.5
			-DBUILD_SHARED_LIBS=OFF
			-DBUILD_TOOLS=OFF
			-DBUILD_EXAMPLES=OFF
			-DBUILD_DOC=OFF
			-DBUILD_REGRESS=OFF
			-DBUILD_OSSFUZZ=OFF
			-DENABLE_OPENSSL=OFF
			-DENABLE_COMMONCRYPTO=OFF
			-DENABLE_GNUTLS=OFF
			-DENABLE_MBEDTLS=OFF
			-DENABLE_WINDOWS_CRYPTO=OFF
			-DENABLE_BZIP2=OFF
			-DENABLE_LZMA=OFF
			-DENABLE_ZSTD=OFF
			-DZLIB_ROOT=${_zlib_prefix}
		)
		if(SK_PLATFORM STREQUAL "wasm")
			list(APPEND _z_args
				-DCMAKE_TOOLCHAIN_FILE=${SK_EMSCRIPTEN_TOOLCHAIN}
				-DCMAKE_BUILD_TYPE=Release
			)
			list(APPEND _z_args_libzip
				-DCMAKE_TOOLCHAIN_FILE=${SK_EMSCRIPTEN_TOOLCHAIN}
				-DCMAKE_BUILD_TYPE=Release
			)
		elseif(CMAKE_BUILD_TYPE)
			list(APPEND _z_args -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
			list(APPEND _z_args_libzip -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
		endif()

		set(_gen_plat)
		if(SK_CHILD_GENERATOR_PLATFORM)
			set(_gen_plat CMAKE_GENERATOR_PLATFORM ${SK_CHILD_GENERATOR_PLATFORM})
		endif()

		sk_reset_stale_cmake_cache("${CMAKE_BINARY_DIR}/tp/zlib" "${_zlib_src}")
		sk_reset_stale_cmake_cache("${CMAKE_BINARY_DIR}/tp/libzip" "${_libzip_src}")

		ExternalProject_Add(zlib_tp
			URL "https://zlib.net/fossils/zlib-${SK_ZLIB_VERSION}.tar.gz"
			DOWNLOAD_EXTRACT_TIMESTAMP TRUE
			SOURCE_DIR "${_zlib_src}"
			BINARY_DIR "${CMAKE_BINARY_DIR}/tp/zlib"
			PREFIX "${CMAKE_BINARY_DIR}/ep/zlib_tp"
			UPDATE_COMMAND ""
			CMAKE_GENERATOR "${SK_CHILD_GENERATOR}"
			${_gen_plat}
			CMAKE_ARGS ${_z_args}
			CMAKE_CACHE_ARGS ${SK_CHILD_CMAKE_CACHE_ARGS}
			BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release --parallel
			INSTALL_COMMAND ${CMAKE_COMMAND} --install <BINARY_DIR> --config Release
			USES_TERMINAL_DOWNLOAD TRUE
			USES_TERMINAL_CONFIGURE TRUE
			USES_TERMINAL_BUILD TRUE
			USES_TERMINAL_INSTALL TRUE
			LOG_OUTPUT_ON_FAILURE TRUE
		)

		set(_libzip_download DOWNLOAD_COMMAND "")
		if(NOT EXISTS "${_libzip_src}/CMakeLists.txt")
			set(_libzip_download
				URL "https://github.com/nih-at/libzip/releases/download/v${SK_LIBZIP_VERSION}/libzip-${SK_LIBZIP_VERSION}.tar.gz"
				DOWNLOAD_EXTRACT_TIMESTAMP TRUE
			)
		endif()

		ExternalProject_Add(libzip_tp
			${_libzip_download}
			SOURCE_DIR "${_libzip_src}"
			BINARY_DIR "${CMAKE_BINARY_DIR}/tp/libzip"
			PREFIX "${CMAKE_BINARY_DIR}/ep/libzip_tp"
			UPDATE_COMMAND ""
			CMAKE_GENERATOR "${SK_CHILD_GENERATOR}"
			${_gen_plat}
			CMAKE_ARGS ${_z_args_libzip}
			CMAKE_CACHE_ARGS ${SK_CHILD_CMAKE_CACHE_ARGS}
			BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release --parallel
			INSTALL_COMMAND ${CMAKE_COMMAND} --install <BINARY_DIR> --config Release
			DEPENDS zlib_tp
			USES_TERMINAL_DOWNLOAD TRUE
			USES_TERMINAL_CONFIGURE TRUE
			USES_TERMINAL_BUILD TRUE
			USES_TERMINAL_INSTALL TRUE
			LOG_OUTPUT_ON_FAILURE TRUE
		)
	endif()
	list(APPEND SK_TP_EXCEL_DEPS libzip_tp)
endif()

message(STATUS "third-party test deps: ${SK_TP_TEST_DEPS}")
message(STATUS "third-party SkExcel deps: ${SK_TP_EXCEL_DEPS}")
