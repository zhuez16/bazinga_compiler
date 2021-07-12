CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(bazinga_compiler)

set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -std=c99 -Wno-deprecated-register")
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_C_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
set(CMAKE_C_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
set(CMAKE_CXX_STANDARD 11)

SET(MID_LEVEL_DIR "${CMAKE_SOURCE_DIR}/tmp")
SET(MID_LEVEL_HEADER "${MID_LEVEL_DIR}/include")
file(MAKE_DIRECTORY ${MID_LEVEL_DIR})
file(MAKE_DIRECTORY ${MID_LEVEL_HEADER})

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

FLEX_TARGET(lexer ${CMAKE_SOURCE_DIR}/src/phase1/lexical_analyzer.l ${MID_LEVEL_DIR}/lex1.cpp)
FLEX_TARGET(lexer2 ${CMAKE_SOURCE_DIR}/src/phase2/analyzer.l ${MID_LEVEL_DIR}/lex2.cpp)
BISON_TARGET(parser ${CMAKE_SOURCE_DIR}/src/phase2/parser.y ${MID_LEVEL_DIR}/bs_parser.cpp DEFINES_FILE ${MID_LEVEL_DIR}/include/bs_parser.h COMPILE_FLAGS --debug)


INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${MID_LEVEL_DIR}/include)
ADD_EXECUTABLE(lexer
        src/test/test_lexer.cpp
        ${FLEX_lexer_OUTPUTS}
        )
ADD_EXECUTABLE(parser
        src/test/test_parser.cpp
        ${FLEX_lexer2_OUTPUTS}
        ${BISON_parser_OUTPUTS})