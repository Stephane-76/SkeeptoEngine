# Ensure the platform artifact tree exists (unix/lib, wasm/bin, …).
# On case-insensitive APFS a stray file named unix/Unix/wasm blocks ar/ld
# with: ar: …/unix/lib/libSkRoot.a: Not a directory.

function(sk_ensure_output_dir _dir)
	if("${_dir}" STREQUAL "")
		return()
	endif()
	if(EXISTS "${_dir}" AND NOT IS_DIRECTORY "${_dir}")
		message(WARNING
			"skeepto-engine: removing stray file '${_dir}' "
			"(need a directory for lib/bin artifacts)")
		file(REMOVE "${_dir}")
	endif()
	file(MAKE_DIRECTORY "${_dir}/lib" "${_dir}/bin")
endfunction()

if(DEFINED SK_OUTPUT AND NOT "${SK_OUTPUT}" STREQUAL "")
	sk_ensure_output_dir("${SK_OUTPUT}")
endif()
