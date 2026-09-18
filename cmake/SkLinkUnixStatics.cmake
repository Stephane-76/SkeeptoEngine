# Link Sk* static archives on SK_PLATFORM=unix.
# GNU ld is single-pass; circular refs between SkRoot / SkFormat / SkSpreadSheet
# need --start-group. Apple ld64 does not support that flag — order is enough.
#
# Never pass libstdc++.a by filename: it is not on the default linker search path
# (Ubuntu CI fails with: ld: cannot find libstdc++.a). g++ already links libstdc++.
#
# Requires SK_SEARCH_LIB. Call after add_executable().
#
#   include("${SK_ROOT}/cmake/SkLinkUnixStatics.cmake")
#   sk_link_unix_statics(MyTarget)
#   sk_link_unix_statics(MyTarget LIBS ${SK_SEARCH_LIB}/libSkRoot.a EXTRA ${cppunit})

function(sk_link_unix_statics target)
	cmake_parse_arguments(SK "" "" "LIBS;EXTRA" ${ARGN})
	if(NOT SK_LIBS)
		set(SK_LIBS
			"${SK_SEARCH_LIB}/libSkSpreadSheet.a"
			"${SK_SEARCH_LIB}/libSkFormat.a"
			"${SK_SEARCH_LIB}/libSkRoot.a"
		)
	endif()
	if(NOT APPLE)
		find_package(Threads REQUIRED)
		target_link_libraries(${target}
			"-Wl,--start-group"
			${SK_LIBS}
			"-Wl,--end-group"
			${SK_EXTRA}
			Threads::Threads
		)
	else()
		target_link_libraries(${target} ${SK_LIBS} ${SK_EXTRA})
	endif()
endfunction()
