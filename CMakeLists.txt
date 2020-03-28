cmake_minimum_required(VERSION 3.13.0)
project(janus)
include_directories(include)
add_compile_options(--std=gnu++2a -Wall -O3 -g)

# Jupiter

file(GLOB_RECURSE JUPITER_SOURCES
	"src/jupiter/*.cc"
)
add_executable(jupiter
        ${JUPITER_SOURCES}
)

# Tests

include(GoogleTest)
file(GLOB_RECURSE TEST_SOURCES
	"test/*.cc"
)
add_executable(testrunner
	${TEST_SOURCES}
)
target_link_libraries(testrunner
	gtest
)
gtest_add_tests(testrunner "" AUTO)
