CMAKE_MINIMUM_REQUIRED(VERSION 3.14)
PROJECT(bazinga_compiler)

SET(MID_LEVEL_DIR "${CMAKE_SOURCE_DIR}/tmp")
file(MAKE_DIRECTORY ${MID_LEVEL_DIR})

find_package(BISON)
find_package(FLEX)

FLEX_TARGET(lexer ${CMAKE_SOURCE_DIR}/src/lexical_analyzer.l ${MID_LEVEL_DIR}/lex.cpp)


INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
ADD_EXECUTABLE( lexer
    src/test/test_lexer.cpp
    ${FLEX_lexer_OUTPUTS}
)