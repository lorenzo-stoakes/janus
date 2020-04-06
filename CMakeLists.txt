cmake_minimum_required(VERSION 3.13.0)
project(janus)
include_directories(include)
set(SHARED_COMPILE_OPTIONS
	--std=gnu++2a -Wall -g)

file(GLOB_RECURSE HEADER_SOURCES
	"include/*.hh"
)
# Excluding sajson for now, will add once static issues resolved.
list(FILTER HEADER_SOURCES EXCLUDE REGEX "sajson.*")

file(GLOB JANUS_SOURCES
	"src/*.cc"
)

# Neptune

file(GLOB_RECURSE NEPTUNE_SOURCES
	"src/neptune/*.cc"
)
add_executable(neptune
	${JANUS_SOURCES}
	${NEPTUNE_SOURCES}
)
target_compile_options(neptune
	PUBLIC ${SHARED_COMPILE_OPTIONS} -O3
)

# Tests

include(GoogleTest)
file(GLOB_RECURSE TEST_SOURCES
	"test/*.cc"
)
add_executable(testrunner
	${JANUS_SOURCES}
	${TEST_SOURCES}
)
target_compile_options(testrunner
	PUBLIC ${SHARED_COMPILE_OPTIONS} -O0 -fno-omit-frame-pointer
	-fsanitize=address,leak,undefined,null,return,signed-integer-overflow,bounds,object-size
)
target_link_libraries(testrunner
	asan
	ubsan
	gtest
)
gtest_add_tests(testrunner "" AUTO)

add_custom_target(test
	COMMAND ./testrunner --gtest_shuffle
	DEPENDS testrunner cppcheck clangformat
	WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
)

# Benchmark

file(GLOB_RECURSE BENCH_SOURCES
	"bench/sajson/*.cc"
)
add_executable(benchmark
	${JANUS_SOURCES}
	${BENCH_SOURCES}
)
target_compile_options(benchmark
	PUBLIC ${SHARED_COMPILE_OPTIONS} -O3 -DNDEBUG
)
add_custom_target(bench
	COMMAND ${PROJECT_BINARY_DIR}/benchmark
	DEPENDS benchmark clangformat
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Static analysis

add_custom_target(clangformat ALL
	find
	  ${CMAKE_CURRENT_SOURCE_DIR}/src
	  ${CMAKE_CURRENT_SOURCE_DIR}/test
	  ${CMAKE_CURRENT_SOURCE_DIR}/bench
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

add_custom_target(tidy
	clang-tidy --quiet -p ${PROJECT_BINARY_DIR} ${JANUS_SOURCES} ${HEADER_SOURCES} ${NEPTUNE_SOURCES}
	DEPENDS testrunner
	COMMENT "Running clang-tidy..."
	VERBATIM
)

add_custom_target(cppcheck
	cppcheck --std=c++11 --language=c++ --enable=warning,style,performance --quiet ${JANUS_SOURCES} ${HEADER_SOURCES} ${NEPTUNE_SOURCES}
	DEPENDS testrunner
	COMMENT "Running cppcheck..."
	VERBATIM
)
