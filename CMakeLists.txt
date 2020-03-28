cmake_minimum_required(VERSION 3.13.0)
project(janus)
include_directories(include)
set(SHARED_COMPILE_OPTIONS
	--std=gnu++2a -Wall -g)

# Jupiter

file(GLOB_RECURSE JUPITER_SOURCES
	"src/jupiter/*.cc"
)
add_executable(jupiter
        ${JUPITER_SOURCES}
)
target_compile_options(jupiter
	PUBLIC ${SHARED_COMPILE_OPTIONS} -O3
)

# Tests

include(GoogleTest)
file(GLOB_RECURSE TEST_SOURCES
	"test/*.cc"
)
add_executable(testrunner
	${TEST_SOURCES}
)
target_compile_options(testrunner
	PUBLIC ${SHARED_COMPILE_OPTIONS} -O0 -fno-omit-frame-pointer
	-fsanitize=address,leak,undefined,null,return,signed-integer-overflow,bounds-strict,object-size
)
target_link_libraries(testrunner
	asan
	ubsan
	gtest
)
gtest_add_tests(testrunner "" AUTO)

add_custom_target(test
	COMMAND ./testrunner --gtest_shuffle --gtest_repeat=3
	DEPENDS testrunner
	WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
)

# clang-format
add_custom_target(clangformat ALL
	find
	  ${CMAKE_CURRENT_SOURCE_DIR}/src
	  ${CMAKE_CURRENT_SOURCE_DIR}/test
	  ${CMAKE_CURRENT_SOURCE_DIR}/include
	  -type f
	  (
	  -name "*.cc"
	  -o
	  -name "*.hh"
	  )
	  -print0
	| xargs -0 clang-format -i
	COMMENT "Running clang-format..."
	VERBATIM
)
