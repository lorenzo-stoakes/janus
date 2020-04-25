cmake_minimum_required(VERSION 3.13.0)
project(janus)
include_directories(include . src 3rdparty/mbedtls-2.22.0/include)
link_directories(3rdparty/mbedtls-2.22.0/library)
set(SHARED_COMPILE_OPTIONS
	--std=gnu++2a -Wall -g)

file(GLOB_RECURSE HEADERS
	"include/*.hh"
)

file(GLOB_RECURSE SOURCES
	"src/*.cc"
	"src/*.tcc"
)

file(GLOB NETWORK_SOURCES
	"src/network/*.cc"
)

# 3rd-party

execute_process(COMMAND -j cat /proc/cpuinfo | grep -c processor OUTPUT_VARIABLE NUM_CORES)
add_custom_target(tls
	make --silent -C ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/mbedtls-2.22.0 -j ${NUM_CORES}
	COMMENT "Building mbedtls..."
	VERBATIM
)

# Excluding sajson for now, will add once static issues resolved.
list(FILTER HEADERS EXCLUDE REGEX "sajson.*")

file(GLOB JANUS_SOURCES
	"src/*.cc"
	"src/*.tcc"
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

target_link_libraries(neptune
	m
	snappy
)

# Jupiter

file(GLOB_RECURSE JUPITER_SOURCES
	"src/jupiter/*.cc"
)
add_executable(jupiter
	${JANUS_SOURCES}
	${NETWORK_SOURCES}
	${JUPITER_SOURCES}
)
target_compile_options(jupiter
	PUBLIC ${SHARED_COMPILE_OPTIONS} -O3
)

target_link_libraries(jupiter
	m
	mbedcrypto
	mbedtls
	mbedx509
)

add_dependencies(jupiter tls)

# Checker

file(GLOB_RECURSE CHECKER_SOURCES
	"src/checker/*.cc"
)
add_executable(checker
	${JANUS_SOURCES}
	${CHECKER_SOURCES}
)
target_compile_options(checker
	PUBLIC ${SHARED_COMPILE_OPTIONS} -O3
)

target_link_libraries(checker
	m
	snappy
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
	m
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
target_link_libraries(benchmark
	m
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
	  -name "*.tcc"
	  -o
	  -name "*.hh"
	  )
	  -print0
	| xargs -0 clang-format -i
	COMMENT "Running clang-format..."
	VERBATIM
)

add_custom_target(tidy
	clang-tidy --quiet -p ${PROJECT_BINARY_DIR} ${HEADERS} ${SOURCES}
	DEPENDS testrunner
	COMMENT "Running clang-tidy..."
	VERBATIM
)

add_custom_target(cppcheck
	cppcheck --std=c++11 --language=c++ --enable=warning,style,performance --quiet --inline-suppr --suppress=unusedStructMember ${HEADERS} ${SOURCES}
	DEPENDS testrunner
	COMMENT "Running cppcheck..."
	VERBATIM
)
