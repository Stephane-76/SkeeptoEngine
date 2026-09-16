# Copy one Emscripten module into a Skeepto tree, renaming .js → .mjs.
# Invoked as: cmake -Dsrc_js=... -Ddest_dir=... -Ddest_stem=... -P this file
#
# Browser:  wasm/bin/SkReactSpreadSheet.js     → public/SkReactSpreadSheet.mjs
# Node:     wasm/bin/SkReactSpreadSheetNode.js → Node/Server/SkReactSpreadSheet.mjs

if(NOT DEFINED src_js OR NOT DEFINED dest_dir OR NOT DEFINED dest_stem)
	message(FATAL_ERROR "SkCopyWasmModule: need -Dsrc_js= -Ddest_dir= -Ddest_stem=")
endif()
if(NOT EXISTS "${src_js}")
	message(FATAL_ERROR "SkCopyWasmModule: missing ${src_js}")
endif()

file(MAKE_DIRECTORY "${dest_dir}")
get_filename_component(_src_dir "${src_js}" DIRECTORY)
get_filename_component(_src_we "${src_js}" NAME_WE)

configure_file("${src_js}" "${dest_dir}/${dest_stem}.mjs" COPYONLY)
if(NOT EXISTS "${_src_dir}/${_src_we}.wasm")
	message(FATAL_ERROR "SkCopyWasmModule: missing ${_src_dir}/${_src_we}.wasm")
endif()
configure_file("${_src_dir}/${_src_we}.wasm" "${dest_dir}/${dest_stem}.wasm" COPYONLY)
if(EXISTS "${_src_dir}/${_src_we}.wasm.map")
	configure_file("${_src_dir}/${_src_we}.wasm.map" "${dest_dir}/${dest_stem}.wasm.map" COPYONLY)
endif()
message(STATUS "SkCopyWasmModule: ${src_js} -> ${dest_dir}/${dest_stem}.mjs")
