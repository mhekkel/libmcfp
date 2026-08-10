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

# foreach(mcfptest IN LISTS tests)
#     add_executable(${mcfptest} ${CMAKE_CURRENT_SOURCE_DIR}/${mcfptest}.cpp $<TARGET_OBJECTS:test-main>)
#     target_link_libraries(${mcfptest} PRIVATE mcfp::mcfp Catch2::Catch2)

#     if(MSVC)
#         # Specify unwind semantics so that MSVC knowns how to handle exceptions
#         target_compile_options(${mcfptest} PRIVATE /EHsc)
#     endif()

#     add_test(NAME ${mcfptest}
#         COMMAND $<TARGET_FILE:${mcfptest}> --data-dir ${CMAKE_CURRENT_SOURCE_DIR})
# endforeach()

if(MCFP_BUILD_CXX_MODULE)
    foreach(mcfptest IN LISTS tests)
		set(mcfptestexe "${mcfptest}-m")
        add_executable(${mcfptestexe} ${CMAKE_CURRENT_SOURCE_DIR}/${mcfptest}.cpp $<TARGET_OBJECTS:test-main>)
        target_link_libraries(${mcfptestexe} PRIVATE mcfp::mcfp-module Catch2::Catch2)

        if(MSVC)
            # Specify unwind semantics so that MSVC knowns how to handle exceptions
            target_compile_options(${mcfptestexe} PRIVATE /EHsc)
        endif()

        add_test(NAME ${mcfptestexe}
            COMMAND $<TARGET_FILE:${mcfptestexe}> --data-dir ${CMAKE_CURRENT_SOURCE_DIR})
    endforeach()
endif()
