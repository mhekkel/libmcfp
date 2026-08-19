# SPDX-FileCopyrightText: 2026 Maarten L. Hekkelman
# SPDX-License-Identifier: BSD-2-Clause

CPMFindPackage(NAME Catch2 GIT_REPOSITORY "https://github.com/catchorg/Catch2" VERSION "3.4.0" EXCLUDE_FROM_ALL YES)

list(APPEND tests
	unit-test
	lib-test
)

add_library(test-main OBJECT "${CMAKE_CURRENT_SOURCE_DIR}/test-main.cpp")
target_compile_features(test-main PUBLIC cxx_std_20)
target_link_libraries(test-main PUBLIC Catch2::Catch2)

function(create_test mcfp-test mcfp-source lib)
	set(mcfp-test-exe ${mcfp-test}-app)
	set(mcfp-test-name ${mcfp-test})

	add_executable(${mcfp-test-exe} ${CMAKE_CURRENT_SOURCE_DIR}/${mcfp-source}.cpp $<TARGET_OBJECTS:test-main>)
	target_link_libraries(${mcfp-test-exe} PRIVATE ${lib} Catch2::Catch2)

	if(MSVC)
		# Specify unwind semantics so that MSVC knowns how to handle exceptions
		target_compile_options(${mcfp-test-exe} PRIVATE /EHsc)
	endif()

	add_test(NAME ${mcfp-test-name}
		COMMAND $<TARGET_FILE:${mcfp-test-exe}> --data-dir ${CMAKE_CURRENT_SOURCE_DIR})

	set(dll_dirs "$<TARGET_RUNTIME_DLL_DIRS:${mcfp-test-exe}>")
	set(is_empty "$<STREQUAL:${dll_dirs},>")
	set(env_op "PATH=path_list_append:")

	set_property(TEST ${mcfp-test-name} APPEND PROPERTY ENVIRONMENT_MODIFICATION
		"$<$<NOT:${is_empty}>:${env_op}$<JOIN:${dll_dirs},$<SEMICOLON>${env_op}>>")
endfunction()

foreach(mcfptest IN LISTS tests)
	create_test(${mcfptest} ${mcfptest} mcfp::mcfp)
endforeach()

if(MCFP_BUILD_CXX_MODULE)
	foreach(mcfptest IN LISTS tests)
		create_test("${mcfptest}-m" ${mcfptest} mcfp::mcfp-module)
	endforeach()
endif()
