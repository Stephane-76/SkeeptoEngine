# Resolve SK_SKEEPTO_DIR and add the skeepto-sync target.
# Browser wasm (ENVIRONMENT=web) → <skeepto>/public/
# Node wasm   (ENVIRONMENT=node, -DSK_NODE) → <skeepto>/Node/Server/
# Does not touch skeepto/build/ (user runs npm run build / copies public → build).

set(SK_SKEEPTO_DIR "" CACHE PATH
	"Skeepto app root. Empty = sibling ../skeepto if present. NONE disables copy.")

set(_sk_skeepto "")
string(TOUPPER "${SK_SKEEPTO_DIR}" _sk_skeepto_up)
if(_sk_skeepto_up STREQUAL "NONE" OR _sk_skeepto_up STREQUAL "OFF")
	set(_sk_skeepto "")
elseif(NOT "${SK_SKEEPTO_DIR}" STREQUAL "")
	get_filename_component(_sk_skeepto "${SK_SKEEPTO_DIR}" ABSOLUTE)
else()
	set(_sk_guess "${CMAKE_SOURCE_DIR}/../skeepto")
	if(EXISTS "${_sk_guess}/public" AND EXISTS "${_sk_guess}/Node/Server")
		get_filename_component(_sk_skeepto "${_sk_guess}" ABSOLUTE)
	endif()
endif()

if(_sk_skeepto STREQUAL "")
	message(STATUS "skeepto-engine: no Skeepto tree (pass -DSK_SKEEPTO_DIR=... to copy wasm)")
	return()
endif()
if(NOT EXISTS "${_sk_skeepto}/public" OR NOT EXISTS "${_sk_skeepto}/Node/Server")
	message(FATAL_ERROR
		"SK_SKEEPTO_DIR=${_sk_skeepto} is not a Skeepto app (need public/ and Node/Server/).\n"
		"Pass -DSK_SKEEPTO_DIR=/path/to/skeepto or -DSK_SKEEPTO_DIR=NONE to skip.")
endif()

message(STATUS "skeepto-engine: will copy wasm into ${_sk_skeepto}")

set(_sk_bin "${CMAKE_SOURCE_DIR}/${SK_OUTPUT_DIR}/bin")
set(_sk_copy "${CMAKE_SOURCE_DIR}/cmake/SkCopyWasmModule.cmake")
set(_sk_sync_cmds)
set(_sk_sync_deps)

if(SK_WASM_MEMORY64)
	set(_sk_web_js "${_sk_bin}/SkReactSpreadSheet64.js")
	set(_sk_web_stem "SkReactSpreadSheet64")
else()
	set(_sk_web_js "${_sk_bin}/SkReactSpreadSheet.js")
	set(_sk_web_stem "SkReactSpreadSheet")
endif()

list(APPEND _sk_sync_deps SkReactSpreadSheet)
list(APPEND _sk_sync_cmds
	COMMAND ${CMAKE_COMMAND}
		-Dsrc_js=${_sk_web_js}
		-Ddest_dir=${_sk_skeepto}/public
		-Ddest_stem=${_sk_web_stem}
		-P ${_sk_copy}
)

# Node glue is wasm32 only (same as compil2Wasm.sh: skip SK_NODE when Memory64).
if(SK_BUILD_REACT_NODE AND NOT SK_WASM_MEMORY64)
	list(APPEND _sk_sync_deps SkReactSpreadSheetNode)
	list(APPEND _sk_sync_cmds
		COMMAND ${CMAKE_COMMAND}
			-Dsrc_js=${_sk_bin}/SkReactSpreadSheetNode.js
			-Ddest_dir=${_sk_skeepto}/Node/Server
			-Ddest_stem=SkReactSpreadSheet
			-P ${_sk_copy}
	)
	if(EXISTS "${_sk_skeepto}/Node/Client")
		list(APPEND _sk_sync_cmds
			COMMAND ${CMAKE_COMMAND}
				-Dsrc_js=${_sk_bin}/SkReactSpreadSheetNode.js
				-Ddest_dir=${_sk_skeepto}/Node/Client
				-Ddest_stem=SkReactSpreadSheet
				-P ${_sk_copy}
		)
	endif()
endif()

add_custom_target(skeepto-sync ALL
	${_sk_sync_cmds}
	DEPENDS ${_sk_sync_deps}
	COMMENT "Copy wasm modules into ${_sk_skeepto} (not build/; sync that with npm run build)"
	VERBATIM
)
