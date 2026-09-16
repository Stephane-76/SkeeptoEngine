# wasm SK_COMPIL: -DSK_COMPIL_MODE=... from the superbuild, or SK_COMPIL in the environment.
if(NOT DEFINED SK_COMPIL_MODE OR SK_COMPIL_MODE STREQUAL "")
	if(DEFINED ENV{SK_COMPIL} AND NOT "$ENV{SK_COMPIL}" STREQUAL "")
		set(SK_COMPIL_MODE "$ENV{SK_COMPIL}")
	else()
		set(SK_COMPIL_MODE "DEBUG")
	endif()
endif()
set(SK_COMPIL_MODE "${SK_COMPIL_MODE}" CACHE STRING "wasm build mode: DEBUG or RELEASE" FORCE)

# Windows: CMAKE_CXX_COMPILER is often forced to emcc (C driver). emcc then
# does not pull libc++ (operator new, std::cout, …). Mac is usually em++.
# DEFAULT_TO_CXX makes executable links C++ regardless of emcc vs em++.
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sDEFAULT_TO_CXX=1")

set(SK_WASM_RELEASE FALSE)
if(SK_COMPIL_MODE STREQUAL "RELEASE")
	set(SK_WASM_RELEASE TRUE)
endif()

message(STATUS "SkWasmCompil: SK_COMPIL_MODE=${SK_COMPIL_MODE}")

# wasm 64-bit (Memory64): -DSK_MEMORY64=ON/OFF, or from env SK_64Bit=ON/OFF.
# Memory64 turns pointers into 64-bit: this is an ABI change, so it MUST be
# applied consistently at COMPILE (here, via CMAKE_CXX_FLAGS) AND at LINK
# (see each module's LINK_FLAGS). All static libs + the final module have to be
# built with the same setting — you cannot mix wasm32 and wasm64 objects.
if(NOT DEFINED SK_MEMORY64 OR SK_MEMORY64 STREQUAL "")
	if(DEFINED ENV{SK_64Bit} AND NOT "$ENV{SK_64Bit}" STREQUAL "")
		set(SK_MEMORY64 "$ENV{SK_64Bit}")
	else()
		set(SK_MEMORY64 "OFF")
	endif()
endif()
string(TOUPPER "${SK_MEMORY64}" _SK_MEMORY64_UP)
set(SK_WASM_MEMORY64 FALSE)
if(_SK_MEMORY64_UP STREQUAL "ON" OR _SK_MEMORY64_UP STREQUAL "1"
	OR _SK_MEMORY64_UP STREQUAL "TRUE" OR _SK_MEMORY64_UP STREQUAL "YES")
	set(SK_WASM_MEMORY64 TRUE)
endif()
set(SK_MEMORY64 "${SK_MEMORY64}" CACHE STRING "wasm 64-bit Memory64 build: ON or OFF" FORCE)

if(SK_WASM_MEMORY64)
	message(STATUS "SkWasmCompil: MEMORY64=ON (wasm64, 64-bit pointers)")
	# wasm64 note: Emscripten's JS-based exceptions / setjmp-longjmp use invoke_*
	# trampolines that pass the function-table index to getWasmTableEntry(). Under
	# MEMORY64 that index is a BigInt and the JS glue throws
	# "Cannot convert a BigInt value to a number" (crash in __wasm_call_ctors).
	# The fix is to use native Wasm exceptions + Wasm longjmp, which don't route
	# through those JS trampolines. These flags must be on BOTH compile and link
	# and are exposed via SK_WASM_MEM64_LINK_FLAGS for each target's LINK_FLAGS.
	# NOTE: use the standard "-m64" target flag rather than the deprecated
	# "-sMEMORY64=1" setting (recent Emscripten warns on the latter). Both select
	# the same 64-bit-pointer (Memory64) ABI, so mixing is safe, but -m64 is the
	# non-deprecated spelling.
	set(SK_WASM_MEM64_LINK_FLAGS "-m64 -fwasm-exceptions -sSUPPORT_LONGJMP=wasm")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SK_WASM_MEM64_LINK_FLAGS}")
else()
	message(STATUS "SkWasmCompil: MEMORY64=OFF (wasm32)")
	set(SK_WASM_MEM64_LINK_FLAGS "")
endif()
