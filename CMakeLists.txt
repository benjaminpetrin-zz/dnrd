cmake_minimum_required(VERSION 2.8.9)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(CheckCCompilerFlag)
include(CheckSymbolExists)
include(CheckCSourceCompiles)
include(CheckPrototypeDefinition)
include(CheckTypeSize)
include(generate_version)

project (dnrd C)


include_directories(${CMAKE_BINARY_DIR}/gen)
set(CMAKE_C_FLAGS "-g -Os")



find_library(CONFIG_LIBRARIES NAMES pthread)
find_library(CONFIG_LIBRARIES NAMES config)
find_library(CONFIG_LIBRARIES NAMES unistd)

check_symbol_exists("usleep" "unistd.h" HAVE_USLEEP)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/build.h.in ${CMAKE_BINARY_DIR}/gen/generated/build.h)

add_executable(dnrd
src/args.c
src/cache.c
src/check.c
src/common.c
src/dns.c
src/domnode.c
src/lib.c
src/main.c
src/master.c
src/qid.c
src/query.c
src/rand.c
src/relay.c
src/sig.c
src/srvnode.c
src/tcp.c
src/udp.c
)

target_link_libraries(dnrd ${CONFIG_LIBRARIES})
#target_link_libraries(dnrd ${CONFIG_LIBRARIES})

