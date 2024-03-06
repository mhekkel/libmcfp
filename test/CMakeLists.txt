find_package(Catch2 QUIET)

if(NOT Catch2_FOUND)
	FetchContent_Declare(
		Catch2
		GIT_REPOSITORY https://github.com/catchorg/Catch2.git
		GIT_TAG v2.13.9)

	FetchContent_MakeAvailable(Catch2)

	set(Catch2_VERSION "2.13.9")
endif()

add_executable(mcfp-unit-test ${CMAKE_CURRENT_SOURCE_DIR}/unit-test.cpp)

target_link_libraries(mcfp-unit-test mcfp::mcfp Catch2::Catch2)

if(${Catch2_VERSION} VERSION_GREATER_EQUAL 3.0.0)
	target_compile_definitions(mcfp-unit-test PUBLIC CATCH22=0)
else()
	target_compile_definitions(mcfp-unit-test PUBLIC CATCH22=1)
endif()

if(MSVC)
	# Specify unwind semantics so that MSVC knowns how to handle exceptions
	target_compile_options(mcfp-unit-test PRIVATE /EHsc)
endif()

add_test(NAME mcfp-unit-test
	COMMAND $<TARGET_FILE:mcfp-unit-test> --data-dir ${CMAKE_CURRENT_SOURCE_DIR})
