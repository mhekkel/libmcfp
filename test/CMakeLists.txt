find_package(Boost REQUIRED)

add_executable(mcfp-unit-test ${CMAKE_CURRENT_SOURCE_DIR}/unit-test.cpp)

target_link_libraries(mcfp-unit-test mcfp::mcfp Boost::boost)

if(MSVC)
	# Specify unwind semantics so that MSVC knowns how to handle exceptions
	target_compile_options(mcfp-unit-test PRIVATE /EHsc)
endif()

add_test(NAME mcfp-unit-test
	COMMAND $<TARGET_FILE:mcfp-unit-test> -- ${CMAKE_CURRENT_SOURCE_DIR})
