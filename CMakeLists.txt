cmake_minimum_required(VERSION 3.13.0)

option(SNAPPY_BUILD_TESTS OFF)
add_subdirectory(3rdparty/snappy-1.1.8)

project(janus CXX)
include_directories(include . src 3rdparty/mbedtls-2.22.0/include 3rdparty/spdlog-1.5.0/include 3rdparty/snappy-1.1.8)
link_directories(3rdparty/mbedtls-2.22.0/library)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang.*")
set(CMAKE_COMPILER_IS_CLANGXX 1)
endif()

execute_process(COMMAND git describe OUTPUT_VARIABLE GIT_VER OUTPUT_STRIP_TRAILING_WHITESPACE)
set(SHARED_COMPILE_OPTIONS
	--std=gnu++2a -Wall -Wno-unknown-pragmas -g -D GIT_VER=${GIT_VER})

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

file(GLOB DB_SOURCES
	"src/db/*.cc"
)

# 3rd-party

execute_process(COMMAND grep -c processor  /proc/cpuinfo OUTPUT_VARIABLE NUM_CORES OUTPUT_STRIP_TRAILING_WHITESPACE)
add_custom_target(tls
	make --silent -C ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/mbedtls-2.22.0/library -j ${NUM_CORES} &&
	bash -c "ar -qc ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/mbedtls-2.22.0/library/libmbed.a ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/mbedtls-2.22.0/library/*.o"
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
	${DB_SOURCES}
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
	mbed
)

add_dependencies(jupiter tls)

# Osiris

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

file(GLOB_RECURSE OSIRIS_SOURCES
	"src/osiris/*.cc"
)

find_package(Qt5 COMPONENTS Core Widgets Charts REQUIRED)

file(GLOB_RECURSE OSIRIS_HEADERS
	"src/osiris/*.hh"
)

qt5_wrap_cpp(OSIRIS_UI_SOURCES ${OSIRIS_HEADERS})

add_executable(osiris
	${JANUS_SOURCES}
	${OSIRIS_UI_SOURCES}
	${OSIRIS_SOURCES}
	${DB_SOURCES}
)
target_compile_options(osiris
	PUBLIC ${SHARED_COMPILE_OPTIONS} -O3
)

target_link_libraries(osiris
	m
	snappy
	Qt5::Widgets
	Threads::Threads
	Qt5::Charts
)

# Apollo

file(GLOB_RECURSE APOLLO_SOURCES
	"src/apollo/*.cc"
)
add_executable(apollo
	${JANUS_SOURCES}
	${DB_SOURCES}
	${APOLLO_SOURCES}
)
target_compile_options(apollo
	PUBLIC ${SHARED_COMPILE_OPTIONS} -O3
)

target_link_libraries(apollo
	m
	pthread
	snappy
)

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

# Dump

file(GLOB_RECURSE DUMP_SOURCES
	"src/dump/*.cc"
)
add_executable(dump
	${JANUS_SOURCES}
	${DUMP_SOURCES}
	${DB_SOURCES}
)
target_compile_options(dump
	PUBLIC ${SHARED_COMPILE_OPTIONS} -O3
)

target_link_libraries(dump
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
	${NETWORK_SOURCES}
	${TEST_SOURCES}
	${DB_SOURCES}
)

if (CMAKE_COMPILER_IS_CLANGXX)
target_compile_options(testrunner
	PUBLIC ${SHARED_COMPILE_OPTIONS} -O0 -fno-omit-frame-pointer
)
else()
target_compile_options(testrunner
	PUBLIC ${SHARED_COMPILE_OPTIONS} -O0 -fno-omit-frame-pointer
	-fsanitize=address,leak,undefined,null,return,signed-integer-overflow,bounds,object-size
)
endif()

if (CMAKE_COMPILER_IS_CLANGXX)
target_link_libraries(testrunner
	gtest
	m
	mbed
	snappy
	pthread
)
else()
target_link_libraries(testrunner
	asan
	gtest
	m
	mbed
	snappy
	ubsan
	pthread
)
endif()

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
